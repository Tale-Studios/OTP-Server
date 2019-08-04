#include "Client.h"
#include "core/global.h"
#include "core/msgtypes.h"
#include "clientagent/ClientMessages.h"
#include "clientagent/ClientAgent.h"
using namespace std;

Client::Client(ConfigNode, ClientAgent* client_agent) :
    m_client_agent(client_agent)
{
    assert(std::this_thread::get_id() == g_main_thread_id);

    m_channel = m_client_agent->m_ct.alloc_channel();
    if(!m_channel) {
        m_log = m_client_agent->log();
        send_disconnect(CLIENT_DISCONNECT_GENERIC, "Client capacity reached");
        return;
    }

    m_allocated_channel = m_channel;

    stringstream name;
    name << "Client (" << m_allocated_channel << ")";

    m_log_owner = std::unique_ptr<LogCategory>(new LogCategory("client", name.str()));

    m_log = m_log_owner.get();

    set_con_name(name.str());

    subscribe_channel(m_channel);
    subscribe_channel(BCHAN_CLIENTS);
}

Client::~Client()
{
    // We need to be holding our own lock, just in case another thread is busy
    // doing some last-microsecond cleanup.
    lock_guard<recursive_mutex> lock(m_client_lock);

    assert(!m_pending_interests.size());
}

void Client::annihilate()
{
    lock_guard<recursive_mutex> lock(m_client_lock);
    if(is_terminated()) {
        return;
    }

    // Unsubscribe from all channels first so the DELETE messages aren't sent back to us.
    unsubscribe_all();
    m_client_agent->m_ct.free_channel(m_allocated_channel);

    // Delete all owned objects
    while(m_owned_objects.size() > 0) {
        auto owned_object = *m_owned_objects.begin();
        doid_t do_id = owned_object.first;
        m_owned_objects.erase(do_id);
        m_log->debug() << "Client exited, deleting owned object with id " << do_id << ".\n";
        DatagramPtr dg = Datagram::create(do_id, m_channel, STATESERVER_OBJECT_DELETE_RAM);
        dg->add_doid(do_id);
        dg->add_bool(0);
        route_datagram(dg);
    }

    // Note that finish() normally results in the InterestOperation deleting
    // itself from m_pending_interests, so we have to be VERY careful with this
    // for loop. Using (it++) ensures that 'it' is advanced BEFORE finish() is
    // called; doing so after means the iterator is invalid!
    for(auto it = m_pending_interests.begin(); it != m_pending_interests.end();) {
        (it++)->second->finish();
    }

    // Tell the MD this client is gone
    terminate();
}

// log_event sends an event to the EventLogger
void Client::log_event(LoggedEvent &event)
{
    stringstream ss;
    ss << "Client:" << m_allocated_channel;
    event.add("sender", ss.str());

    g_eventsender.send(event);
}

void Client::generate_timeout(TimeoutSetCallback timeout_set_callback)
{
    {
        std::lock_guard<std::mutex> lock(m_timeout_mutex);
        m_pending_timeouts.push(timeout_set_callback);
    }

    if(std::this_thread::get_id() != g_main_thread_id) {
        TaskQueue::singleton.enqueue_task([self = this]() {
            self->generate_timeouts();
        });
    } else {
        generate_timeouts();
    }
}

void Client::generate_timeouts()
{
    assert(std::this_thread::get_id() == g_main_thread_id);

    if(m_is_generating_timeouts) {
        // Already in the middle of another generate_timeouts invocation.
        return;
    }

    m_is_generating_timeouts = true;

    {
        std::lock_guard<std::mutex> lock(m_timeout_mutex);

        while(!m_pending_timeouts.empty()) {
            TimeoutSetCallback timeout_set_callback = m_pending_timeouts.front();
            m_pending_timeouts.pop();
            Timeout* timeout = new Timeout();
            timeout_set_callback(timeout);
        }
    }

    m_is_generating_timeouts = false;
}

