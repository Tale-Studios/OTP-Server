#include "DistributedObject.h"
#include <unordered_set>
#include "core/global.h"
#include "core/msgtypes.h"
#include "dclass/dcClass.h"
#include "dclass/dcField.h"
#include "dclass/dcAtomicField.h"
#include "dclass/dcMolecularField.h"
using namespace std;

DistributedObject::DistributedObject(StateServer *stateserver, doid_t do_id, doid_t parent_id,
                                     zone_t zone_id, DCClass *dclass, DatagramIterator &dgi,
                                     bool has_other) :
    m_stateserver(stateserver), m_do_id(do_id), m_parent_id(INVALID_DO_ID), m_zone_id(0),
    m_dclass(dclass), m_ai_channel(INVALID_CHANNEL), m_owner_channel(INVALID_CHANNEL),
    m_ai_explicitly_set(false), m_parent_synchronized(false), m_next_context(0)
{
    stringstream name;
    name << dclass->get_name() << "(" << do_id << ")";
    m_log = new LogCategory("object", name.str());
    set_con_name(name.str());

    for(int i = 0; i < m_dclass->get_num_inherited_fields(); ++i) {
        DCField *field = m_dclass->get_inherited_field(i);
        if(field->is_required() && !field->as_molecular_field()) {
            dgi.unpack_field(field, m_required_fields[field]);
        }
    }

    if(has_other) {
        uint16_t count = dgi.read_uint16();
        for(int i = 0; i < count; ++i) {
            uint16_t field_id = dgi.read_uint16();
            DCField *field = m_dclass->get_field_by_index(field_id);
            if(!field) {
                m_log->error() << "Received unknown field with ID " << field_id
                               << " within an OTHER section.\n";
                break;
            }

            if(field->is_ram()) {
                dgi.unpack_field(field, m_ram_fields[field]);
            } else {
                m_log->error() << "Received non-RAM field " << field->get_name()
                               << " within an OTHER section.\n";
                dgi.skip_field(field);
            }
        }
    }

    subscribe_channel(do_id);

    m_log->debug() << "Object created..." << endl;

    dgi.seek_payload(); // Seek back to front of payload, to read sender.
    handle_location_change(parent_id, zone_id, dgi.read_channel());
    wake_children();
}

DistributedObject::DistributedObject(StateServer *stateserver, channel_t sender, doid_t do_id,
                                     doid_t parent_id, zone_t zone_id, DCClass *dclass,
                                     UnorderedFieldValues& required, FieldValues& ram) :
    m_stateserver(stateserver), m_do_id(do_id), m_parent_id(INVALID_DO_ID), m_zone_id(0),
    m_dclass(dclass), m_ai_channel(INVALID_CHANNEL), m_owner_channel(INVALID_CHANNEL),
    m_ai_explicitly_set(false), m_next_context(0)
{
    stringstream name;
    name << dclass->get_name() << "(" << do_id << ")";
    m_log = new LogCategory("object", name.str());

    m_required_fields = required;
    m_ram_fields = ram;

    subscribe_channel(do_id);
    handle_location_change(parent_id, zone_id, sender);
    wake_children();
}

DistributedObject::~DistributedObject()
{
    if(m_log) {
        delete m_log;
        m_log = nullptr;
    }
}

void DistributedObject::append_all_data(DatagramPtr dg)
{
    dg->add_doid(m_do_id);
    dg->add_location(m_parent_id, m_zone_id);
    dg->add_uint16(m_dclass->get_number());
    size_t field_count = m_dclass->get_num_inherited_fields();
    for(size_t i = 0; i < field_count; ++i) {
        DCField *field = m_dclass->get_inherited_field(i);
        if(field->is_required() && !field->as_molecular_field()) {
            // We only want to append all required atomic fields.
            // This should also work for owned objects.
            dg->add_data(m_required_fields[field]);
        }
    }
}

void DistributedObject::append_required_data(DatagramPtr dg, bool client_only)
{
    dg->add_doid(m_do_id);
    dg->add_location(m_parent_id, m_zone_id);
    dg->add_uint16(m_dclass->get_number());
    size_t field_count = m_dclass->get_num_inherited_fields();
    for(size_t i = 0; i < field_count; ++i) {
        DCField *field = m_dclass->get_inherited_field(i);
        if(field->is_required() && !field->as_molecular_field() && (!client_only
                || field->is_broadcast() || field->is_clrecv())) {
            dg->add_data(m_required_fields[field]);
        }
    }
}

