#include "Client.h"
#include "ClientFactory.h"
#include "ClientAgent.h"
#include "ToontownClientManager.h"
#include "TalkAssistant.h"
#include "DisneyClientMessages.h"
#include "net/NetworkClient.h"
#include "core/global.h"
#include "core/msgtypes.h"
#include "config/constraints.h"
#include "dclass/dcPacker.h"
#include "dclass/dcClass.h"
#include "dclass/dcField.h"
#include "dna/DNALoader.h"
#include "dna/DNAStorage.h"
#include "dna/DNAVisGroup.h"
#include "util/Timeout.h"

using namespace std;

static ConfigGroup disneyclient_config("libdisney", ca_client_config);

static ConfigVariable<string> game_name_config("game_name", "otp", disneyclient_config);
static ConfigVariable<uint32_t> database_id_config("database_id", 4003, disneyclient_config);
static ConfigVariable<string> database_file_config("database_file", "account-bridge.json", disneyclient_config);
static ConfigVariable<string> name_file_config("name_file", "NameMasterEnglish.txt", disneyclient_config);
static ConfigVariable<string> whitelist_file_config("whitelist_file", "whitelist.dat", disneyclient_config);
static ConfigVariable<string> blacklist_file_config("blacklist_file", "blacklist.dat", disneyclient_config);
static ConfigVariable<vector<string> > dna_files_config("dna_files", vector<string>(), disneyclient_config);
static ConfigVariable<bool> send_hash_to_client("send_hash", true, disneyclient_config);
static ConfigVariable<bool> send_version_to_client("send_version", true, disneyclient_config);

static ConfigVariable<uint64_t> write_buffer_size("write_buffer_size", 256 * 1024,
        disneyclient_config);
static ConfigVariable<unsigned int> write_timeout_ms("write_timeout_ms", 6000, disneyclient_config);

// By default, have heartbeat disabled.
static ConfigVariable<long> heartbeat_timeout_config("heartbeat_timeout", 0, disneyclient_config);

// We only want one client manager instance for every DisneyClient:
static OTPClientManager* g_cm;
static ToontownClientManager* g_ttcm;

// Same with our DNA classes:
static DNALoader* g_dna_loader;
static DNAStorage* g_dna_store;

// And our TalkAssistant:
static TalkAssistant* g_talk_assistant;

class DisneyClient : public Client, public NetworkHandler
{
  private:
    std::shared_ptr<NetworkClient> m_client;

    ConfigNode m_config;
    string m_game_name;
    string m_name_file;
    string m_database_file;
    string m_whitelist_file;
    string m_blacklist_file;
    vector<string> m_dna_files;
    uint32_t m_database_id;
    bool m_clean_disconnect;
    bool m_relocate_owned;
    bool m_send_hash;
    bool m_send_version;
    doid_t m_av_id;

    // Heartbeat:
    long m_heartbeat_timeout;
    Timeout* m_heartbeat_timer = nullptr;

  public:
    DisneyClient(ConfigNode config, ClientAgent* client_agent, const std::shared_ptr<uvw::TcpHandle> &socket,
                 const uvw::Addr &remote, const uvw::Addr &local, const bool haproxy_mode) :
        Client(config, client_agent), m_client(std::make_shared<NetworkClient>(this)),
        m_config(config), m_game_name(game_name_config.get_rval(config)),
        m_dna_files(dna_files_config.get_rval(config)),
        m_name_file(name_file_config.get_rval(config)),
        m_database_file(database_file_config.get_rval(config)),
        m_whitelist_file(whitelist_file_config.get_rval(config)),
        m_blacklist_file(blacklist_file_config.get_rval(config)),
        m_database_id(database_id_config.get_rval(config)), m_clean_disconnect(false),
        m_send_hash(send_hash_to_client.get_rval(config)),
        m_send_version(send_version_to_client.get_rval(config)),
        m_heartbeat_timeout(heartbeat_timeout_config.get_rval(config)),
        m_av_id(0)
    {
        pre_initialize();

        m_client->initialize(socket, remote, local, haproxy_mode);
    }

    inline virtual bool get_send_hash()
    {
        return m_send_hash;
    }

    inline virtual bool get_send_version()
    {
        return m_send_version;
    }

    inline virtual channel_t get_allocated_channel()
    {
        return m_allocated_channel;
    }

    inline virtual channel_t get_client_channel()
    {
        return m_channel;
    }

    inline virtual void set_client_channel(channel_t client_channel)
    {
        m_channel = client_channel;
    }

    virtual void log_warning(string warning)
    {
        m_log->warning() << warning << endl;
    }

    inline virtual string get_server_version()
    {
        return m_client_agent->get_version();
    }

    inline virtual uint32_t get_dc_hash()
    {
        return m_client_agent->get_hash();
    }

    inline virtual void set_client_state(ClientState state)
    {
        m_state = state;
    }

    inline virtual string get_whitelist_file()
    {
        return m_whitelist_file;
    }

    inline virtual string get_blacklist_file()
    {
        return m_blacklist_file;
    }

    inline virtual uint32_t get_avatar_id()
    {
        return m_channel & 0xFFFFFFFF;
    }

    inline virtual uint32_t get_account_id()
    {
        return (m_channel >> 32) & 0xFFFFFFFF;
    }

