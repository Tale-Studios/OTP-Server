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

StateServer::StateServer(RoleConfig roleconfig) : Role(roleconfig)
{
    channel_t channel = control_channel.get_rval(m_roleconfig);
    if(channel != INVALID_CHANNEL) {
        subscribe_channel(channel);
        subscribe_channel(BCHAN_STATESERVERS);

        std::stringstream name;
        name << "StateServer(" << channel << ")";
        m_log = std::unique_ptr<LogCategory>(new LogCategory("stateserver", name.str()));
        set_con_name(name.str());
    }
}

void StateServer::handle_generate(DatagramIterator &dgi, bool has_other)
{
    doid_t do_id = dgi.read_doid();
    doid_t parent_id = dgi.read_doid();
    zone_t zone_id = dgi.read_zone();
    uint16_t dc_id = dgi.read_uint16();

    // Make sure the object id is unique
    if(m_objs.find(do_id) != m_objs.end()) {
        m_log->warning() << "Received generate for already-existing object ID=" << do_id << std::endl;
        return;
    }

    // Make sure the class exists in the file
    DCClass *dc_class = g_dcf->get_class(dc_id);
    if(!dc_class) {
        m_log->error() << "Received create for unknown dclass with class id '" << dc_id << "'\n";
        return;
    }

    // Make sure we aren't creating an object with a DOID of zero.
    if(do_id == 0) {
        m_log->error() << "Attempted to create an object with DOID 0.\n";
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
        m_log->info() << "Root object registered to ID " << do_id << ".\n";
        m_root_object = do_id;
    }

    // If we don't have a root object yet, the object couldn't possibly be valid in the tree.
    if(m_root_object == 0) {
        m_log->error() << "Attempted to create an object when there isn't a root object yet!\n";
        return;
    }

    // Check if the parent is the root object.
    if(m_root_object != 0 && parent_id == m_root_object) {
        m_log->info() << "New AI server with ID " << do_id << " connected.\n";
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
    m_objs[do_id] = obj;
    m_dcids[do_id] = dc_id;
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
        handle_generate(dgi, false);
        break;
    }
    case STATESERVER_CREATE_OBJECT_WITH_REQUIRED_OTHER: {
        handle_generate(dgi, true);
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