void DistributedObject::append_other_data(DatagramPtr dg, bool client_only)
{
    if(client_only) {
        vector<DCField*> broadcast_fields;
        for(auto it = m_ram_fields.begin(); it != m_ram_fields.end(); ++it) {
            if(it->first->is_broadcast() || it->first->is_clrecv()) {
                broadcast_fields.push_back(it->first);
            }
        }

        dg->add_uint16(broadcast_fields.size());
        for(auto it = broadcast_fields.begin(); it != broadcast_fields.end(); ++it) {
            dg->add_uint16((*it)->get_number());
            dg->add_data(m_ram_fields[*it]);
        }
    } else {
        dg->add_uint16(m_ram_fields.size());
        for(auto it = m_ram_fields.begin(); it != m_ram_fields.end(); ++it) {
            dg->add_uint16(it->first->get_number());
            dg->add_data(it->second);
        }
    }
}

void DistributedObject::send_interest_entry(channel_t location, uint32_t context)
{
    DatagramPtr dg = Datagram::create(location, m_do_id, m_ram_fields.size() ?
                                      STATESERVER_OBJECT_ENTER_INTEREST_WITH_REQUIRED_OTHER :
                                      STATESERVER_OBJECT_ENTER_INTEREST_WITH_REQUIRED);
    dg->add_uint32(context);
    append_required_data(dg, true);
    if(m_ram_fields.size()) {
        append_other_data(dg, true);
    }
    route_datagram(dg);
}

void DistributedObject::send_location_entry(unordered_set<channel_t> targets)
{
    DatagramPtr dg = Datagram::create(targets, m_do_id, m_ram_fields.size() ?
                                      STATESERVER_OBJECT_ENTER_LOCATION_WITH_REQUIRED_OTHER :
                                      STATESERVER_OBJECT_ENTER_LOCATION_WITH_REQUIRED);
    append_required_data(dg, true);
    if(m_ram_fields.size()) {
        append_other_data(dg, true);
    }
    route_datagram(dg);
}

void DistributedObject::send_ai_entry(channel_t ai)
{
    DatagramPtr dg = Datagram::create(ai, m_do_id, m_ram_fields.size() ?
                                      STATESERVER_OBJECT_ENTER_AI_WITH_REQUIRED_OTHER :
                                      STATESERVER_OBJECT_ENTER_AI_WITH_REQUIRED);
    append_required_data(dg);
    if(m_ram_fields.size()) {
        append_other_data(dg);
    }
    route_datagram(dg);
}

void DistributedObject::send_owner_entry(channel_t owner)
{
    // Owner entries are required fields trailed by other fields.
    DatagramPtr dg = Datagram::create(owner, m_do_id, STATESERVER_OBJECT_ENTER_OWNER_WITH_REQUIRED_OTHER);
    append_all_data(dg);
    append_other_data(dg, true);
    route_datagram(dg);
}