    virtual void set_avatar_id(uint32_t av_id)
    {
        m_av_id = av_id;
    }

    virtual vector<doid_t> get_visible_avatars()
    {
        vector<doid_t> avs;
        for(auto it : m_visible_objects) {
            if(it.second.dcc == g_cm->m_player_class && it.second.id != m_av_id) {
                avs.push_back(it.second.id);
            }
        }

        return avs;
    }

    virtual void write_server_event(LoggedEvent& event)
    {
        log_event(event);
    }

    virtual void subscribe_to_channel(channel_t channel)
    {
        subscribe_channel(channel);
    }

    virtual void unsubscribe_from_channel(channel_t channel)
    {
        unsubscribe_channel(channel);
    }

    virtual void create_post_remove(channel_t channel, DatagramHandle dg)
    {
        add_post_remove(channel, dg);
    }

    virtual void wipe_post_removes(channel_t channel)
    {
        clear_post_removes(channel);
    }

    virtual void dispatch_datagram(DatagramHandle dg)
    {
        route_datagram(dg);
    }

    virtual void load_DNA_files()
    {
        // Only load our DNA files if we haven't already:
        if(g_dna_loader == nullptr || g_dna_store == nullptr) {
            // Instantiate the DNA classes we need to use.
            g_dna_loader = new DNALoader();
            g_dna_store = new DNAStorage();

            // Load each DNA file from the config.
            for(auto it = m_dna_files.begin(); it != m_dna_files.end(); ++it) {
                // Open up the file.
                ifstream dna_file(*it);
                istream& dna_stream = dna_file;
                if(dna_file.good()) {
                    // Load it.
                    g_dna_loader->load_DNA_file_AI(g_dna_store, &dna_stream, *it);
                } else {
                    // Uh oh: this file does not exist or is corrupt.
                    m_client_agent->log()->warning() << "DNA file doesn't exist or is corrupt!" << endl;
                }
            }
        }
    }

    inline virtual void pre_initialize()
    {
        // Set NetworkClient configuration.
        m_client->set_write_timeout(write_timeout_ms.get_rval(m_config));
        m_client->set_write_buffer(write_buffer_size.get_rval(m_config));

        // Load our DNA files.
        load_DNA_files();
    }

    virtual void heartbeat_timeout()
    {
        lock_guard<recursive_mutex> lock(m_client_lock);
        // The heartbeat timer has already deleted itself at this point
        // Holding on to it means receive_disconnect will try to invoke cancel() on it, and we can't have that.
        m_heartbeat_timer = nullptr;
        send_disconnect(CLIENT_DISCONNECT_NO_HEARTBEAT,
                        "Server timed out while waiting for heartbeat.");
    }

    virtual void initialize()
    {
        //If heartbeat, start the heartbeat timer now.
        if(m_heartbeat_timeout != 0) {
            m_heartbeat_timer = new Timeout(m_heartbeat_timeout,
                                std::bind(&DisneyClient::heartbeat_timeout,
                                          this));
            m_heartbeat_timer->start();
        }

        stringstream ss;
        ss << "Client (" << m_client->get_remote().ip
           << ":" << m_client->get_remote().port << ", " << m_channel << ")";
        m_log->set_name(ss.str());
        set_con_name(ss.str());

        // Set up our Client manager depending on supported game types.
        if(m_game_name == "otp" && g_cm == nullptr) {
            g_cm = new OTPClientManager(g_dcf->get_class_by_name("DistributedPlayer"),
                                        m_database_id, 6, "developer", m_database_file);
        } else if(m_game_name == "toon" && g_cm == nullptr) {
            g_ttcm = new ToontownClientManager(g_dcf->get_class_by_name("DistributedToon"),
                                               m_database_id, "developer", m_database_file, m_name_file);
            g_cm = g_ttcm;
        }

        if(g_talk_assistant == nullptr) {
            g_talk_assistant = new TalkAssistant(g_cm);
        }

        // Load the whitelist.
        g_talk_assistant->load_whitelist(m_whitelist_file);

        // We only log client-connected events for non-LOCAL (HAProxy L4 checks et al) NetworkClient objects.
        if(!m_client->is_local()) {
            // Create event for EventLogger
            LoggedEvent event("client-connected");

            // Add remote endpoint to log
            ss.str(""); // empty the stream
            ss << m_client->get_remote().ip
               << ":" << m_client->get_remote().port;
            event.add("remote_address", ss.str());

            // Add local endpoint to log
            ss.str(""); // empty the stream
            ss << m_client->get_local().ip
               << ":" << m_client->get_local().port;
            event.add("local_address", ss.str());

            // Log created event
            log_event(event);
        }
    }

    // send_disconnect must close any connections with a connected client; the given reason and
    // error should be forwarded to the client. Additionaly, it is recommend to log the event.
    // Handler for CLIENTAGENT_EJECT.
    virtual void send_disconnect(uint16_t reason, const string &error_string, bool security = false)
    {
        if(m_client->is_connected()) {
            Client::send_disconnect(reason, error_string, security);

            DatagramPtr resp = Datagram::create();
            resp->add_uint16(CLIENT_GO_GET_LOST);
            resp->add_uint16(reason);
            resp->add_string(error_string);
            m_client->send_datagram(resp);

            m_clean_disconnect = true;
            m_client->disconnect();
        }
    }