// lookup_object returns the class of the object with a do_id.
// If that object is not visible to the client, nullptr will be returned instead.
DCClass *Client::lookup_object(doid_t do_id)
{
    // First see if it's an UberDOG:
    if(g_uberdogs.find(do_id) != g_uberdogs.end()) {
        return g_uberdogs[do_id].dcc;
    }

    // Let's check if this is an object the client owns:
    if(m_owned_objects.find(do_id) != m_owned_objects.end()) {
        return m_owned_objects[do_id].dcc;
    }

    // Next, check if this is an object that this client can see:
    else if(m_seen_objects.find(do_id) != m_seen_objects.end()) {
        if(m_visible_objects.find(do_id) != m_visible_objects.end()) {
            return m_visible_objects[do_id].dcc;
        }
    }

    // Lastly, we can check if this is a declared object:
    else if(m_declared_objects.find(do_id) != m_declared_objects.end()) {
        return m_declared_objects[do_id].dcc;
    }

    // We're at the end of our rope; we have no clue what this object is.
    return nullptr;
}

// lookup_interests returns a list of all the interests that a parent-zone pair is visible to.
vector<Interest> Client::lookup_interests(doid_t parent_id, zone_t zone_id)
{
    vector<Interest> interests;
    for(const auto& it : m_interests) {
        if(parent_id == it.second.parent && (it.second.zones.find(zone_id) != it.second.zones.end())) {
            interests.push_back(it.second);
        }
    }
    return interests;
}

// build_interest will build an interest from a datagram. It is expected that the datagram
// iterator is positioned such that next item to be read is the interest_id.
void Client::build_interest(DatagramIterator &dgi, bool multiple, Interest &out)
{
    uint16_t interest_id = dgi.read_uint16();
    doid_t parent = dgi.read_doid();

    out.id = interest_id;
    out.parent = parent;

    uint16_t count = 1;
    if(multiple) {
        count = dgi.read_uint16();
    }

    // TODO: We shouldn't have to do this ourselves, figure out where else we're doing
    //       something wrong.
    out.zones.rehash((unsigned int)(ceil(count / out.zones.max_load_factor())));

    for(uint16_t x{}; x < count; ++x) {
        zone_t zone = dgi.read_zone();
        out.zones.insert(out.zones.end(), zone);
    }
}

// add_interest will start a new interest operation and retrieve all the objects an interest
// from the server, subscribing to each zone in the interest.  If the interest already
// exists, the interest will be updated with the new zones passed in by the argument.
void Client::add_interest(Interest &i, uint32_t context, channel_t caller)
{
    unordered_set<zone_t> new_zones;

    for(const auto& it : i.zones) {
        if(lookup_interests(i.parent, it).empty()) {
            new_zones.insert(it);
        }
    }

    if(m_interests.find(i.id) != m_interests.end()) {
        // This is an already-open interest that is actually being altered.
        // Therefore, we need to delete the objects that the client can see
        // through this interest only.

        Interest previous_interest = m_interests[i.id];
        unordered_set<zone_t> killed_zones;

        for(const auto& it : previous_interest.zones) {
            if(lookup_interests(previous_interest.parent, it).size() > 1) {
                // An interest other than the altered one can see this parent/zone,
                // so we don't care about it.
                continue;
            }

            // If we've gotten here: parent,*it is unique, so if the new interest
            // doesn't cover it, we add it to the killed zones.
            if(i.parent != previous_interest.parent || i.zones.find(it) == i.zones.end()) {
                killed_zones.insert(it);
            }
        }

        // Now that we know what zones to kill, let's get to it:
        close_zones(previous_interest.parent, killed_zones);
    }
    m_interests[i.id] = i;

    if(new_zones.empty()) {
        // We aren't requesting any new zones with this operation, so don't
        // bother firing off a State Server request. Instead, let the caller
        // know we're already done:
        if(caller == m_channel)
        {
            handle_interest_done(i.id, context);
        }
        else
        {
            notify_interest_done(i.id, caller);
        }

        return;
    }

    uint32_t request_context = m_next_context++;

    InterestOperation *iop = new InterestOperation(this,
            i.id, context, request_context, i.parent, new_zones, caller);
    m_pending_interests.emplace(request_context, iop);

    DatagramPtr resp = Datagram::create();
    resp->add_server_header(i.parent, m_channel, STATESERVER_OBJECT_GET_ZONES_OBJECTS);
    resp->add_uint32(request_context);
    resp->add_doid(i.parent);
    resp->add_uint16(new_zones.size());
    for(const auto& it : new_zones) {
        resp->add_zone(it);
        subscribe_channel(location_as_channel(i.parent, it));
    }
    route_datagram(resp);
}