void DistributedObject::handle_location_change(doid_t new_parent, zone_t new_zone, channel_t sender)
{
    doid_t old_parent = m_parent_id;
    zone_t old_zone = m_zone_id;

    // Set of channels that must be notified about location_change
    unordered_set<channel_t> targets;

    // Notify AI of changing location
    if(m_ai_channel) {
        targets.insert(m_ai_channel);
    }

    // Notify Owner of changing location
    if(m_owner_channel) {
        targets.insert(m_owner_channel);
    }

    if(new_parent == m_do_id) {
        m_log->warning() << "Object cannot be parented to itself.\n";
        return;
    }

    // Handle parent change
    if(new_parent != old_parent) {
        // Unsubscribe from the old parent's child-broadcast channel.
        if(old_parent) { // If we have an old parent
            unsubscribe_channel(parent_to_children(m_parent_id));
            // Notify old parent of changing location
            targets.insert(old_parent);
            // Notify old location of changing location
            targets.insert(location_as_channel(old_parent, old_zone));
            // Notify object channel of changing location
            targets.insert(object_as_channel(old_parent, m_do_id));
        }

        m_parent_id = new_parent;
        m_zone_id = new_zone;

        // Subscribe to new one...
        if(new_parent) { // If we have a new parent
            subscribe_channel(parent_to_children(m_parent_id));
            if(!m_ai_explicitly_set) {
                // Ask the new parent what its AI is.
                DatagramPtr dg = Datagram::create(m_parent_id, m_do_id, STATESERVER_OBJECT_GET_AI);
                dg->add_uint32(m_next_context++);
                route_datagram(dg);
            }
            targets.insert(new_parent); // Notify new parent of changing location
        } else if(!m_ai_explicitly_set) {
            m_ai_channel = 0;
        }
    } else if(new_zone != old_zone) {
        m_zone_id = new_zone;
        // Notify parent of changing zone
        targets.insert(m_parent_id);
        // Notify old location of changing location
        targets.insert(location_as_channel(m_parent_id, old_zone));
        // Notify object channel of changing location
        targets.insert(object_as_channel(m_parent_id, m_do_id));
    } else {
        return; // Not actually changing location, no need to handle.
    }

    // Send changing location message
    DatagramPtr dg = Datagram::create(targets, sender, STATESERVER_OBJECT_CHANGING_LOCATION);
    dg->add_doid(m_do_id);
    dg->add_location(new_parent, new_zone);
    dg->add_location(old_parent, old_zone);
    route_datagram(dg);

    // At this point, the new parent (which may or may not be the same as the
    // old parent) is unaware of our existence in this zone.
    m_parent_synchronized = false;

    // Send enter location message
    if(new_parent) {
        unordered_set<channel_t> locations;
        locations.insert(location_as_channel(new_parent, new_zone));
        locations.insert(object_as_channel(new_parent, m_do_id));
        send_location_entry(locations);
    }
}

void DistributedObject::handle_ai_change(channel_t new_ai, channel_t sender,
        bool channel_is_explicit)
{
    channel_t old_ai = m_ai_channel;
    if(new_ai == old_ai) {
        return;
    }

    // Set of channels that must be notified about ai_change
    unordered_set<channel_t> targets;

    if(old_ai) {
        targets.insert(old_ai);
    }
    if(!m_zone_objects.empty()) {
        // We have at least one child, so we want to notify the children as well
        targets.insert(parent_to_children(m_do_id));
    }

    m_ai_channel = new_ai;
    m_ai_explicitly_set = channel_is_explicit;

    DatagramPtr dg = Datagram::create(targets, sender, STATESERVER_OBJECT_CHANGING_AI);
    dg->add_doid(m_do_id);
    dg->add_channel(new_ai);
    dg->add_channel(old_ai);
    route_datagram(dg);

    if(new_ai) {
        m_log->trace() << "Sending AI entry to " << new_ai << ".\n";
        send_ai_entry(new_ai);
    }
}

void DistributedObject::begin_delete(bool ai_deletion)
{
    if(m_parent_id) {
        // Leave parent on explicit delete ram
        DatagramPtr dg = Datagram::create(m_parent_id, m_do_id, STATESERVER_OBJECT_CHANGING_LOCATION);
        dg->add_doid(m_do_id);
        dg->add_location(INVALID_DO_ID, 0);
        dg->add_location(m_parent_id, m_zone_id);
        route_datagram(dg);
    }

    std::unordered_set<doid_t> doids;

    for(auto kv : m_zone_objects) {
        for(auto it : kv.second) {
            doids.insert(it);
        }
    }

    m_deletion_count = doids.size();

    if(doids.size() < 1) {
        // We have no children. We can finish the deletion process now.
        if(m_parent_id) {
            // However, if we have a parent, we need to acknowledge
            // our deletion.
            DatagramPtr dg = Datagram::create(m_parent_id, m_do_id, STATESERVER_OBJECT_DELETE_RAM);
            dg->add_doid(m_do_id);
            dg->add_bool(ai_deletion);
            route_datagram(dg);

        }
        finish_delete(false, ai_deletion);
        return;
    }

    m_deletion_process = true;
    delete_children(ai_deletion);
}

