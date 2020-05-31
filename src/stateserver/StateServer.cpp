#include "core/types.h"
#include "core/global.h"
#include "core/msgtypes.h"
#include "config/constraints.h"
#include "dclass/dcClass.h"
#include <exception>
#include <stdexcept>

#include "DistributedObject.h"
#include "StateServer.h"

static RoleConfigGroup stateserver_config("stateserver");

static ConfigVariable<channel_t> control_channel("control", INVALID_CHANNEL, stateserver_config);
static InvalidChannelConstraint control_not_invalid(control_channel);
static ReservedChannelConstraint control_not_reserved(control_channel);

static ConfigGroup generate_config("generate", stateserver_config);
static ConfigVariable<doid_t> min_id("min", INVALID_DO_ID, generate_config);
static ConfigVariable<doid_t> max_id("max", UINT_MAX, generate_config);
static InvalidDoidConstraint min_not_invalid(min_id);
static InvalidDoidConstraint max_not_invalid(max_id);
static ReservedDoidConstraint min_not_reserved(min_id);
static ReservedDoidConstraint max_not_reserved(max_id);

StateServer::StateServer(RoleConfig roleconfig) : Role(roleconfig),
    m_min_id(min_id.get_rval(roleconfig)),
    m_max_id(max_id.get_rval(roleconfig))
{
    ConfigNode generate = stateserver_config.get_child_node(generate_config, roleconfig);
    m_min_id = min_id.get_rval(generate);
    m_max_id = max_id.get_rval(generate);

    // Declare all of our initial variables for object ID allocation:
    doid_t size = m_max_id - m_min_id + 1;
    m_table = (doid_t*)malloc(size * sizeof(doid_t));
    for(doid_t i = 0; i < size; ++i) {
        // Update each element in our allocation table.
        m_table[i] = i + 1;
    }
    m_table[size - 1] = INDEX_END;
    m_next_free = 0;
    m_last_free = size - 1;

    m_channel = control_channel.get_rval(m_roleconfig);
    if(m_channel != INVALID_CHANNEL) {
        subscribe_channel(m_channel);
        subscribe_channel(BCHAN_STATESERVERS);

        std::stringstream name;
        name << "StateServer(" << m_channel << ")";
        m_log = std::unique_ptr<LogCategory>(new LogCategory("stateserver", name.str()));
        set_con_name(name.str());
    }
}

doid_t StateServer::get_next_id()
{
    if (m_next_free == INDEX_END) {
        // Yikes! All IDs are allocated.
        m_log->error() << "All available object IDs have been allocated!" << endl;
        return 0;
    }

    // Retrieve the next free index.
    doid_t index = m_next_free;
    if(index == INDEX_ALLOCATED) {
        // This index was already allocated!
        m_log->error() << "Attempted to allocate an object ID which is already allocated!" << endl;
        return 0;
    }

    // Update the index and the next freed index.
    m_next_free = m_table[m_next_free];
    m_table[index] = INDEX_ALLOCATED;

    // Get the next ID.
    doid_t id = index + m_min_id;

    return id;
}

void StateServer::free_id(doid_t id)
{
    if(id < m_min_id || id > m_max_id) {
        // This ID is out of range.
        m_log->error() << "Attempted to free an out-of-range ID!" << endl;
        return;
    }

    // Retrieve the index of this ID.
    doid_t index = id - m_min_id;
    if(m_table[index] != INDEX_ALLOCATED) {
        // This ID has not been allocated already.
        m_log->error() << "Attempted to free a non-allocated ID!" << endl;
        return;
    }

    if(m_next_free != INDEX_END) {
        if(m_table[m_last_free] != INDEX_END) {
            // The last free ID is reserved...
            m_log->error() << "The last freed ID is reserved!" << endl;
        }

        // Update the last freed index in our table.
        m_table[m_last_free] = index;
    }

    // Put this index at the end of the chain.
    m_table[index] = INDEX_END;
    m_last_free = index;

    // If the next free index is the end, this will be our next index.
    if(m_next_free == INDEX_END) {
        m_next_free = index;
    }
}