// remove_interest find each zone an interest which is not part of another interest and
// passes it to close_zones() to be removed from the client's visibility.
void Client::remove_interest(Interest &i, uint32_t context, channel_t caller)
{
    unordered_set<zone_t> killed_zones;

    for(const auto& it : i.zones) {
        if(lookup_interests(i.parent, it).size() == 1) {
            // We're the only interest who can see this zone, so let's kill it.
            killed_zones.insert(it);
        }
    }

    // Now that we know what zones to kill, let's get to it:
    close_zones(i.parent, killed_zones);

    notify_interest_done(i.id, caller);
    handle_interest_done(i.id, context);

    m_interests.erase(i.id);
}

// cloze_zones removes objects visible through the zones from the client and unsubscribes
// from the associated location channels for those objects.
void Client::close_zones(doid_t parent, const unordered_set<zone_t> &killed_zones)
{
    // Kill off all objects that are in the matched parent/zones:

    vector<doid_t> to_remove;
    for(const auto& it : m_visible_objects) {
        const VisibleObject& visible_object = it.second;
        if(visible_object.parent != parent) {
            // Object does not belong to the parent in question; ignore.
            continue;
        }

        if(killed_zones.find(visible_object.zone) != killed_zones.end()) {
            handle_remove_object(visible_object.id);
            m_seen_objects.erase(visible_object.id);
            m_historical_objects.insert(visible_object.id);
            to_remove.push_back(visible_object.id);
        }
    }

    for(const auto& it : to_remove) {
        m_visible_objects.erase(it);
    }

    // Close all of the channels:
    for(const auto& it : killed_zones) {
        unsubscribe_channel(location_as_channel(parent, it));
    }
}

// is_historical_object returns true if the object was once visible to the client, but has
// since been deleted.  The return is still true even if the object has become visible again.
bool Client::is_historical_object(doid_t do_id)
{
    return m_historical_objects.find(do_id) != m_historical_objects.end();
}

// send_disconnect must close any connections with a connected client;
// the given reason and error should be forwarded to the client.
// Client::send_disconnect can be called by subclasses to handle logging the event.
void Client::send_disconnect(uint16_t reason, const string &error_string, bool security)
{
    (security ? m_log->security() : m_log->debug())
            << "Ejecting client (" << reason << "): "
            << error_string << endl;

    LoggedEvent event(security ? "client-ejected-security" : "client-ejected");
    event.add("reason_code", to_string((unsigned long long)reason));
    event.add("reason_msg", error_string);
    log_event(event);
}