void DistributedObject::finish_delete(bool notify_parent, bool ai_deletion)
{
    unordered_set<channel_t> targets;
    if(m_parent_id) {
        if(notify_parent) {
            // We do have a parent and need to acknowledge our own deletion.
            DatagramPtr dg = Datagram::create(m_parent_id, m_do_id, STATESERVER_OBJECT_DELETE_RAM);
            dg->add_doid(m_do_id);
            dg->add_bool(ai_deletion);
            route_datagram(dg);
        }
        targets.insert(location_as_channel(m_parent_id, m_zone_id));
        targets.insert(object_as_channel(m_parent_id, m_do_id));
    }
    if(m_owner_channel) {
        targets.insert(m_owner_channel);
    }
    if(m_ai_channel) {
        targets.insert(m_ai_channel);
    }

    DatagramPtr dg = Datagram::create(targets, m_do_id, STATESERVER_OBJECT_DELETE_RAM);
    dg->add_doid(m_do_id);
    dg->add_bool(ai_deletion);
    route_datagram(dg);

    m_stateserver->m_objs.erase(m_do_id);
    m_stateserver->m_dcids.erase(m_do_id);
    m_stateserver->free_id(m_do_id);
    m_log->debug() << "Deleted.\n";

    terminate();
}

void DistributedObject::delete_children(bool ai_deletion)
{
    // Delete each child.
    DatagramPtr dg = Datagram::create(parent_to_children(m_do_id), m_do_id,
                                      STATESERVER_OBJECT_DELETE_CHILDREN);
    dg->add_bool(ai_deletion);
    route_datagram(dg);
}

void DistributedObject::wake_children()
{
    DatagramPtr dg = Datagram::create(parent_to_children(m_do_id), m_do_id,
                                      STATESERVER_OBJECT_GET_LOCATION);
    dg->add_uint32(STATESERVER_CONTEXT_WAKE_CHILDREN);
    route_datagram(dg);
}

void DistributedObject::save_field(DCField *field, const vector<uint8_t> &data)
{
    if(field->is_required()) {
        m_required_fields[field] = data;
    } else if(field->is_ram()) {
        m_ram_fields[field] = data;
    }
}

bool DistributedObject::handle_one_update(DatagramIterator &dgi, channel_t sender)
{
    vector<uint8_t> data;
    uint16_t field_id = dgi.read_uint16();
    DCField *field = m_dclass->get_field_by_index(field_id);
    if(!field) {
        m_log->error() << "Received set_field for field: " << field_id
                       << ", not valid for class: " << m_dclass->get_name() << ".\n";
        return false;
    }

    m_log->trace() << "Handling update for '" << field->get_name() << "'.\n";

    dgsize_t field_start = dgi.tell();

    try {
        dgi.unpack_field(field, data);
    } catch(const DatagramIteratorEOF&) {
        m_log->error() << "Received truncated update for " << field->get_name() << ".\n";
        return false;
    }

    DCMolecularField *molecular = field->as_molecular_field();
    if(molecular) {
        dgi.seek(field_start);
        int n = molecular->get_num_atomics();
        for(int i = 0; i < n; ++i) {
            vector<uint8_t> field_data;
            DCAtomicField *atomic = molecular->get_atomic(i);
            dgi.unpack_field(atomic, field_data);
            save_field(atomic->as_field(), field_data);
        }
    } else {
        save_field(field, data);
    }

    unordered_set<channel_t> targets;
    if(field->is_broadcast()) {
        targets.insert(location_as_channel(m_parent_id, m_zone_id));
        targets.insert(object_as_channel(m_parent_id, m_do_id));
    }
    if(field->is_airecv() && m_ai_channel && m_ai_channel != sender) {
        targets.insert(m_ai_channel);
    }
    if(field->is_ownrecv() && m_owner_channel && m_owner_channel != sender) {
        targets.insert(m_owner_channel);
    }
    if(targets.size()) { // TODO: Review this for efficiency?
        DatagramPtr dg = Datagram::create(targets, sender, STATESERVER_OBJECT_SET_FIELD);
        dg->add_doid(m_do_id);
        dg->add_uint16(field_id);
        dg->add_data(data);
        route_datagram(dg);
    }
    return true;
}

bool DistributedObject::handle_one_get(DatagramPtr out, uint16_t field_id,
                                       bool succeed_if_unset, bool is_subfield)
{
    DCField *field = m_dclass->get_field_by_index(field_id);
    if(!field) {
        m_log->error() << "Received get_field for field: " << field_id
                       << ", not valid for class: " << m_dclass->get_name() << ".\n";
        return false;
    }
    m_log->trace() << "Handling query for '" << field->get_name() << "'.\n";

    DCMolecularField *molecular = field->as_molecular_field();
    if(molecular) {
        int n = molecular->get_num_atomics();
        out->add_uint16(field_id);
        for(int i = 0; i < n; ++i) {
            if(!handle_one_get(out, molecular->get_atomic(i)->get_number(), succeed_if_unset, true)) {
                return false;
            }
        }
        return true;
    }

    if(m_required_fields.count(field)) {
        if(!is_subfield) {
            out->add_uint16(field_id);
        }
        out->add_data(m_required_fields[field]);
    } else if(m_ram_fields.count(field)) {
        if(!is_subfield) {
            out->add_uint16(field_id);
        }
        out->add_data(m_ram_fields[field]);
    } else {
        return succeed_if_unset;
    }

    return true;
}

