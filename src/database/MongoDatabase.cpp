#include "DatabaseBackend.h"
#include "DBBackendFactory.h"
#include "DatabaseServer.h"

#include "core/global.h"
#include "util/DatagramIterator.h"
#include "util/Datagram.h"

#include "dclass/dcAtomicField.h"
#include "dclass/dcSubatomicType.h"
#include "dclass/dcSimpleParameter.h"
#include "dclass/dcArrayParameter.h"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/types/value.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>

#include <limits>
#include <list>

using namespace std;
using namespace bsoncxx::builder::stream;

static ConfigGroup mongodb_backend_config("mongodb", db_backend_config);
static ConfigVariable<string> db_server("server", "mongodb://127.0.0.1/test",
                                        mongodb_backend_config);
static ConfigVariable<int> num_workers("workers", 8, mongodb_backend_config);

// These are helper functions to convert between BSON values and packed DC
// field values.
class ConversionException : public exception
{
  public:
    ConversionException(const char *desc) : m_desc(desc), m_what(desc) {}
    ConversionException(const char *desc, const string &name) : m_desc(desc)
    {
        push_name(name);
    }
    virtual const char *what() const throw()
    {
        return m_what.c_str();
    }
    void push_name(const string &name)
    {
        m_names.push_front(name);

        m_what = "";
        for(const auto &it : m_names) {
            if(m_what != "") {
                m_what += ".";
            }
            m_what += it;
        }
        m_what += ": " + m_desc;
    }
  private:
    string m_desc;
    string m_what;
    list<string> m_names;
};

static void dc2bson(single_context builder,
                        DCSimpleParameter *type, DatagramIterator &dgi)
{
    using namespace bsoncxx::types;
    switch(type->get_type()) {
    case ST_int8: {
        builder << b_int32 {dgi.read_int8()};
    }
    break;
    case ST_int16: {
        builder << b_int32 {dgi.read_int16()};
    }
    break;
    case ST_int32: {
        builder << b_int32 {dgi.read_int32()};
    }
    break;
    case ST_int64: {
        builder << b_int64 {dgi.read_int64()};
    }
    break;
    case ST_uint8: {
        builder << b_int32 {dgi.read_uint8()};
    }
    break;
    case ST_uint16: {
        builder << b_int32 {dgi.read_uint16()};
    }
    break;
    case ST_uint32: {
        builder << b_int64 {dgi.read_uint32()};
    }
    break;
    case ST_uint64: {
        // Note: Values over 1/2 the maximum will become negative.
        builder << b_int64 {static_cast<int64_t>(dgi.read_uint64())};
    }
    break;
    case ST_char: {
        unsigned char c = dgi.read_uint8();
        string str(1, c);
        builder << b_utf8 {str};
    }
    break;
    case ST_float64: {
        builder << b_double {dgi.read_float64()};
    }
    break;
    case ST_string: {
        vector<uint8_t> vec = dgi.read_data(type->get_fixed_byte_size());
        string str((const char *)vec.data(), vec.size());
        builder << b_utf8 {str};
    }
    break;
    case ST_blob: {
        vector<uint8_t> blob = dgi.read_data(type->get_fixed_byte_size());
        if(blob.data() == nullptr) {
            // libbson gets upset if passed a nullptr here, but it's valid for
            // vector.data() to return nullptr if it's empty, so we make
            // something up instead:
            builder << b_binary { bsoncxx::binary_sub_type::k_binary, 0, (const uint8_t*)1 };
        } else {
            builder << b_binary {
                bsoncxx::binary_sub_type::k_binary,
                static_cast<uint32_t>(blob.size()),
                blob.data()
            };
        }
    }
    break;
    case ST_blob32: {
        vector<uint8_t> blob = dgi.read_blob();
        if(blob.data() == nullptr) {
            // libbson gets upset if passed a nullptr here, but it's valid for
            // vector.data() to return nullptr if it's empty, so we make
            // something up instead:
            builder << b_binary { bsoncxx::binary_sub_type::k_binary, 0, (const uint8_t*)1 };
        } else {
            builder << b_binary {
                bsoncxx::binary_sub_type::k_binary,
                static_cast<uint32_t>(blob.size()),
                blob.data()
            };
        }
    }
    break;
    case ST_int8array:
    case ST_int16array:
    case ST_int32array:
    case ST_uint8array:
    case ST_uint16array:
    case ST_uint32uint8array: {
        const DCArrayParameter *array = type->as_array_parameter();

        auto sub_builder = builder << open_array;

        for(size_t i = 0; i < array->get_array_size(); i++) {
            dc2bson(sub_builder, array->get_element_type()->as_simple_parameter(), dgi);
        }

        sub_builder << close_array;
    }
    break;
    case ST_invalid:
    default:
        assert(false);
        break;
    }
}