// handle_datagram is the handler for datagrams received from the OTP cluster
void Client::handle_datagram(DatagramHandle in_dg, DatagramIterator &dgi)
{
    lock_guard<recursive_mutex> lock(m_client_lock);

    // If we're terminated, or we just aren't accepting messages,
    // just return.
    if(is_terminated() || !m_accept_messages) {
        return;
    }

    // Grab the sender channel.
    channel_t sender = dgi.read_channel();

    // We don't want to receive messages from ourselves.
    if(sender == m_channel) {
        return;
    }

    // Grab the message type.
    uint16_t msgtype = dgi.read_uint16();

    // Handle each message type we expect.
    switch(msgtype) {
    case CLIENTAGENT_EJECT: {
        uint16_t reason = dgi.read_uint16();
        string error_string = dgi.read_string();
        send_disconnect(reason, error_string);
        return;
    }
    break;
    case CLIENTAGENT_DROP: {
        handle_drop();
        return;
    }
    break;
    case CLIENTAGENT_SET_STATE: {
        m_state = (ClientState)dgi.read_uint16();
    }
    break;
    case CLIENTAGENT_ADD_INTEREST: {
        uint32_t context = m_next_context++;

        Interest i;
        build_interest(dgi, false, i);
        handle_add_interest(i, context);
        add_interest(i, context, sender);
    }
    break;
    case CLIENTAGENT_ADD_INTEREST_MULTIPLE: {
        uint32_t context = m_next_context++;

        Interest i;
        build_interest(dgi, true, i);
        handle_add_interest(i, context);
        add_interest(i, context, sender);
    }
    break;
    case CLIENTAGENT_REMOVE_INTEREST: {
        uint32_t context = m_next_context++;

        uint16_t id = dgi.read_uint16();
        Interest &i = m_interests[id];
        handle_remove_interest(id, context);
        remove_interest(i, context, sender);
    }
    break;
    case CLIENTAGENT_SET_CLIENT_ID: {
        if(m_channel != m_allocated_channel) {
            unsubscribe_channel(m_channel);
        }

        m_channel = dgi.read_channel();
        subscribe_channel(m_channel);
    }
    break;
    case CLIENTAGENT_SEND_DATAGRAM: {
        DatagramPtr forward = Datagram::create();
        forward->add_data(dgi.read_datagram());
        forward_datagram(forward, true);
    }
    break;
    case CLIENTAGENT_OPEN_CHANNEL: {
        subscribe_channel(dgi.read_channel());
    }
    break;
    case CLIENTAGENT_CLOSE_CHANNEL: {
        unsubscribe_channel(dgi.read_channel());
    }
    break;
    case CLIENTAGENT_ADD_POST_REMOVE: {
        add_post_remove(m_allocated_channel, dgi.read_datagram());
    }
    break;
    case CLIENTAGENT_CLEAR_POST_REMOVES: {
        clear_post_removes(m_allocated_channel);
    }
    break;
    case CLIENTAGENT_DECLARE_OBJECT: {
        doid_t do_id = dgi.read_doid();
        uint16_t dc_id = dgi.read_uint16();

        if(m_declared_objects.find(do_id) != m_declared_objects.end()) {
            m_log->warning() << "Received object declaration for previously declared object "
                             << do_id << ".\n";
            return;
        }

        DeclaredObject obj;
        obj.id = do_id;
        obj.dcc = g_dcf->get_class(dc_id);
        m_declared_objects[do_id] = obj;
    }
    break;
    case CLIENTAGENT_UNDECLARE_OBJECT: {
        doid_t do_id = dgi.read_doid();

        if(m_declared_objects.find(do_id) == m_declared_objects.end()) {
            m_log->warning() << "Received undeclare object for unknown object "
                             << do_id << ".\n";
            return;
        }

        m_declared_objects.erase(do_id);
    }
    break;
    case CLIENTAGENT_SET_FIELDS_SENDABLE: {
        doid_t do_id = dgi.read_doid();
        uint16_t field_count = dgi.read_uint16();

        unordered_set<uint16_t> fields;
        for(uint16_t i{}; i < field_count; ++i) {
            fields.insert(dgi.read_uint16());
        }
        m_fields_sendable[do_id] = fields;
    }
    break;
    case CLIENTAGENT_SET_OBJECT_RELOCATABLE: {
        doid_t do_id = dgi.read_doid();

        m_log->trace() << "Received SetRelocatable for object with id " << do_id << ".\n";

        m_objects_relocatable.insert(do_id);
    }
    break;
    case CLIENTAGENT_ADD_SESSION_OBJECT: {
        doid_t do_id = dgi.read_doid();
        if(m_session_objects.find(do_id) != m_session_objects.end()) {
            m_log->warning() << "Received add session object for existing session object "
                             << do_id << ".\n";
            return;
        }

        m_log->debug() << "Added session object with id " << do_id << ".\n";

        m_session_objects.insert(do_id);
    }
    break;
    case CLIENTAGENT_REMOVE_SESSION_OBJECT: {
        doid_t do_id = dgi.read_doid();

        if(m_session_objects.find(do_id) == m_session_objects.end()) {
            m_log->warning() << "Received remove session object for non-session object "
                             << do_id << ".\n";
            return;
        }

        m_log->debug() << "Removed session object with id " << do_id << ".\n";

        m_session_objects.erase(do_id);
    }
    break;
    case CLIENTAGENT_GET_TLVS: {
        DatagramPtr resp = Datagram::create(sender, m_channel, CLIENTAGENT_GET_TLVS_RESP);
        resp->add_uint32(dgi.read_uint32()); // Context
        resp->add_blob(get_tlvs());
        route_datagram(resp);
    }
    break;
    case CLIENTAGENT_GET_NETWORK_ADDRESS: {
        DatagramPtr resp = Datagram::create(sender, m_channel, CLIENTAGENT_GET_NETWORK_ADDRESS_RESP);
        resp->add_uint32(dgi.read_uint32()); // Context
        resp->add_string(get_remote_address());
        resp->add_uint16(get_remote_port());
        resp->add_string(get_local_address());
        resp->add_uint16(get_local_port());
        route_datagram(resp);
    }
    break;
    case STATESERVER_OBJECT_SET_FIELD: {
        doid_t do_id = dgi.read_doid();
        if(!lookup_object(do_id)) {
            if(try_queue_pending(do_id, in_dg)) {
                return;
            }
            m_log->warning() << "Received server-side field update for unknown object "
                             << do_id << ".\n";
            return;
        }
        if(sender != m_channel) {
            uint16_t field_id = dgi.read_uint16();
            handle_set_field(do_id, field_id, dgi);
        }
    }
    break;
    case STATESERVER_OBJECT_SET_FIELDS: {
        doid_t do_id = dgi.read_doid();
        if(!lookup_object(do_id)) {
            if(try_queue_pending(do_id, in_dg)) {
                return;
            }
            m_log->warning() << "Received server-side multi-field update for unknown object "
                             << do_id << ".\n";
            return;
        }
        if(sender != m_channel) {
            uint16_t num_fields = dgi.read_uint16();
            handle_set_fields(do_id, num_fields, dgi);
        }
    }
    break;
    case STATESERVER_OBJECT_DELETE_RAM: {
        doid_t do_id = dgi.read_doid();
        bool ai_deletion = dgi.read_bool();

        m_log->trace() << "Received DeleteRam for object with id " << do_id << "\n.";

        if(ai_deletion) {
            // If the AI server we're on has crashed, then we need to fall over and die.
            m_accept_messages = false;
            stringstream ss;
            ss << "The AI server has unexpectedly disconnected.";
            send_disconnect(CLIENT_DISCONNECT_AI_DISCONNECT, ss.str());
            return;
        }

        if(!lookup_object(do_id)) {
            if(try_queue_pending(do_id, in_dg)) {
                return;
            }
            m_log->warning() << "Received server-side object delete for unknown object "
                             << do_id << ".\n";
            return;
        }

        if(m_session_objects.find(do_id) != m_session_objects.end()) {
            // We have to erase the object from our session_objects here, because
            // the object has already been deleted and we don't want it to be deleted
            // again in the client's destructor.
            m_session_objects.erase(do_id);

            // Eject the client since its session object has been deleted.
            stringstream ss;
            ss << "The session object with id " << do_id << " has been unexpectedly deleted.";
            send_disconnect(CLIENT_DISCONNECT_SESSION_OBJECT_DELETED, ss.str());
            return;
        }

        if(m_seen_objects.find(do_id) != m_seen_objects.end()) {
            handle_remove_object(do_id);
            m_seen_objects.erase(do_id);
        }

        if(m_owned_objects.find(do_id) != m_owned_objects.end()) {
            handle_remove_ownership(do_id);
            m_owned_objects.erase(do_id);
        }

        m_historical_objects.insert(do_id);
        m_visible_objects.erase(do_id);
    }
    break;
    case STATESERVER_OBJECT_ENTER_OWNER_WITH_REQUIRED_OTHER: {
        doid_t do_id = dgi.read_doid();
        doid_t parent = dgi.read_doid();
        zone_t zone = dgi.read_zone();
        uint16_t dc_id = dgi.read_uint16();

        if (m_owned_objects.find(do_id) == m_owned_objects.end())
        {
            OwnedObject obj;
            obj.id = do_id;
            obj.parent = parent;
            obj.zone = zone;
            obj.dcc = g_dcf->get_class(dc_id);
            m_owned_objects[do_id] = obj;
        }

        handle_add_ownership(do_id, parent, zone, dc_id, dgi);
    }
    break;
    case STATESERVER_OBJECT_ENTER_LOCATION_WITH_REQUIRED:
    case STATESERVER_OBJECT_ENTER_LOCATION_WITH_REQUIRED_OTHER: {
        doid_t do_id = dgi.read_doid();
        doid_t parent = dgi.read_doid();
        zone_t zone = dgi.read_zone();
        dgi.skip(sizeof(uint16_t)); // dc_id

        bool with_other = (msgtype == STATESERVER_OBJECT_ENTER_LOCATION_WITH_REQUIRED_OTHER);

        // XXX HACK
        // something is wrong with gcc version 6.5.0 20181026 (Ubuntu 6.5.0-2ubuntu1~16.04)
        size_t i = 0;
        size_t size = m_pending_interests.size();
        for(auto& it : m_pending_interests) {
            if (i++ > size) break;
            InterestOperation *interest_operation = it.second;
            if(interest_operation->m_parent == parent &&
               interest_operation->m_zones.find(zone) != interest_operation->m_zones.end()) {

                // Add the DoId to m_pending_objects.
                m_pending_objects.emplace(do_id, it.first);

                // Queue the generate in the IOP.
                interest_operation->queue_datagram(in_dg);

                // Nothing else to do for us here.
                return;
            }
        }

        // Seek back to where we started and handle the message.
        dgi.seek_payload();
        dgi.skip(sizeof(channel_t) + sizeof(uint16_t)); // sender + msgtype

        // Generate the object.
        handle_object_entrance(dgi, with_other);
    }
    break;
    case STATESERVER_OBJECT_ENTER_INTEREST_WITH_REQUIRED:
    case STATESERVER_OBJECT_ENTER_INTEREST_WITH_REQUIRED_OTHER: {
        uint32_t request_context = dgi.read_uint32();
        auto it = m_pending_interests.find(request_context);
        if(it == m_pending_interests.end()) {
            m_log->warning() << "Received object entrance into interest with unknown context "
                             << request_context << ".\n";
            return;
        }

        m_pending_objects.emplace(dgi.read_doid(), request_context);
        it->second->queue_expected(in_dg);
        if(it->second->is_ready()) {
            it->second->finish();
        }
        return;
    }
    break;
    case STATESERVER_OBJECT_GET_ZONES_COUNT_RESP: {
        uint32_t context = dgi.read_uint32();
        // using doid_t because <max_objects_in_zones> == <max_total_objects>
        doid_t count = dgi.read_doid();

        auto it = m_pending_interests.find(context);
        if(it == m_pending_interests.end()) {
            m_log->error() << "Received GET_ZONES_COUNT_RESP for unknown context "
                           << context << ".\n";
            return;
        }

        it->second->set_expected(count);
        if(it->second->is_ready()) {
            it->second->finish();
        }
    }
    break;
    case STATESERVER_OBJECT_CHANGING_LOCATION: {
        doid_t do_id = dgi.read_doid();
        if(try_queue_pending(do_id, in_dg)) {
            // We received a generate for this object, and the generate is sitting in a pending iop
            // we'll just store this dg under the m_pending_datagrams queue on the iop
            return;
        }
        doid_t n_parent = dgi.read_doid();
        zone_t n_zone = dgi.read_zone();

        bool disable = true;
        for(const auto& it : m_interests) {
            const Interest& i = it.second;
            if (i.parent == n_parent) {
                for(const auto& it2 : i.zones) {
                    if(it2 == n_zone) {
                        disable = false;
                        break;
                    }
                }
            }
        }

        bool session = m_session_objects.find(do_id) != m_session_objects.end();
        bool visible = m_visible_objects.find(do_id) != m_visible_objects.end();
        bool owned = m_owned_objects.find(do_id) != m_owned_objects.end();

        if(!visible && !owned) {
            // We don't actually *see* this object, we're receiving this
            // message as a fluke.
            return;
        }

        if(visible) {
            m_visible_objects[do_id].parent = n_parent;
            m_visible_objects[do_id].zone = n_zone;
        }

        if(session) {
            if(owned) {
                // This is an owned session object. Therefore:
                // - It should be 'visible' to us regardless of interest
                // - We should keep track of its location
                m_owned_objects[do_id].parent = n_parent;
                m_owned_objects[do_id].zone = n_zone;

                // Fire off CLIENT_OBJECT_LOCATION.
                handle_change_location(do_id, n_parent, n_zone);
            }
            else {
                // This is a session object, but it isn't owned...
                // Session objects must always be owned.
                stringstream ss;
                ss << "The session object with id " << do_id
                   << " is no longer owned.";
                send_disconnect(CLIENT_DISCONNECT_SESSION_OBJECT_DELETED, ss.str());
            }

            return;
        }

        // Disable this object if:
        // 1 - We don't have interest in its location (i.e. disable == true)
        // 2 - It's already visible to us in the first place
        if(disable && visible) {
            handle_remove_object(do_id);
            m_seen_objects.erase(do_id);
            m_historical_objects.insert(do_id);
            m_visible_objects.erase(do_id);
        }
        else {
            handle_change_location(do_id, n_parent, n_zone);
        }
    }
    break;
    case STATESERVER_OBJECT_CHANGING_OWNER: {
        doid_t do_id = dgi.read_doid();
        channel_t n_owner = dgi.read_channel();
        dgi.skip(sizeof(channel_t)); // don't care about the old owner

        if(n_owner == m_channel) {
            // We should already own this object, nothing changes and we
            // might get another enter_owner message.
            return;
        }

        if(m_owned_objects.find(do_id) == m_owned_objects.end()) {
            m_log->error() << "Received ChangingOwner for unowned object with id "
                           << do_id << ".\n";
            return;
        }

        // If it's a session object, disconnect the client.
        // Session objects must stick with their original owner.
        if(m_session_objects.find(do_id) != m_session_objects.end()) {
            stringstream ss;
            ss << "The session object with id " << do_id
               << " has attempted to change ownership.";
            send_disconnect(CLIENT_DISCONNECT_SESSION_OBJECT_DELETED, ss.str());
            return;
        }

        // N.B.: This object visible might be still visible through an interest.
        // We don't have to touch it, just remove the ownership
        handle_remove_ownership(do_id);
        m_owned_objects.erase(do_id);
    }
    break;
    default:
        if(!handle_cluster_datagram(in_dg, dgi, sender, msgtype)) {
            m_log->error() << "Recv'd unknown server msgtype " << msgtype << "\n.";
        }
    }
}