void DistributedObject::handle_object_query(DatagramIterator &dgi, uint16_t msg_type,
                                            channel_t sender, bool do_id, bool multiple)
{
    uint32_t context = dgi.read_uint32();
    doid_t queried_parent = dgi.read_doid();

    m_log->trace() << "Handling object query with parent '" << queried_parent << "'"
                   << ".  My id is " << m_do_id << " and my parent is " << m_parent_id
                   << ".\n";

    uint16_t count = 1;
    if(multiple) {
        count = dgi.read_uint16();
    }

    if(queried_parent == m_parent_id) {
        // Query was relayed from parent! See if we match any of the zones
        // and if so, reply:
        for(uint16_t i = 0; i < count; ++i) {
            if(do_id && (dgi.read_doid() == m_do_id) ||
               !do_id && (dgi.read_zone() == m_zone_id)) {
                // The parent forwarding this request down to us may or may
                // not yet know about our presence (and therefore have us
                // included in the count that it sent to the interested
                // peer). If we are included in this count, we reply with a
                // normal interest entry. If not, we reply with a standard
                // location entry and allow the interested peer to resolve
                // the difference itself.
                if(m_parent_synchronized) {
                    send_interest_entry(sender, context);
                } else {
                    send_location_entry(unordered_set<channel_t>{sender});
                }
                break;
            }
        }
    } else if(queried_parent == m_do_id) {
        doid_t child_count = 0;

        // Start datagram to relay to children
        DatagramPtr child_dg = Datagram::create(parent_to_children(m_do_id), sender, msg_type);
        child_dg->add_uint32(context);
        child_dg->add_doid(queried_parent);
        child_dg->add_uint16(count);

        if(do_id) {
            // Get all doids requested:
            for(int i = 0; i < count; ++i) {
                child_count++;
                child_dg->add_doid(dgi.read_doid());
            }
        } else {
            // Get all zones requested:
            for(int i = 0; i < count; ++i) {
                zone_t zone = dgi.read_zone();
                child_count += m_zone_objects[zone].size();
                child_dg->add_zone(zone);
            }
        }

        // Reply to requestor with count of objects expected
        DatagramPtr count_dg = Datagram::create(sender, m_do_id, STATESERVER_OBJECT_GET_COUNT_RESP);
        count_dg->add_uint32(context);
        count_dg->add_doid(child_count);
        route_datagram(count_dg);

        // Bounce the message down to all children and have them decide
        // whether or not to reply.
        // TODO: Is this really that efficient?
        if(child_count > 0) {
            route_datagram(child_dg);
        }
    }
}