void StateServer::handle_generate(DatagramIterator &dgi, channel_t sender, bool has_other)
{
    uint32_t context = dgi.read_uint32();
    doid_t parent_id = dgi.read_doid();
    zone_t zone_id = dgi.read_zone();
    uint16_t dc_id = dgi.read_uint16();

    // Allocate a new object ID.
    doid_t do_id = get_next_id();

    // Make sure the object id is unique. This case should never occur.
    if(m_objs.find(do_id) != m_objs.end()) {
        m_log->error() << "Received generate for already existing object ID " << do_id << std::endl;
        return;
    }

    // Make sure we aren't creating an object with a DOID of zero.
    if(do_id == 0) {
        m_log->error() << "Failed to allocate object ID to newly requested object!\n";
        return;
    }

    // Make sure the class exists in the file.
    DCClass *dc_class = g_dcf->get_class(dc_id);
    if(!dc_class) {
        m_log->error() << "Received create for unknown dclass with class ID '" << dc_id << "'\n";
        return;
    }

    // If the object doesn't have a parent or zone, then it must be the root object.
    if(parent_id == 0 && zone_id == 0) {
        // Make sure we don't already have a root object...
        if(m_root_object) {
            m_log->error() << "Attempted to create a root object when one already exists!\n";
            return;
        }

        // Set our root object.
        m_log->info() << "Root object registered to object ID " << do_id << ".\n";
        m_root_object = do_id;
    }

    // If we don't have a root object yet, the object couldn't possibly be valid in the tree.
    if(m_root_object == 0) {
        m_log->error() << "Attempted to create an object when there isn't a root object yet!\n";
        return;
    }

    // Check if the parent is the root object.
    if(m_root_object != 0 && parent_id == m_root_object) {
        m_log->info() << "New AI server with object ID " << do_id << " connected.\n";
    }

    // Create the object
    DistributedObject *obj;
    try {
        obj = new DistributedObject(this, do_id, parent_id, zone_id, dc_class, dgi, has_other);
    } catch(const DatagramIteratorEOF&) {
        m_log->error() << "Received truncated generate for "
                       << dc_class->get_name() << "(" << do_id << ")" << std::endl;
        return;
    }

    // Store the object's information.
    m_objs[do_id] = obj;
    m_dcids[do_id] = dc_id;

    // Send the generate response back to the sender.
    DatagramPtr dg = Datagram::create(sender, m_channel,
        has_other ? STATESERVER_CREATE_OBJECT_WITH_REQUIRED_OTHER_RESP : STATESERVER_CREATE_OBJECT_WITH_REQUIRED_RESP);
    dg->add_uint32(context);
    dg->add_doid(do_id);
    dg->add_doid(parent_id);
    dg->add_zone(zone_id);
    dg->add_uint16(dc_id);
    route_datagram(dg);
}

void StateServer::handle_delete_ai(DatagramIterator& dgi, channel_t sender)
{
    channel_t ai_channel = dgi.read_channel();
    std::unordered_set<channel_t> targets;
    for(const auto& it : m_objs) {
        if(it.second && it.second->m_ai_channel == ai_channel && it.second->m_ai_explicitly_set) {
            targets.insert(it.second->m_do_id);
        }
    }

    if(targets.size()) {
        DatagramPtr dg = Datagram::create(targets, sender, STATESERVER_DELETE_AI_OBJECTS);
        dg->add_channel(ai_channel);
        route_datagram(dg);
    }
}

void StateServer::handle_datagram(DatagramHandle, DatagramIterator &dgi)
{
    channel_t sender = dgi.read_channel();
    uint16_t msgtype = dgi.read_uint16();
    switch(msgtype) {
    case STATESERVER_CREATE_OBJECT_WITH_REQUIRED: {
        handle_generate(dgi, sender, false);
        break;
    }
    case STATESERVER_CREATE_OBJECT_WITH_REQUIRED_OTHER: {
        handle_generate(dgi, sender, true);
        break;
    }
    case STATESERVER_DELETE_AI_OBJECTS: {
        handle_delete_ai(dgi, sender);
        break;
    }
    default:
        m_log->warning() << "Received unknown message: msgtype=" << msgtype << std::endl;
    }
}

RoleFactoryItem<StateServer> ss_fact("stateserver");