bool Client::try_queue_pending(doid_t do_id, DatagramHandle dg)
{
    auto it = m_pending_objects.find(do_id);
    if(it != m_pending_objects.end()) {
        // the dg should be queued under the appropriate iop
        m_pending_interests.find(it->second)->second->queue_datagram(dg);
        return true;
    }
    // still no idea what do_id was being talked about
    return false;
}

void Client::handle_object_entrance(DatagramIterator &dgi, bool other)
{
    doid_t do_id = dgi.read_doid();
    doid_t parent = dgi.read_doid();
    zone_t zone = dgi.read_zone();
    uint16_t dc_id = dgi.read_uint16();

    // Make sure this object isn't in our pending map.
    m_pending_objects.erase(do_id);

    if(m_seen_objects.find(do_id) != m_seen_objects.end()) {
        return;
    }

    if(m_session_objects.find(do_id) != m_session_objects.end()) {
        return;
    }

    if(m_visible_objects.find(do_id) == m_visible_objects.end()) {
        VisibleObject obj;
        obj.id = do_id;
        obj.dcc = g_dcf->get_class(dc_id);
        obj.parent = parent;
        obj.zone = zone;
        m_visible_objects[do_id] = obj;
    }

    // Insert the object into our seen objects.
    m_seen_objects.insert(do_id);

    handle_add_object(do_id, parent, zone, dc_id, dgi, other);
}