template<typename T> T handle_bson_number(const bsoncxx::types::value &value)
{
    int64_t i;
    double d;
    bool is_double = false;
    if(value.type() == bsoncxx::type::k_int32) {
        i = value.get_int32().value;
    } else if(value.type() == bsoncxx::type::k_int64) {
        i = value.get_int64().value;
    } else if(value.type() == bsoncxx::type::k_double) {
        d = value.get_double().value;
        is_double = true;
    } else {
        throw ConversionException("Non-numeric BSON type encountered");
    }

    if(numeric_limits<T>::is_integer && is_double) {
        // We have to convert a double to an integer. Error if the double is
        // not floored.
        double intpart;
        if(modf(d, &intpart) != 0.0) {
            throw ConversionException("Non-integer double encountered");
        }

        if(d > numeric_limits<int64_t>::max() ||
           d < numeric_limits<int64_t>::min()) {
            throw ConversionException("Excessively large (or small) double encountered");
        }

        i = static_cast<int64_t>(d);
    } else if(!numeric_limits<T>::is_integer && !is_double) {
        // Integer to double. Simple enough:
        d = i;
    }

    // Special case: uint64_t is stored as an int64_t. If we're doing that,
    // just cast and return:
    if(is_same<T, uint64_t>::value) {
        return static_cast<uint64_t>(i);
    }

    // For floating types, we'll just cast and return:
    if(!numeric_limits<T>::is_integer) {
        return static_cast<T>(d);
    }

    // For everything else, cast if in range:
    int64_t max = static_cast<int64_t>(numeric_limits<T>::max());
    int64_t min = static_cast<int64_t>(numeric_limits<T>::min());

    if(i > max || i < min) {
        throw ConversionException("Integer is out of range");
    }

    return static_cast<T>(i);
}