    // receive_datagram is the handler for datagrams received over the network from a Client.
    virtual void receive_datagram(DatagramHandle dg)
    {
        lock_guard<recursive_mutex> lock(m_client_lock);
        DatagramIterator dgi(dg);
        try {
            switch(m_state) {
            // Client has just connected and needs to login.
            case CLIENT_STATE_NEW:
                m_state = CLIENT_STATE_ANONYMOUS;
                handle_client_heartbeat();
                handle_pre_auth(dgi);
                break;
            // We don't differentiate between new and anonymous.
            case CLIENT_STATE_ANONYMOUS:
                handle_pre_auth(dgi);
                break;
            // We have declared the Client authenticated and the client
            // can now interact with the server cluster normally.
            case CLIENT_STATE_ESTABLISHED:
                handle_authenticated(dgi);
                break;
            }
        } catch(const DatagramIteratorEOF&) {
            // Occurs when a handler attempts to read past end of datagram
            send_disconnect(CLIENT_DISCONNECT_TRUNCATED_DATAGRAM,
                            "Datagram unexpectedly ended while iterating.");
            return;
        } catch(const DatagramOverflow&) {
            // Occurs when a handler attempts to prepare or forward a datagram to be sent
            // internally and, the resulting datagram is larger than the max datagram size.
            send_disconnect(CLIENT_DISCONNECT_OVERSIZED_DATAGRAM,
                            "ClientDatagram too large to be routed on MD.", true);
            return;
        }

        if(dgi.get_remaining()) {
            // All client handlers should consume all data in the datagram (for validation and security).
            // If the handler read all the data it expected, and some remains, the datagram was sent with
            // additional junk data on the end.
            send_disconnect(CLIENT_DISCONNECT_OVERSIZED_DATAGRAM, "Datagram contains excess data.", true);
            return;
        }
    }

    // receive_disconnect is called when the Client closes the tcp
    //     connection or otherwise when the tcp connection is lost.
    // Note: In the OTP client protocol, the server is normally
    //       responsible for terminating the connection.
    virtual void receive_disconnect(const uvw::ErrorEvent &evt)
    {
        lock_guard<recursive_mutex> lock(m_client_lock);

        if(!m_clean_disconnect && !m_client->is_local()) {
            stringstream ss;
            LoggedEvent event("client-lost");
            // Remote address:
            ss << m_client->get_remote().ip
               << ":" << m_client->get_remote().port;
            event.add("remote_address", ss.str());
            // Clear the stream.
            ss.str("");
            // Local address:
            ss << m_client->get_local().ip
               << ":" << m_client->get_local().port;
            event.add("local_address", ss.str());
            event.add("reason", evt.what());
            log_event(event);
        }

        if(m_heartbeat_timer != nullptr) {
            m_heartbeat_timer->cancel();
            m_heartbeat_timer = nullptr;
        }

        // If we had an active avatar, it is now lost.
        if(m_av_id != 0) {
            g_cm->lost_object(*this, m_av_id);
        }

        annihilate();
    }

    // forward_datagram should foward the datagram to the client, or where appopriate parse
    // the packet and send the appropriate equivalent data.
    // Handler for CLIENTAGENT_SEND_DATAGRAM.
    virtual void forward_datagram(DatagramHandle dg, bool send_datagram = 0)
    {
        m_client->send_datagram(dg);
    }

    // handle_drop should immediately disconnect the client without sending any more data.
    // Handler for CLIENTAGENT_DROP.
    virtual void handle_drop()
    {
        m_clean_disconnect = true;
        m_client->disconnect();
    }