// notify_interest_done send a CLIENTAGENT_DONE_INTEREST_RESP to the
// interest operation's caller, if one has been set.
void Client::notify_interest_done(uint16_t interest_id, channel_t caller)
{
    if(caller == 0) {
        return;
    }

    DatagramPtr resp = Datagram::create(caller, m_channel, CLIENTAGENT_DONE_INTEREST_RESP);
    resp->add_channel(m_channel);
    resp->add_uint16(interest_id);
    route_datagram(resp);
}

/* ========================== *
 *       HELPER CLASSES       *
 * ========================== */
InterestOperation::InterestOperation(
    Client *client,
    uint16_t interest_id, uint32_t client_context, uint32_t request_context,
    doid_t parent, unordered_set<zone_t> zones, channel_t caller) :
    m_client(client),
    m_interest_id(interest_id),
    m_client_context(client_context),
    m_request_context(request_context),
    m_parent(parent), m_zones(zones),
    m_caller(caller)
{
}

void InterestOperation::finish()
{
    // Make a temporary map of DC ID->generate set.
    map<uint16_t, set<DatagramHandle> > dc_map;

    // Gather the generates together by DC ID.
    for(const auto& it : m_pending_generates) {
        DatagramIterator dgi(it);
        dgi.seek_payload();

        // We need to generate objects in order of DC ID,
        // however we don't have the DC ID to iterate by.
        // We'll need to skip through to fetch the DC ID of each generate
        // and compile them into a new temporary map and iterate over that.
        dgi.skip(sizeof(channel_t) + sizeof(uint16_t) + sizeof(uint32_t));
        dgi.skip(sizeof(doid_t) + sizeof(doid_t) + sizeof(zone_t));

        // Get the DC ID and store the generate in the temporary map.
        uint16_t dc_id = dgi.read_uint16();
        dc_map[dc_id].insert(it);
    }

    // Iterate over the temporary map and emit the generates.
    for(auto it = dc_map.begin(); it != dc_map.end(); ++it) {
        for(const auto& dg_it : it->second) {
            DatagramIterator dgi(dg_it);
            dgi.seek_payload();
            dgi.skip(sizeof(channel_t)); // skip sender

            uint16_t msgtype = dgi.read_uint16();
            bool with_other = (msgtype == STATESERVER_OBJECT_ENTER_INTEREST_WITH_REQUIRED_OTHER);

            dgi.skip(sizeof(uint32_t)); // skip request_context
            m_client->handle_object_entrance(dgi, with_other);
        }
    }

    // Distribute the interest done message.
    if(m_caller == m_client->m_channel) {
        m_client->handle_interest_done(m_interest_id, m_client_context);
    }
    else {
        m_client->notify_interest_done(m_interest_id, m_caller);
    }

    // N. B. We need to delete the pending interest before we send queued
    //       datagrams, so that they aren't just re-added to the queue.
    //       Move the queued datagrams to the stack so it is safe to delete the Operation.
    vector<DatagramHandle> dispatch = move(m_pending_datagrams);

    // Delete the pending interest.
    m_client->m_pending_interests.erase(m_request_context);

    // Dispatch other received and queued messages
    for(const auto& it : dispatch) {
        DatagramIterator dgi(it);
        dgi.seek_payload();
        m_client->handle_datagram(it, dgi);
    }

    // Delete the InterestOperation.
    delete this;
}

bool InterestOperation::is_ready()
{
    return m_has_total && m_pending_generates.size() >= m_total;
}

void InterestOperation::set_expected(doid_t total)
{
    if(!m_has_total) {
        m_total = total;
        m_has_total = true;
    }
}

void InterestOperation::queue_expected(DatagramHandle dg)
{
    // One of our interested objects is in line to be generated.
    // Store the datagram in our pending generates.
    m_pending_generates.push_back(dg);
}

void InterestOperation::queue_datagram(DatagramHandle dg)
{
    // One of our pending objects has a datagram that needs to be received
    // when the object is finally generated.
    // Store the datagram in our pending datagrams.
    m_pending_datagrams.push_back(dg);
}