static void bson2dc(DCSimpleParameter *type,
                    const string &field_name,
                    const bsoncxx::types::value &value,
                    Datagram &dg)
{
    try {
        switch(type->get_type()) {
        case ST_int8: {
            dg.add_int8(handle_bson_number<int8_t>(value));
        }
        break;
        case ST_int16: {
            dg.add_int16(handle_bson_number<int16_t>(value));
        }
        break;
        case ST_int32: {
            dg.add_int32(handle_bson_number<int32_t>(value));
        }
        break;
        case ST_int64: {
            dg.add_int64(handle_bson_number<int64_t>(value));
        }
        break;
        case ST_uint8: {
            dg.add_uint8(handle_bson_number<uint8_t>(value));
        }
        break;
        case ST_uint16: {
            dg.add_uint16(handle_bson_number<uint16_t>(value));
        }
        break;
        case ST_uint32: {
            dg.add_uint32(handle_bson_number<uint32_t>(value));
        }
        break;
        case ST_uint64: {
            dg.add_uint64(handle_bson_number<uint64_t>(value));
        }
        break;
        case ST_char: {
            if(value.type() != bsoncxx::type::k_utf8 || value.get_utf8().value.size() != 1) {
                throw ConversionException("Expected single-length string for char field");
            }
            dg.add_uint8(value.get_utf8().value[0]);
        }
        break;
        case ST_float64: {
            dg.add_float64(handle_bson_number<double>(value));
        }
        break;
        case ST_string: {
            if(value.type() != bsoncxx::type::k_utf8) {
                throw ConversionException("Expected string");
            }
            string str = value.get_utf8().value.to_string();
            dg.add_string(str);
        }
        break;
        case ST_blob: {
            if(value.type() != bsoncxx::type::k_binary) {
                throw ConversionException("Expected bindata");
            }
            auto binary = value.get_binary();
            dg.add_data(binary.bytes, binary.size);
        }
        break;
        case ST_blob32: {
            if(value.type() != bsoncxx::type::k_binary) {
                throw ConversionException("Expected bindata");
            }
            auto binary = value.get_binary();
            dg.add_blob(binary.bytes, binary.size);
        }
        break;
        case ST_int8array:
        case ST_int16array:
        case ST_int32array:
        case ST_uint8array:
        case ST_uint16array:
        case ST_uint32array: {
            if(value.type() != bsoncxx::type::k_array) {
                throw ConversionException("Expected array");
            }
            DCSimpleParameter *element_type = type->as_array_parameter()->get_element_type()->as_simple_parameter();
            auto array = value.get_array().value;

            size_t index = 0;
            for(const auto& it : array) {
                stringstream array_index;
                array_index << "[" << index++ << "]";
                bson2dc(element_type, array_index.str(), it.get_value(), dg);
            }
        }
        break;
        case ST_uint32uint8array: {
            if(value.type() != bsoncxx::type::k_array) {
                throw ConversionException("Expected array");
            }
            DCSimpleParameter *element_type = type->as_array_parameter()->get_element_type()->as_simple_parameter();
            auto array = value.get_array().value;

            size_t index = 0;
            for(const auto& it : array) {
                stringstream array_index;
                array_index << "[" << index++ << "]";
                bson2dc(element_type, array_index.str(), it.get_value(), dg);
            }
        }
        break;
        case ST_invalid:
        default:
            assert(false);
            break;
        }
    } catch(ConversionException &e) {
        e.push_name(field_name);
        throw;
    }
}

static mongocxx::instance mongo_inst;

class MongoDatabase : public DatabaseBackend
{
  public:
    MongoDatabase(ConfigNode dbeconfig, doid_t min_id, doid_t max_id) :
        DatabaseBackend(dbeconfig, min_id, max_id),
        m_shutdown(false)
    {
        stringstream log_name;
        log_name << "Database-MongoDB" << "(Range: [" << min_id << ", " << max_id << "])";
        m_log = new LogCategory("mongodb", log_name.str());

        // Init URI.
        try {
            m_uri = mongocxx::uri {db_server.get_rval(m_config)};
        } catch(mongocxx::logic_error &) {
            m_log->fatal() << "Could not parse URI!" << endl;
            exit(1);
        }

        // Init the globals collection/document:
        auto client = new_connection();
        auto globals = client[m_uri.database()]["otp.globals"].find_one(
                           document {} << "_id" << "GLOBALS" << finalize);
        if(!globals) {
            client[m_uri.database()]["otp.globals"].insert_one(
                document {} << "_id" << "GLOBALS"
                << "doid" << open_document
                << "monotonic" << static_cast<int64_t>(min_id)
                << "free" << open_array << close_array
                << close_document << finalize);
        }

        // Spawn worker threads:
        for(int i = 0; i < num_workers.get_rval(m_config); ++i) {
            m_threads.push_back(new thread(bind(&MongoDatabase::run_thread, this)));
        }
    }

    ~MongoDatabase()
    {
        // Shutdown threads:
        {
            lock_guard<mutex> guard(m_lock);
            m_shutdown = true;
            m_cv.notify_all();
        }
        for(auto &it : m_threads) {
            it->join();
            delete it;
        }

        delete m_log;
    }

    virtual void submit(DBOperation *operation)
    {
        lock_guard<mutex> guard(m_lock);
        m_operation_queue.push(operation);
        m_cv.notify_one();
    }

  private:
    LogCategory *m_log;

    mongocxx::uri m_uri;

    queue<DBOperation *> m_operation_queue;
    condition_variable m_cv;

    mutex m_lock;
    vector<thread *> m_threads;
    bool m_shutdown;

    mongocxx::client new_connection()
    {
        return (mongocxx::client {m_uri});
    }