    // handle_add_interest should inform the client of an interest added by the server.
    virtual void handle_add_interest(const Interest& i, uint32_t context)
    {
        bool multiple = i.zones.size() > 1;

        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_ADD_INTEREST);
        resp->add_uint32(context);
        resp->add_uint16(i.id);
        resp->add_doid(i.parent);
        if(multiple) {
            resp->add_uint16(i.zones.size());
        }
        for(auto it = i.zones.begin(); it != i.zones.end(); ++it) {
            resp->add_zone(*it);
        }
        m_client->send_datagram(resp);
    }

    // handle_remove_interest should inform the client an interest was removed by the server.
    virtual void handle_remove_interest(uint16_t interest_id, uint32_t context)
    {
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_REMOVE_INTEREST);
        resp->add_uint32(context);
        resp->add_uint16(interest_id);
        m_client->send_datagram(resp);
    }

    // handle_add_object should inform the client of a new object. The datagram iterator
    // provided starts at the 'required fields' data, and may have optional fields following.
    // Handler for OBJECT_ENTER_LOCATION (an object, enters the Client's interest).
    virtual void handle_add_object(doid_t do_id, doid_t parent_id, zone_t zone_id, uint16_t dc_id,
                                   DatagramIterator &dgi, bool other)
    {
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(other ? CLIENT_CREATE_OBJECT_REQUIRED_OTHER : CLIENT_CREATE_OBJECT_REQUIRED);
        resp->add_location(parent_id, zone_id);
        resp->add_uint16(dc_id);
        resp->add_doid(do_id);
        resp->add_data(dgi.read_remainder());
        m_client->send_datagram(resp);
    }

    // handle_add_ownership should inform the client it has control of a new object. The datagram
    // iterator provided starts at the 'required fields' data, and may have 'optional fields'.
    // Handler for OBJECT_ENTER_OWNER (an object, enters the Client's ownership).
    virtual void handle_add_ownership(doid_t do_id, doid_t parent_id, zone_t zone_id, uint16_t dc_id,
                                      DatagramIterator &dgi)
    {
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_GET_AVATAR_DETAILS_RESP);
        resp->add_uint32(do_id);
        resp->add_uint8(0);
        resp->add_data(dgi.read_remainder());
        m_client->send_datagram(resp);
    }

    // handle_set_field should inform the client that the field has been updated.
    virtual void handle_set_field(doid_t do_id, uint16_t field_id, DatagramIterator &dgi)
    {
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_OBJECT_UPDATE_FIELD);
        resp->add_doid(do_id);
        resp->add_uint16(field_id);
        resp->add_data(dgi.read_remainder());
        m_client->send_datagram(resp);
    }

    // handle_set_fields should inform the client that a group of fields has been updated. Not used.
    virtual void handle_set_fields(doid_t do_id, uint16_t num_fields, DatagramIterator &dgi)
    {
    }

    // handle_change_location should inform the client that the objects location has changed.
    virtual void handle_change_location(doid_t do_id, doid_t new_parent, zone_t new_zone)
    {
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_OBJECT_LOCATION);
        resp->add_doid(do_id);
        resp->add_location(new_parent, new_zone);
        m_client->send_datagram(resp);
    }

    // handle_remove_object should send a mesage to remove the object from the connected client.
    // Handler for cases where an object is no longer visible to the client;
    //     for example, when it changes zone, leaves visibility, or is deleted.
    virtual void handle_remove_object(doid_t do_id, bool deleted)
    {
        DatagramPtr resp = Datagram::create();
        if(deleted) {
            resp->add_uint16(CLIENT_OBJECT_DELETE);
        }
        else {
            resp->add_uint16(CLIENT_OBJECT_DISABLE);
        }
        resp->add_doid(do_id);
        m_client->send_datagram(resp);
    }

    // handle_remove_ownership should notify the client it no longer has control of the object.
    // Handle when the client loses ownership of an object.
    virtual void handle_remove_ownership(doid_t do_id)
    {
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_OBJECT_DISABLE_OWNER);
        resp->add_doid(do_id);
        m_client->send_datagram(resp);
    }

    // handle_interest_done is called when all of the objects from an opened interest have been
    // received. Typically, informs the client that a particular group of objects is loaded.
    virtual void handle_interest_done(uint16_t interest_id, uint32_t context)
    {
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_DONE_INTEREST_RESP);
        resp->add_uint16(interest_id);
        resp->add_uint32(context);
        m_client->send_datagram(resp);
    }

    // A database object related to this Client has been created.
    virtual void handle_create_object_resp(DatagramIterator &dgi)
    {
        uint32_t ctx = dgi.read_uint32();
        doid_t do_id = dgi.read_uint32();

        // Call back the operator.
        if(g_cm->m_context_operator.find(ctx) != g_cm->m_context_operator.end()) {
            g_cm->m_context_operator[ctx]->handle_create(ctx, do_id);
        }
    }

    // Field(s) were updated by us.
    virtual void handle_set_field_resp(DatagramIterator &dgi)
    {
        // Collect the context and success.
        uint32_t ctx = dgi.read_uint32();
        uint8_t success = dgi.read_uint8();

        // Call back the operator.
        if(g_cm->m_context_operator.find(ctx) != g_cm->m_context_operator.end()) {
            g_cm->m_context_operator[ctx]->handle_update(ctx, success);
        }
    }

    // handle_get_field_resp occurs when the DB server sends a DBSERVER_OBJECT_GET_ALL_RESP.
    virtual void handle_get_field_resp(DatagramIterator &dgi)
    {
        // Collect the information the DB server sent us.
        uint32_t ctx = dgi.read_uint32();
        uint8_t success = dgi.read_uint8();

        // Call back the operator if we didn't succeed.
        if(!success) {
            if(g_cm->m_context_operator.find(ctx) != g_cm->m_context_operator.end()) {
                g_cm->m_context_operator[ctx]->handle_query(dgi, ctx, 0);
            }
        }

        uint16_t dclass_id = dgi.read_uint16();

        if(g_cm->m_context_operator.find(ctx) != g_cm->m_context_operator.end()) {
            g_cm->m_context_operator[ctx]->handle_query(dgi, ctx, dclass_id);
        }
    }

    // handle_get_activated_resp occurs when the DBSS sends a DBSS_OBJECT_GET_ACTIVATED_RESP.
    virtual void handle_get_activated_resp(DatagramIterator &dgi)
    {
        // Collect the information the DB server sent us.
        uint32_t ctx = dgi.read_uint32();
        uint32_t do_id = dgi.read_uint32();
        bool activated = dgi.read_uint8() ? true : false;

        if(g_cm->m_context_operator.find(ctx) != g_cm->m_context_operator.end()) {
            g_cm->m_context_operator[ctx]->get_activated_resp(do_id, ctx, activated);
        }
    }

    // Custom handling for internally received datagrams.
    virtual bool handle_cluster_datagram(DatagramHandle in_dg, DatagramIterator &dgi,
                                         channel_t sender, uint16_t msgtype)
    {
        switch(msgtype) {
        case DBSERVER_CREATE_OBJECT_RESP:
            handle_create_object_resp(dgi);
            break;
        case DBSERVER_OBJECT_GET_ALL_RESP:
            handle_get_field_resp(dgi);
            break;
        case DBSERVER_OBJECT_SET_FIELD_IF_EQUALS_RESP:
            handle_set_field_resp(dgi);
            break;
        case DBSERVER_OBJECT_SET_FIELDS_IF_EQUALS_RESP:
            handle_set_field_resp(dgi);
            break;
        case DBSS_OBJECT_GET_ACTIVATED_RESP:
            handle_get_activated_resp(dgi);
            break;
        case CLIENT_FRIEND_ONLINE: {
            DatagramPtr resp = Datagram::create();
            resp->add_uint16(CLIENT_FRIEND_ONLINE);
            resp->add_data(dgi.read_remainder());
            m_client->send_datagram(resp);
        }
        break;
        case CLIENT_FRIEND_OFFLINE: {
            DatagramPtr resp = Datagram::create();
            resp->add_uint16(CLIENT_FRIEND_OFFLINE);
            resp->add_data(dgi.read_remainder());
            m_client->send_datagram(resp);
        }
        break;
        default:
            return false;
        }

        // If we broke, we were successful.
        return true;
    }

    // Client needs to login.
    virtual void handle_pre_auth(DatagramIterator &dgi)
    {
        uint16_t msg_type = dgi.read_uint16();
        switch(msg_type) {
        case CLIENT_DISCONNECT: {
            LoggedEvent event("client-disconnected");
            log_event(event);

            m_clean_disconnect = true;
            m_client->disconnect();
        }
        break;
        case CLIENT_LOGIN_2: {
            string play_token = dgi.read_string();
            string version = dgi.read_string();
            uint32_t dc_hash = dgi.read_uint32();
            int32_t token_type = dgi.read_int32();
            dgi.skip(sizeof(string)); // validate download
            string want_magic_words = dgi.read_string();

            g_cm->login(*this, play_token, m_channel, version,
                        dc_hash, token_type, want_magic_words);
        }
        break;
        case CLIENT_LOGIN_TOONTOWN: {
            if(m_game_name != "toon") {
                // We aren't running in Toontown mode. Disconnect.
                stringstream ss;
                ss << "Message type " << msg_type << " cannot be sent when not in Toontown mode.";
                send_disconnect(CLIENT_DISCONNECT_INVALID_MSGTYPE, ss.str(), true);
                return;
            }

            string play_token = dgi.read_string();
            string version = dgi.read_string();
            uint32_t dc_hash = dgi.read_uint32();
            int32_t token_type = dgi.read_int32();
            string want_magic_words = dgi.read_string();

            g_cm->login(*this, play_token, m_channel, version,
                        dc_hash, token_type, want_magic_words);
        }
        break;
        case CLIENT_OBJECT_UPDATE_FIELD:
            handle_client_object_update_field(dgi);
            break;
        case CLIENT_HEARTBEAT:
            handle_client_heartbeat();
            break;
        default:
            stringstream ss;
            ss << "Message type " << msg_type << " not allowed prior to authentication.";
            send_disconnect(CLIENT_DISCONNECT_INVALID_MSGTYPE, ss.str(), true);
            return;
        }
    }

    // We have declared the Client authenticated and the client
    // can now interact with the server cluster normally.
    virtual void handle_authenticated(DatagramIterator &dgi)
    {
        uint16_t msg_type = dgi.read_uint16();
        switch(msg_type) {
        case CLIENT_DISCONNECT: {
            LoggedEvent event("client-disconnected");
            log_event(event);

            m_clean_disconnect = true;
            m_client->disconnect();
        }
        break;
        case CLIENT_OBJECT_UPDATE_FIELD:
            handle_client_object_update_field(dgi);
            break;
        case CLIENT_OBJECT_LOCATION:
            handle_client_object_location(dgi);
            break;
        case CLIENT_SET_AVATAR: {
            doid_t av_id = dgi.read_uint32();

            g_cm->request_play_avatar(*this, get_account_id(),
                                      av_id, get_avatar_id());

            m_av_id = av_id;
        }
        break;
        case CLIENT_REMOVE_FRIEND:
            g_ttcm->remove_friend_request(*this, get_account_id(),
                                          dgi.read_uint32(), get_avatar_id());
            break;
        case CLIENT_GET_FRIEND_LIST:
            g_ttcm->get_friends_list_request(*this, get_account_id(), get_avatar_id());
            break;
        case CLIENT_GET_AVATAR_DETAILS:
        case CLIENT_GET_PET_DETAILS:
            g_ttcm->get_avatar_details_request(*this, get_account_id(),
                                               dgi.read_uint32(), get_avatar_id());
            break;
        case CLIENT_ADD_INTEREST:
            handle_client_add_interest(dgi, false);
            break;
        case CLIENT_REMOVE_INTEREST:
            handle_client_remove_interest(dgi);
            break;
        case CLIENT_CREATE_AVATAR: {
            uint16_t echo_context = dgi.read_uint16();
            string dna_string = dgi.read_string();
            uint8_t index = dgi.read_uint8();

            g_cm->create_avatar(*this, get_account_id(),
                                dna_string, index);
        }
        break;
        case CLIENT_SET_NAME_PATTERN: {
            if(m_game_name != "toon") {
                // We aren't running in Toontown mode. Disconnect.
                stringstream ss;
                ss << "Message type " << msg_type << " cannot be sent when not in Toontown mode.";
                send_disconnect(CLIENT_DISCONNECT_INVALID_MSGTYPE, ss.str(), true);
                return;
            }

            uint32_t av_id = dgi.read_uint32();

            // A slight discrepancy caused by Disney:
            // f1/f2/f3/f4 are supposed to be uint8, but the client
            // sends everything as int16!
            int16_t p1 = dgi.read_int16();
            uint8_t f1 = (uint8_t)dgi.read_int16();
            int16_t p2 = (uint8_t)dgi.read_int16();
            uint8_t f2 = (uint8_t)dgi.read_int16();
            int16_t p3 = (uint8_t)dgi.read_int16();
            uint8_t f3 = (uint8_t)dgi.read_int16();
            int16_t p4 = (uint8_t)dgi.read_int16();
            uint8_t f4 = (uint8_t)dgi.read_int16();

            g_ttcm->set_name_pattern(*this, get_account_id(), av_id, p1, f1,
                                     p2, f2, p3, f3, p4, f4);
        }
        break;
        case CLIENT_SET_WISHNAME: {
            if(m_game_name != "toon") {
                // We aren't running in Toontown mode. Disconnect.
                stringstream ss;
                ss << "Message type " << msg_type << " cannot be sent when not in Toontown mode.";
                send_disconnect(CLIENT_DISCONNECT_INVALID_MSGTYPE, ss.str(), true);
                return;
            }

            uint32_t av_id = dgi.read_uint32();
            string name = dgi.read_string();

            g_ttcm->set_name_typed(*this, get_account_id(), av_id, name);
        }
        break;
        case CLIENT_GET_AVATARS:
            g_cm->request_avatar_list(*this, get_account_id());
            break;
        case CLIENT_HEARTBEAT:
            handle_client_heartbeat();
            break;
        default:
            stringstream ss;
            ss << "Message type " << msg_type << " not valid.";
            send_disconnect(CLIENT_DISCONNECT_INVALID_MSGTYPE, ss.str(), true);
            return;
        }
    }

    // handle_client_heartbeat should ensure this client does not get reset for the current interval.
    // Handler for CLIENT_HEARTBEAT message
    virtual void handle_client_heartbeat()
    {
        if(m_heartbeat_timer != nullptr) {
            m_heartbeat_timer->reset();
        }
    }

    // handle_client_object_update_field occurs when a client sends an OBJECT_UPDATE_FIELD
    virtual void handle_client_object_update_field(DatagramIterator &dgi)
    {
        doid_t do_id = dgi.read_doid();
        uint16_t field_id = dgi.read_uint16();

        // Get class of object from cache
        DCClass *dcc = lookup_object(do_id);

        // A bit of a hack, but Toontown clients will notify offline
        // friends when they get removed, which is unnecessary.
        if(!dcc && field_id == 109) { // friendsNotify
            dgi.skip(dgi.get_remaining());
            return;
        }

        // If the class couldn't be found, error out:
        if(!dcc) {
            dgi.skip(dgi.get_remaining());
            if(is_historical_object(do_id)) {
                // The client isn't disconnected in this case because it could be a delayed
                // message, we also have to skip to the end so a disconnect overside_datagram
                // is not sent.
                // TODO: Allow configuration to limit how long historical objects remain,
                //       for example with a timeout or bad-message limit.
            } else {
                stringstream ss;
                ss << "Client tried to send update to nonexistent object " << do_id;
                send_disconnect(CLIENT_DISCONNECT_MISSING_OBJECT, ss.str(), true);
            }
            return;
        }

        // If the client is not in the ESTABLISHED state, it may only send updates
        // to anonymous UberDOGs.
        if(m_state != CLIENT_STATE_ESTABLISHED) {
            if(g_uberdogs.find(do_id) == g_uberdogs.end() || !g_uberdogs[do_id].anonymous) {
                stringstream ss;
                ss << "Client tried to send update to non-anonymous object "
                   << dcc->get_name() << "(" << do_id << ")";
                send_disconnect(CLIENT_DISCONNECT_ANONYMOUS_VIOLATION, ss.str(), true);
                return;
            }
        }

        // Check that the client sent a field that actually exists in the class.
        DCField *field = dcc->get_field_by_index(field_id);
        if(!field) {
            stringstream ss;
            ss << "Client tried to send update for nonexistent field " << field_id << " to object "
               << dcc->get_name() << "(" << do_id << ")";
            send_disconnect(CLIENT_DISCONNECT_FORBIDDEN_FIELD, ss.str(), true);
            return;
        }

        // Check that the client is actually allowed to send updates to this field
        bool is_owned = m_owned_objects.find(do_id) != m_owned_objects.end();
        if(!field->has_keyword("clsend") && !(is_owned && field->has_keyword("ownsend"))) {
            auto send_it = m_fields_sendable.find(do_id);
            if(send_it == m_fields_sendable.end() ||
               send_it->second.find(field_id) == send_it->second.end()) {
                stringstream ss;
                ss << "Client tried to send update for non-sendable field: "
                   << dcc->get_name() << "(" << do_id << ")." << field->get_name();
                send_disconnect(CLIENT_DISCONNECT_FORBIDDEN_FIELD, ss.str(), true);
                return;
            }
        }

        if((field_id == 103 || field_id == 104) && m_av_id > 0) {
            // The client is sending a talk field. We need to take this over
            // and route it through the TalkPath system.
            if(field_id == 103) {
                g_talk_assistant->set_talk(*this, m_av_id, dgi);
            } else {
                g_talk_assistant->set_talk_whisper(*this, do_id, m_av_id, dgi);
            }
            return;
        }

        // If a datagram read-related exception occurs while unpacking data it will be handled by
        // receive_datagram and the client will be dc'd with "truncated datagram".
        vector<uint8_t> data;

        try {
            dgi.unpack_field(field, data);
        } catch(const FieldConstraintViolation& violation) {
            // The field that was being updated has constraints.
            // One of its attributes (either length or value) violates the type constraints specified in our dclass.
            std::stringstream ss;
;           ss << "Client tried to send update that violates the constraints for field "
               << dcc->get_name() << "(" << do_id << ")." << field->get_name()
               << ": " << violation.what();
            send_disconnect(CLIENT_DISCONNECT_FIELD_CONSTRAINT, ss.str(), true);
            return;
        }

        // If an exception occurs while packing data it will be handled by
        // receive_datagram and the client will be dc'd with "oversized datagram".
        DatagramPtr resp = Datagram::create();
        resp->add_server_header(do_id, m_channel, STATESERVER_OBJECT_SET_FIELD);
        resp->add_doid(do_id);
        resp->add_uint16(field_id);
        resp->add_data(data);
        route_datagram(resp);
    }

    // handle_client_object_location occurs when a client sends an OBJECT_LOCATION message.
    // When sent by the client, this represents a request to change the object's location.
    virtual void handle_client_object_location(DatagramIterator &dgi)
    {
        // Check that the object the client is trying manipulate actually exists
        // and that the client is actually allowed to change the object's location
        doid_t do_id = dgi.read_doid();

        bool is_owned = m_owned_objects.find(do_id) != m_owned_objects.end();
        bool is_relocatable = m_objects_relocatable.find(do_id) != m_objects_relocatable.end();
        if(!is_owned && !is_relocatable) {
            if(is_historical_object(do_id)) {
                // The client isn't disconnected in this case because it could be a delayed
                // message, we also have to skip to the end so a disconnect overside_datagram
                // is not sent.
                // TODO: Allow configuration to limit how long historical objects remain,
                //       for example with a timeout or bad-message limit.
                dgi.skip(dgi.get_remaining());
            }

            else if(m_visible_objects.find(do_id) != m_visible_objects.end()) {
                send_disconnect(CLIENT_DISCONNECT_FORBIDDEN_RELOCATE,
                                "Can't relocate an object the client doesn't own", true);
            }

            else {
                stringstream ss;
                ss << "Client tried to manipulate unknown object " << do_id;
                send_disconnect(CLIENT_DISCONNECT_MISSING_OBJECT, ss.str(), true);
            }

            return;
        }

        // Update the object's location
        DatagramPtr dg = Datagram::create(do_id, 0, STATESERVER_OBJECT_SET_LOCATION);
        dg->add_doid(dgi.read_doid()); // Parent
        dg->add_zone(dgi.read_zone()); // Zone
        route_datagram(dg);
    }

    // handle_client_add_interest occurs is called when the client adds an interest.
    virtual void handle_client_add_interest(DatagramIterator &dgi, bool multiple)
    {
        uint16_t handle = dgi.read_uint16();
        uint32_t context = dgi.read_uint32();
        uint32_t parent_id = dgi.read_uint32();

        set<zone_t> zones;
        for(dgsize_t i{}; i < dgi.get_remaining(); ++i) {
            zones.insert(dgi.read_zone());
        }

        // If we're Toontown, we need to make sure all VisGroups
        // are included in this interest, if applicable.
        if(m_game_name == "toon" && zones.size() == 1 && m_av_id > 0) {
            zone_t zone_id = *next(zones.begin(), 0);

            // Determine if this zone is within the Cog HQ range or not.
            bool is_cog_hq_zone = zone_id >= 10000 && zone_id < 15000;

            // Calculate the VisGroup branch ID.
            zone_t branch_id = zone_id - zone_id % 100;
            if(!is_cog_hq_zone) {
                if(zone_id % 1000 >= 500) {
                    branch_id -= 500;
                }
            }

            // Calculate the zone suffix.
            zone_t suffix = zone_id % 1000;
            bool safe_zone = false;
            if(suffix >= 500) {
                suffix -= 500;
            }
            if(!is_cog_hq_zone && suffix < 100) {
                safe_zone = true;
            }

            if(branch_id > 0 && ((zone_id != branch_id || zone_id >= 10000 && zone_id != 11200) && !safe_zone)) {
                map<zone_t, vector<zone_t> > zone_vis_map;
                for(size_t i = 0; i < g_dna_store->get_num_DNA_vis_groups_AI(); ++i) {
                    string group_full_name = g_dna_store->get_DNA_vis_group_name(i);
                    DNAVisGroup* vis_group = g_dna_store->get_DNA_vis_group_AI(i);

                    // Set up a stream for splitting the vis group's full name.
                    vector<string> result;
                    stringstream ss(group_full_name);
                    string tok;
                    size_t splits = 0;

                    // Keep splitting by the colon delimiter until we hit
                    // our maximum split (just one).
                    while(std::getline(ss, tok, ':')) {
                        if(splits > 1) {
                            break;
                        }

                        result.push_back(tok);
                        ++splits;
                    }

                    // Now, get the first element in our vector, and
                    // turn it into a VisZone ID.
                    string string_zone = result.front();
                    zone_t vis_zone_id = (zone_t)stoi(string_zone);
                    if(vis_zone_id != zone_id) {
                        continue;
                    }

                    // Calculate the true zone ID.
                    vis_zone_id = get_true_zone_id(vis_zone_id, branch_id);

                    // Put each visible zone in a new vector.
                    vector<zone_t> visibles;
                    for(size_t i = 0; i < vis_group->get_num_visibles(); ++i) {
                        visibles.push_back(stoi(vis_group->get_visible_name(i)));
                    }

                    // Calculate the branch zone.
                    zone_t branch_zone = vis_zone_id - vis_zone_id % 100;
                    if(!(vis_zone_id >= 10000 && vis_zone_id < 15000)) {
                        if(vis_zone_id % 1000 >= 500) {
                            branch_zone -= 500;
                        }
                    }

                    // Add the branch zone to the visibles vector as well.
                    visibles.push_back(branch_zone);

                    // Finally, add the visibles to the map we created.
                    zone_vis_map[vis_zone_id] = visibles;
                }

                // Toss each recorded zone into the set.
                for(auto it : zone_vis_map) {
                    zones.insert(it.first);
                    for(auto zone : it.second) {
                        zones.insert(zone);
                    }
                }

                // Move the current avatar's location.
                DatagramPtr dg = Datagram::create(m_av_id, 0, STATESERVER_OBJECT_SET_LOCATION);
                dg->add_doid(parent_id);
                dg->add_zone(zone_id);
                route_datagram(dg);

                // Finally, include the branch ID as well.
                zones.insert(branch_id);
            }
        }

        // Compile a new Datagram with the newer interest packet format.
        DatagramPtr dg = Datagram::create();
        dg->add_uint16(handle);
        dg->add_doid(parent_id);
        dg->add_uint16(zones.size());
        for(auto zone : zones) {
            dg->add_zone(zone);
        }

        // Get the new DatagramIterator.
        DatagramIterator new_dgi = DatagramIterator(dg);

        Interest i;
        build_interest(new_dgi, true, i);
        add_interest(i, context, m_channel);
    }

    // handle_client_remove_interest is called when the client removes an interest.
    virtual void handle_client_remove_interest(DatagramIterator &dgi)
    {
        uint16_t id = dgi.read_uint16();
        uint32_t context = dgi.read_uint32();

        // check the interest actually exists to be removed
        if(m_interests.find(id) == m_interests.end()) {
            send_disconnect(CLIENT_DISCONNECT_GENERIC, "Tried to remove a non-existing interest.", true);
            return;
        }

        Interest &i = m_interests[id];
        remove_interest(i, context);
    }

    virtual zone_t get_canonical_zone_id(zone_t zone_id)
    {
        if(zone_id == 0) {
            zone_id = 2000;
        } else if(zone_id >= 22000 && zone_id < 61000) {
            zone_id = zone_id % 2000;
            if(zone_id < 1000) {
                zone_id = zone_id + 2000;
            } else {
                zone_id = zone_id - 1000 + 8000;
            }
        }

        return zone_id;
    }

    virtual zone_t get_true_zone_id(zone_t zone_id, zone_t current_zone_id)
    {
        if(zone_id >= 22000 && zone_id < 61000 || zone_id == 0) {
            zone_id = get_canonical_zone_id(zone_id);
        }

        if(current_zone_id >= 22000 && current_zone_id < 61000) {
            zone_t hood_id = zone_id - zone_id % 1000;
            zone_t offset = current_zone_id - current_zone_id % 2000;
            if(hood_id == 2000) {
                return zone_id - 2000 + offset;
            } else if(hood_id == 8000) {
                return zone_id - 8000 + offset + 1000;
            }
        }

        return zone_id;
    }

    virtual const std::string get_remote_address()
    {
        return m_client->get_remote().ip;
    }

    virtual uint16_t get_remote_port()
    {
        return m_client->get_remote().port;
    }

    virtual const std::string get_local_address()
    {
        return m_client->get_local().ip;
    }

    virtual uint16_t get_local_port()
    {
        return m_client->get_local().port;
    }

    virtual const std::vector<uint8_t>& get_tlvs() const
    {
        return m_client->get_tlvs();
    }

};

static ClientType<DisneyClient> disney_client_fact("libdisney");