void DistributedObject::handle_datagram(DatagramHandle, DatagramIterator &dgi)
{
    channel_t sender = dgi.read_channel();
    uint16_t msgtype = dgi.read_uint16();
    switch(msgtype) {
    case STATESERVER_DELETE_AI_OBJECTS: {
        if(m_ai_channel != dgi.read_channel()) {
            m_log->warning() << " received reset for wrong AI channel.\n";
            break; // Not my AI!
        }

        // Log the AI being deleted.
        m_log->warning() << "AI channel " << m_ai_channel << " is now being deleted.\n";

        // Begin the deletion process.
        begin_delete(true);

        break;
    }
    case STATESERVER_OBJECT_DELETE_RAM: {
        if(m_do_id != dgi.read_doid()) {
            if(m_deletion_process) {
                // We've received this as a response from a child which is now deleted.
                // Increment the amount of children that are deleted.
                ++m_deleted;
                if(m_deleted == m_deletion_count) {
                    // All children are deleted. We can delete ourselves now.
                    finish_delete(true);
                }
            }

            // Nothing else to do here.
            break;
        }

        // If we received more data, assume we received a boolean.
        if(dgi.get_remaining() > 0) {
            // Begin the deletion process.
            begin_delete(dgi.read_bool());
        } else {
            // Begin the deletion process.
            begin_delete();
        }

        break;
    }
    case STATESERVER_OBJECT_DELETE_CHILDREN: {
        begin_delete(dgi.read_bool());
        break;
    }
    case STATESERVER_OBJECT_SET_FIELD: {
        if(m_do_id != dgi.read_doid()) {
            break; // Not meant for me!
        }
        handle_one_update(dgi, sender);

        break;
    }
    case STATESERVER_OBJECT_SET_FIELDS: {
        if(m_do_id != dgi.read_doid()) {
            break; // Not meant for me!
        }
        uint16_t field_count = dgi.read_uint16();
        for(int16_t i = 0; i < field_count; ++i) {
            if(!handle_one_update(dgi, sender)) {
                break;
            }
        }
        break;
    }
    case STATESERVER_OBJECT_CHANGING_AI: {
        doid_t r_parent_id = dgi.read_doid();
        channel_t new_channel = dgi.read_channel();
        m_log->trace() << "Received ChangingAI notification from " << r_parent_id << ".\n";
        if(r_parent_id != m_parent_id) {
            m_log->warning() << "Received AI channel from " << r_parent_id
                             << " but my parent_id is " << m_parent_id << ".\n";
            break;
        }
        if(m_ai_explicitly_set) {
            break;
        }
        handle_ai_change(new_channel, sender, false);

        break;
    }
    case STATESERVER_OBJECT_SET_AI: {
        channel_t new_channel = dgi.read_channel();
        m_log->trace() << "Updating AI to " << new_channel << ".\n";
        handle_ai_change(new_channel, sender, true);

        break;
    }
    case STATESERVER_OBJECT_GET_AI: {
        m_log->trace() << "Received AI query from " << sender << ".\n";
        DatagramPtr dg = Datagram::create(sender, m_do_id, STATESERVER_OBJECT_GET_AI_RESP);
        dg->add_uint32(dgi.read_uint32()); // Get context
        dg->add_doid(m_do_id);
        dg->add_channel(m_ai_channel);
        route_datagram(dg);

        break;
    }
    case STATESERVER_OBJECT_GET_AI_RESP: {
        dgi.read_uint32(); // Discard context
        doid_t r_parent_id = dgi.read_doid();
        m_log->trace() << "Received AI query response from " << r_parent_id << ".\n";
        if(r_parent_id != m_parent_id) {
            m_log->warning() << "Received AI channel from " << r_parent_id
                             << " but my parent_id is " << m_parent_id << ".\n";
            break;
        }

        channel_t new_ai = dgi.read_channel();
        if(m_ai_explicitly_set) {
            break;
        }
        handle_ai_change(new_ai, sender, false);

        break;
    }
    case STATESERVER_OBJECT_CHANGING_LOCATION: {
        doid_t child_id = dgi.read_doid();
        doid_t new_parent = dgi.read_doid();
        zone_t new_zone = dgi.read_zone();
        doid_t r_do_id = dgi.read_doid();
        zone_t r_zone = dgi.read_zone();
        if(new_parent == m_do_id) {
            if(m_do_id == r_do_id) {
                if(new_zone == r_zone) {
                    break; // No change, so do nothing.
                }

                auto &children = m_zone_objects[r_zone];
                children.erase(child_id);
                if(children.empty()) {
                    m_zone_objects.erase(r_zone);
                }
            }

            m_zone_objects[new_zone].insert(child_id);

            DatagramPtr dg = Datagram::create(child_id, m_do_id, STATESERVER_OBJECT_LOCATION_ACK);
            dg->add_doid(m_do_id);
            dg->add_zone(new_zone);
            route_datagram(dg);
        } else if(r_do_id == m_do_id) {
            auto &children = m_zone_objects[r_zone];
            children.erase(child_id);
            if(children.empty()) {
                m_zone_objects.erase(r_zone);
            }
        } else {
            m_log->warning() << "Received changing location from " << child_id
                             << " for " << r_do_id << ", but my id is " << m_do_id << ".\n";
        }

        break;
    }
    case STATESERVER_OBJECT_LOCATION_ACK: {
        doid_t r_parent_id = dgi.read_doid();
        zone_t r_zone_id = dgi.read_zone();
        if(r_parent_id != m_parent_id) {
            m_log->trace() << "Received location acknowledgement from " << r_parent_id
                           << " but my parent_id is " << m_parent_id << ".\n";
        } else if(r_zone_id != m_zone_id) {
            m_log->trace() << "Received location acknowledgement for zone " << r_zone_id
                           << " but my zone_id is " << m_zone_id << ".\n";
        } else {
            m_log->trace() << "Parent acknowledged my location change.\n";
            m_parent_synchronized = true;
        }
        break;
    }
    case STATESERVER_OBJECT_SET_LOCATION: {
        doid_t new_parent = dgi.read_doid();
        zone_t new_zone = dgi.read_zone();
        m_log->trace() << "Updating location to Parent: " << new_parent
                       << ", Zone: " << new_zone << ".\n";

        handle_location_change(new_parent, new_zone, sender);

        break;
    }
    case STATESERVER_OBJECT_GET_LOCATION: {
        uint32_t context = dgi.read_uint32();

        DatagramPtr dg = Datagram::create(sender, m_do_id, STATESERVER_OBJECT_GET_LOCATION_RESP);
        dg->add_uint32(context);
        dg->add_doid(m_do_id);
        dg->add_location(m_parent_id, m_zone_id);
        route_datagram(dg);

        break;
    }
    case STATESERVER_OBJECT_GET_LOCATION_RESP: {
        // This case occurs immediately after object creation.
        // A parent expects to receive a location_resp from each
        // of its pre-existing children.

        if(dgi.read_uint32() != STATESERVER_CONTEXT_WAKE_CHILDREN) {
            m_log->warning() << "Received unexpected GetLocationResp from "
                             << dgi.read_uint32() << ".\n";
            break;
        }

        // Get DOID of our child
        doid_t doid = dgi.read_doid();

        // Get location
        doid_t r_parent = dgi.read_doid();
        zone_t r_zone = dgi.read_zone();

        // Update the child count
        if(r_parent == m_do_id) {
            m_zone_objects[r_zone].insert(doid);
        }
        break;
    }
    case STATESERVER_OBJECT_GET_ALL: {
        uint32_t context = dgi.read_uint32();
        if(dgi.read_doid() != m_do_id) {
            return;    // Not meant for this object!
        }
        DatagramPtr dg = Datagram::create(sender, m_do_id, STATESERVER_OBJECT_GET_ALL_RESP);
        dg->add_uint32(context);
        append_required_data(dg);
        append_other_data(dg);
        route_datagram(dg);

        break;
    }
    case STATESERVER_OBJECT_GET_FIELD: {
        uint32_t context = dgi.read_uint32();
        if(dgi.read_doid() != m_do_id) {
            return;    // Not meant for this object!
        }
        uint16_t field_id = dgi.read_uint16();

        DatagramPtr raw_field = Datagram::create();
        bool success = handle_one_get(raw_field, field_id);

        DatagramPtr dg = Datagram::create(sender, m_do_id, STATESERVER_OBJECT_GET_FIELD_RESP);
        dg->add_uint32(context);
        dg->add_bool(success);
        if(success) {
            dg->add_data(raw_field);
        }
        route_datagram(dg);

        break;
    }
    case STATESERVER_OBJECT_GET_FIELDS: {
        uint32_t context = dgi.read_uint32();
        if(dgi.read_doid() != m_do_id) {
            return;    // Not meant for this object!
        }
        uint16_t field_count = dgi.read_uint16();

        // Read our requested fields into a sorted set
        set<uint16_t> requested_fields;
        for(int i = 0; i < field_count; ++i) {
            uint16_t field_id = dgi.read_uint16();
            if(!requested_fields.insert(field_id).second) {
                DCField* field = m_dclass->get_field_by_index(field_id);
                if(field != nullptr) {
                    // If it is null, handle_one_get will produce a warning for us later
                    m_log->warning() << "Received duplicate field '"
                                     << field->get_name() << "' in get_fields.\n";
                }
            }
        }

        // Try to get the values for all the fields
        bool success = true;
        uint16_t fields_found = 0;
        DatagramPtr raw_fields = Datagram::create();
        for(auto it = requested_fields.begin(); it != requested_fields.end(); ++it) {
            uint16_t field_id = *it;
            uint16_t length = raw_fields->size();
            if(!handle_one_get(raw_fields, field_id, true)) {
                success = false;
                break;
            }
            if(raw_fields->size() > length) {
                fields_found++;
            }
        }

        // Send get fields response
        DatagramPtr dg = Datagram::create(sender, m_do_id, STATESERVER_OBJECT_GET_FIELDS_RESP);
        dg->add_uint32(context);
        dg->add_bool(success);
        if(success) {
            dg->add_uint16(fields_found);
            dg->add_data(raw_fields);
        }
        route_datagram(dg);

        break;
    }
    case STATESERVER_OBJECT_SET_OWNER: {
        channel_t new_owner = dgi.read_channel();
        m_log->trace() << "Updating owner to " << new_owner << "...\n";
        if(new_owner == m_owner_channel) {
            m_log->trace() << "... owner is the same, do nothing.\n";
            return;
        }

        if(m_owner_channel) {
            m_log->trace() << "... broadcasting changing owner...\n";
            DatagramPtr dg = Datagram::create(m_owner_channel, sender, STATESERVER_OBJECT_CHANGING_OWNER);
            dg->add_doid(m_do_id);
            dg->add_channel(new_owner);
            dg->add_channel(m_owner_channel);
            route_datagram(dg);
        }

        m_owner_channel = new_owner;

        if(new_owner) {
            m_log->trace() << "... sending owner entry...\n";
            send_owner_entry(new_owner);
        }

        m_log->trace() << "... updated owner.\n";
        break;
    }
    case STATESERVER_OBJECT_GET_CHILDREN: {
        handle_object_query(dgi, STATESERVER_OBJECT_GET_CHILDREN, sender, true, true);

        break;
    }
    case STATESERVER_OBJECT_GET_ZONE_OBJECTS:
    case STATESERVER_OBJECT_GET_ZONES_OBJECTS: {
        handle_object_query(dgi, STATESERVER_OBJECT_GET_ZONES_OBJECTS, sender,
                            false, msgtype == STATESERVER_OBJECT_GET_ZONES_OBJECTS);

        break;
    }
    case STATESERVER_OBJECT_GET_CHILD_COUNT: {
        uint32_t context = dgi.read_uint32();

        // Get the child count.
        doid_t child_count = 0;
        for(auto kv : m_zone_objects) {
            child_count += m_zone_objects[kv.first].size();
        }

        // Send the response with the number of children.
        DatagramPtr dg = Datagram::create(sender, m_do_id, STATESERVER_OBJECT_GET_CHILD_COUNT_RESP);
        dg->add_doid(child_count);
        route_datagram(dg);
        break;
    }

    // zones in OTP don't have meaning to the cluster itself
    // as such, there is no table of zones to query in the network
    // instead, a zone is said to be active if it has at least one object in it
    // to get the active zones, get the keys from m_zone_objects and dump them into a std::set<zone_t>
    // using an std::set ensures that no duplicate zones are sent
    // TODO: evaluate efficiency on large games with many DistributedObjects

    case STATESERVER_GET_ACTIVE_ZONES: {
        uint32_t context = dgi.read_uint32();

        std::unordered_set<zone_t> keys;

        for(auto kv : m_zone_objects) {
            keys.insert(kv.first);
        }

        DatagramPtr dg = Datagram::create(sender, m_do_id, STATESERVER_GET_ACTIVE_ZONES_RESP);

        dg->add_uint32(context);
        dg->add_uint16(keys.size());

        std::unordered_set<zone_t>::iterator it;
        for(it = keys.begin(); it != keys.end(); ++it) {
            dg->add_zone(*it);
        }

        route_datagram(dg);
        break;
    }
    case STATESERVER_BOUNCE_MESSAGE: {
        vector<string> messages;

        // Iterate over sent messages
        while(dgi.get_remaining() > 0) {
            messages.push_back(dgi.read_string());
        }

        // Dispatch each message
        for(auto message : messages) {
            DatagramPtr dg = Datagram::create(message);
            DatagramIterator dgi(dg);
            dgi.seek(1 + sizeof(channel_t)); // Ignore server header junk
            handle_datagram(dg, dgi);
        }

        break;
    }
    default:
        if(msgtype < STATESERVER_MSGTYPE_MIN || msgtype > STATESERVER_MSGTYPE_MAX) {
            m_log->warning() << "Received unknown message of type " << msgtype << ".\n";
        } else {
            m_log->trace() << "Ignoring stateserver message of type " << msgtype << ".\n";
        }
    }
}