    void run_thread()
    {
        unique_lock<mutex> guard(m_lock);

        auto client = new_connection();
        mongocxx::database db = client[m_uri.database()];

        while(true) {
            if(m_operation_queue.size() > 0) {
                DBOperation *op = m_operation_queue.front();
                m_operation_queue.pop();

                guard.unlock();
                handle_operation(db, op);
                guard.lock();
            } else if(m_shutdown) {
                break;
            } else {
                m_cv.wait(guard);
            }
        }
    }

    void handle_operation(mongocxx::database &db, DBOperation *operation)
    {
        // First, figure out what kind of operation it is, and dispatch:
        switch(operation->type()) {
        case DBOperation::OperationType::CREATE_OBJECT: {
            handle_create(db, operation);
        }
        break;
        case DBOperation::OperationType::DELETE_OBJECT: {
            handle_delete(db, operation);
        }
        break;
        case DBOperation::OperationType::GET_OBJECT:
        case DBOperation::OperationType::GET_FIELDS: {
            handle_get(db, operation);
        }
        break;
        case DBOperation::OperationType::SET_FIELDS:
        case DBOperation::OperationType::UPDATE_FIELDS: {
            handle_modify(db, operation);
        }
        break;
        }
    }

    void handle_create(mongocxx::database &db, DBOperation *operation)
    {
        // First, let's convert the requested object into BSON; this way, if
        // a failure happens, it happens before we waste a doid.

        auto builder = document {};
        try {
            for(const auto& it : operation->set_fields()) {
                DatagramPtr dg = Datagram::create();
                dg->add_data(it.second);
                DatagramIterator dgi(dg);

                DCAtomicField *atomic_field = it.first->as_atomic_field();
                size_t parameters = atomic_field->get_num_elements();

                for(unsigned int i = 0; i < parameters; ++i) {
                    // Grab each simple parameter and run through it in dc2bson.
                    DCParameter *parameter = atomic_field->get_element(i);
                    DCSimpleParameter *simple_parameter = parameter->as_simple_parameter();

                    dc2bson(builder << it.first->get_name(), simple_parameter, dgi);
                }
            }
        } catch(ConversionException &e) {
            m_log->error() << "While formatting "
                           << operation->dclass()->get_name()
                           << " for insertion: " << e.what() << endl;
            operation->on_failure();
            return;
        }
        auto fields = builder << finalize;

        doid_t doid = assign_doid(db);
        if(doid == INVALID_DO_ID) {
            // The error will already have been emitted at this point, so
            // all that's left for us to do is fail silently:
            operation->on_failure();
            return;
        }

        m_log->trace() << "Inserting new " << operation->dclass()->get_name()
                       << "(" << doid << "): " << bsoncxx::to_json(fields) << endl;

        try {
            db["otp.objects"].insert_one(document {}
                                            << "_id" << static_cast<int64_t>(doid)
                                            << "dclass" << operation->dclass()->get_name()
                                            << "fields" << fields
                                            << finalize);
        } catch(mongocxx::operation_exception &e) {
            m_log->error() << "Cannot insert new "
                           << operation->dclass()->get_name()
                           << "(" << doid << "): " << e.what() << endl;
            operation->on_failure();
            return;
        }

        operation->on_complete(doid);
    }

    void handle_delete(mongocxx::database &db, DBOperation *operation)
    {
        bool success;
        try {
            auto result = db["otp.objects"].delete_one(document {}
                          << "_id" << static_cast<int64_t>(operation->doid()) <<
                          finalize);
            success = result && (result->deleted_count() == 1);
        } catch(mongocxx::operation_exception &e) {
            m_log->error() << "Unexpected error while deleting "
                           << operation->doid() << ": " << e.what() << endl;
            operation->on_failure();
            return;
        }

        if(success) {
            free_doid(db, operation->doid());
            operation->on_complete();
        } else {
            m_log->error() << "Tried to delete non-existent doid "
                           << operation->doid() << endl;
            operation->on_failure();
        }
    }

    void handle_get(mongocxx::database &db, DBOperation *operation)
    {
        bsoncxx::stdx::optional<bsoncxx::document::value> obj;
        try {
            obj = db["otp.objects"].find_one(document {}
                                                << "_id" << static_cast<int64_t>(operation->doid())
                                                << finalize);
        } catch(mongocxx::operation_exception &e) {
            m_log->error() << "Unexpected error occurred while trying to"
                           " retrieve object with DOID "
                           << operation->doid() << ": " << e.what() << endl;
            operation->on_failure();
            return;
        }

        if(!obj) {
            m_log->warning() << "Got queried for non-existent object with DOID "
                             << operation->doid() << endl;
            operation->on_failure();
            return;
        }

        DBObjectSnapshot *snap = format_snapshot(operation->doid(), *obj);
        if(!snap || !operation->verify_class(snap->m_dclass)) {
            operation->on_failure();
        } else {
            operation->on_complete(snap);
        }
    }

    void handle_modify(mongocxx::database &db, DBOperation *operation)
    {
        // Format the changes to be made:
        document sets_builder {};
        document unsets_builder {};
        for(const auto& it : operation->set_fields()) {
            stringstream fieldname;
            fieldname << "fields." << it.first->get_name();
            if(it.second.empty()) {
                unsets_builder << fieldname.str() << true;
            } else {
                DatagramPtr dg = Datagram::create();
                dg->add_data(it.second);
                DatagramIterator dgi(dg);

                DCAtomicField *atomic_field = it.first->as_atomic_field();
                size_t parameters = atomic_field->get_num_elements();

                for(unsigned int i = 0; i < parameters; ++i) {
                    // Grab each simple parameter and run through it in dc2bson.
                    DCParameter *parameter = atomic_field->get_element(i);
                    DCSimpleParameter *simple_parameter = parameter->as_simple_parameter();

                    dc2bson(sets_builder << fieldname.str(), simple_parameter, dgi);
                }
            }
        }
        auto sets = sets_builder << finalize;
        auto unsets = unsets_builder << finalize;

        auto updates_builder = document {};
        if(!sets.view().empty()) updates_builder << "$set" << sets;
        if(!unsets.view().empty()) updates_builder << "$unset" << unsets;
        auto updates = updates_builder << finalize;

        // Also format any criteria for the change:
        document query {};
        query << "_id" << static_cast<int64_t>(operation->doid());
        for(const auto &it : operation->criteria_fields()) {
            stringstream fieldname;
            fieldname << "fields." << it.first->get_name();
            if(it.second.empty()) {
                query << fieldname.str() << open_document << "$exists" << false << close_document;
            } else {
                DatagramPtr dg = Datagram::create();
                dg->add_data(it.second);
                DatagramIterator dgi(dg);

                DCAtomicField *atomic_field = it.first->as_atomic_field();
                size_t parameters = atomic_field->get_num_elements();

                for(unsigned int i = 0; i < parameters; ++i) {
                    // Grab each simple parameter and run through it in dc2bson.
                    DCParameter *parameter = atomic_field->get_element(i);
                    DCSimpleParameter *simple_parameter = parameter->as_simple_parameter();

                    dc2bson(query << fieldname.str(), simple_parameter, dgi);
                }
            }
        }
        auto query_obj = query << finalize;

        m_log->trace() << "Performing updates to " << operation->doid()
                       << ": " << bsoncxx::to_json(updates) << endl;
        m_log->trace() << "Query is: " << bsoncxx::to_json(query_obj) << endl;

        bsoncxx::stdx::optional<bsoncxx::document::value> obj;
        try {
            obj = db["otp.objects"].find_one_and_update(query_obj.view(), updates.view());
        } catch(mongocxx::operation_exception &e) {
            m_log->error() << "Unexpected error while modifying "
                           << operation->doid() << ": " << e.what() << endl;
            operation->on_failure();
            return;
        }

        if(obj) {
            m_log->trace() << "Update result: " << bsoncxx::to_json(*obj) << endl;
        } else {
            m_log->trace() << "Update result: [server could not find a matching object]" << endl;
        }

        if(!obj) {
            // Okay, something didn't work right. If we had criteria, let's
            // try to fetch the object without the criteria to see if it's a
            // criteria mismatch or a missing DOID.
            if(!operation->criteria_fields().empty()) {
                try {
                    obj = db["otp.objects"].find_one(document {}
                                                        << "_id" << static_cast<int64_t>(operation->doid()) << finalize);
                } catch(mongocxx::operation_exception &e) {
                    m_log->error() << "Unexpected error while modifying "
                                   << operation->doid() << ": " << e.what() << endl;
                    operation->on_failure();
                    return;
                }
                if(obj) {
                    // There's the problem. Now we can send back a snapshot:
                    DBObjectSnapshot *snap = format_snapshot(operation->doid(), *obj);
                    if(snap && operation->verify_class(snap->m_dclass)) {
                        operation->on_criteria_mismatch(snap);
                        return;
                    } else {
                        // Something else weird happened with our snapshot;
                        // either the class wasn't recognized or it was the
                        // wrong class. Either way, an error has been logged,
                        // and we need to fail the operation.
                        operation->on_failure();
                        return;
                    }
                }
            }

            // Nope, not that. We're missing the DOID.
            m_log->error() << "Attempted to modify unknown DOID: "
                           << operation->doid() << endl;
            operation->on_failure();
            return;
        }

        // If we've gotten to this point: Hooray! The change has gone
        // through to the database.
        // Let's, however, double-check our changes. (Specifically, we should
        // run verify_class so that we know the frontend is happy with what
        // kind of object we just modified.)
        auto obj_v = obj->view();
        string dclass_name = obj_v["dclass"].get_utf8().value.to_string();
        DCClass *dclass = g_dcf->get_class_by_name(dclass_name);
        if(!dclass) {
            m_log->error() << "Encountered unknown database object: "
                           << dclass_name << "(" << operation->doid() << ")" << endl;
        } else if(operation->verify_class(dclass)) {
            // Yep, it all checks out. Complete the operation:
            operation->on_complete();
            return;
        }

        // If we've gotten here, something is seriously wrong. We've just
        // mucked with an object without knowing the consequences! What have
        // we done?! We've created an abomination in the database! Kill it!
        // Kill it with fire!

        // All we really can do to mitigate this is scream at the user (which
        // the above verification has already done by now) and revert the
        // object back to how it was when we found it.
        // NOTE: This DOES have the potential for data loss, because we're
        // wiping out any changes that conceivably could have happened
        // between the findandmodify and now. In dev environments, (which we
        // are probably in right now, if other components are making
        // outlandish requests like this) this shouldn't be a huge issue.
        m_log->trace() << "Reverting changes made to " << operation->doid() << endl;
        try {
            db["otp.objects"].replace_one(
                document {} << "_id" << static_cast<int64_t>(operation->doid()) << finalize,
                obj_v);
        } catch(mongocxx::operation_exception &e) {
            // Wow, we REALLY fail at life.
            m_log->error() << "Could not revert corrupting changes to "
                           << operation->doid() << ": " << e.what() << endl;
        }
        operation->on_failure();
    }

    // Get a DBObjectSnapshot from a MongoDB BSON object; returns nullptr if failure.
    DBObjectSnapshot *format_snapshot(doid_t doid, bsoncxx::document::view obj)
    {
        m_log->trace() << "Formatting database snapshot of " << doid << ": "
                       << bsoncxx::to_json(obj) << endl;

        string dclass_name = obj["dclass"].get_utf8().value.to_string();
        DCClass *dclass = g_dcf->get_class_by_name(dclass_name);
        if(!dclass) {
            m_log->error() << "Encountered unknown database object: "
                           << dclass_name << "(" << doid << ")" << endl;
            return nullptr;
        }

        auto fields = obj["fields"].get_document().value;

        DBObjectSnapshot *snap = new DBObjectSnapshot();
        snap->m_dclass = dclass;

        try {
            for(const auto &it : fields) {
                string name = it.key().to_string();
                DCField *field = dclass->get_field_by_name(name);
                if(!field) {
                    m_log->warning() << "Encountered unexpected field " << name
                                     << " while formatting " << dclass_name
                                     << "(" << doid << "); ignored." << endl;
                    continue;
                }

                DCAtomicField *atomic_field = field->as_atomic_field();
                size_t parameters = atomic_field->get_num_elements();

                // Create the datagram that bson2dc will store
                // the field data in.
                DatagramPtr dg = Datagram::create();

                for(unsigned int i = 0; i < parameters; ++i) {
                    // Grab the type through the simple parameter.
                    DCParameter *parameter = atomic_field->get_element(i);
                    DCSimpleParameter *simple_parameter = parameter->as_simple_parameter();

                    // Run through the parameter in bson2dc.
                    bson2dc(simple_parameter, field->get_name(), it.get_value(), *dg);
                }

                snap->m_fields[field].resize(dg->size());
                memcpy(snap->m_fields[field].data(), dg->get_data(), dg->size());
            }
        } catch(ConversionException &e) {
            m_log->error() << "Unexpected error while trying to format"
                           " database snapshot for " << dclass_name << "(" << doid << "): "
                           << e.what() << endl;
            delete snap;
            return nullptr;
        }

        return snap;
    }

    // This function is used by handle_create to get a fresh DOID assignment.
    doid_t assign_doid(mongocxx::database &db)
    {
        try {
            doid_t doid = assign_doid_monotonic(db);
            if(doid != INVALID_DO_ID) {
                return doid;
            }

            // We've exhausted our supply of doids from the monotonic counter.
            // We must now resort to pulling things out of the free list:
            return assign_doid_reuse(db);
        } catch(mongocxx::operation_exception &e) {
            m_log->error() << "Unexpected error occurred while trying to"
                           " allocate a new DOID: " << e.what() << endl;
            return INVALID_DO_ID;
        }
    }

    doid_t assign_doid_monotonic(mongocxx::database &db)
    {
        auto obj = db["otp.globals"].find_one_and_update(
                       document {} << "_id" << "GLOBALS"
                       << "doid.monotonic" << open_document << "$gte" << static_cast<int64_t>(m_min_id) << close_document
                       << "doid.monotonic" << open_document << "$lte" << static_cast<int64_t>(m_max_id) << close_document
                       << finalize,
                       document {} << "$inc" << open_document
                       << "doid.monotonic" << 1
                       << close_document << finalize);

        // If the findandmodify command failed, the document either doesn't
        // exist, or we ran out of monotonic doids.
        if(!obj) {
            return INVALID_DO_ID;
        }

        m_log->trace() << "assign_doid_monotonic: got globals element: "
                       << bsoncxx::to_json(*obj) << endl;

        doid_t doid = handle_bson_number<doid_t>(obj->view()["doid"]["monotonic"].get_value());
        return doid;
    }

    // This is used when the monotonic counter is exhausted:
    doid_t assign_doid_reuse(mongocxx::database &db)
    {
        auto obj = db["otp.globals"].find_one_and_update(
                       document {} << "_id" << "GLOBALS"
                       << "doid.free.0" << open_document
                       << "$exists" << true
                       << close_document << finalize,
                       document {} << "$pop" << open_document
                       << "doid.free" << -1
                       << close_document << finalize);

        // If the findandmodify command failed, the document either doesn't
        // exist, or we ran out of reusable doids.
        if(!obj) {
            m_log->error() << "Could not allocate a reused DOID!" << endl;
            return INVALID_DO_ID;
        }

        m_log->trace() << "assign_doid_reuse: got globals element: "
                       << bsoncxx::to_json(*obj) << endl;

        // Otherwise, use the first one:
        doid_t doid = handle_bson_number<doid_t>
                      (obj->view()["doid"]["free"].get_array().value[0].get_value());
        return doid;
    }

    // This returns a DOID to the free list:
    void free_doid(mongocxx::database &db, doid_t doid)
    {
        m_log->trace() << "Returning doid " << doid << " to the free pool..." << endl;

        try {
            db["otp.globals"].update_one(
                document {} << "_id" << "GLOBALS" << finalize,
                document {} << "$push" << open_document
                << "doid.free" << static_cast<int64_t>(doid)
                << close_document << finalize);
        } catch(mongocxx::operation_exception &e) {
            m_log->error() << "Could not return doid " << doid
                           << " to free pool: " << e.what() << endl;
        }
    }
};

DBBackendFactoryItem<MongoDatabase> mongodb_factory("mongodb");
