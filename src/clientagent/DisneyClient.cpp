#include "Client.h"
#include "ClientMessages.h"
#include "ClientFactory.h"
#include "ClientAgent.h"
#include "DisneyClientMessages.h"
#include "net/NetworkClient.h"
#include "json/json.hpp"
#include "core/global.h"
#include "core/msgtypes.h"
#include "config/constraints.h"
#include "dclass/dcPacker.h"
#include "dclass/dcClass.h"
#include "dclass/dcField.h"
#include "util/Timeout.h"

using namespace std;
using json = nlohmann::json;

static ConfigGroup disneyclient_config("libdisney", ca_client_config);

static ConfigVariable<uint32_t> database_id_config("database_id", 4003, disneyclient_config);
static ConfigVariable<string> database_file_config("database_file", "account-bridge.json", disneyclient_config);
static ConfigVariable<bool> send_hash_to_client("send_hash", true, disneyclient_config);
static ConfigVariable<bool> send_version_to_client("send_version", true, disneyclient_config);

static ConfigVariable<uint64_t> write_buffer_size("write_buffer_size", 256 * 1024,
        disneyclient_config);
static ConfigVariable<unsigned int> write_timeout_ms("write_timeout_ms", 6000, disneyclient_config);

// By default, have heartbeat disabled.
static ConfigVariable<long> heartbeat_timeout_config("heartbeat_timeout", 0, disneyclient_config);

// Avatar context struct:
struct AvatarContext {
    uint16_t echo_context;
    string dna_string;
    uint8_t index;
};

// Post-creation Avatar context struct:
struct AvatarCreateContext {
    uint16_t echo_context;
    string dna_string;
    uint8_t index;
    std::set<uint32_t> av_set;
};

class DisneyClient : public Client, public NetworkHandler
{
  private:
    std::shared_ptr<NetworkClient> m_client;
    uint32_t m_context = 0;
    std::map<uint32_t, string> m_token_contexts;
    std::map<uint32_t, AvatarContext> m_create_av_contexts;
    std::map<uint32_t, AvatarCreateContext> m_creation_av_contexts;

    ConfigNode m_config;
    string m_database_file;
    uint32_t m_database_id;
    bool m_clean_disconnect;
    bool m_relocate_owned;
    bool m_send_hash;
    bool m_send_version;

    // Heartbeat:
    long m_heartbeat_timeout;
    Timeout* m_heartbeat_timer = nullptr;

  public:
    DisneyClient(ConfigNode config, ClientAgent* client_agent, const std::shared_ptr<uvw::TcpHandle> &socket,
                 const uvw::Addr &remote, const uvw::Addr &local, const bool haproxy_mode) :
        Client(config, client_agent), m_client(std::make_shared<NetworkClient>(this)),
        m_config(config), m_database_file(database_file_config.get_rval(config)),
        m_database_id(database_id_config.get_rval(config)), m_clean_disconnect(false),
        m_send_hash(send_hash_to_client.get_rval(config)),
        m_send_version(send_version_to_client.get_rval(config)),
        m_heartbeat_timeout(heartbeat_timeout_config.get_rval(config))
    {
        pre_initialize();

        m_client->initialize(socket, remote, local, haproxy_mode);
    }

    inline void pre_initialize()
    {
        // Set NetworkClient configuration.
        m_client->set_write_timeout(write_timeout_ms.get_rval(m_config));
        m_client->set_write_buffer(write_buffer_size.get_rval(m_config));
    }

    void heartbeat_timeout()
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

        annihilate();
    }

    // forward_datagram should foward the datagram to the client, or where appopriate parse
    // the packet and send the appropriate equivalent data.
    // Handler for CLIENTAGENT_SEND_DATAGRAM.
    virtual void forward_datagram(DatagramHandle dg)
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
        resp->add_uint16(multiple ? CLIENT_ADD_INTEREST_MULTIPLE : CLIENT_ADD_INTEREST);
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

    // handle_set_fields should inform the client that a group of fields has been updated.
    virtual void handle_set_fields(doid_t do_id, uint16_t num_fields, DatagramIterator &dgi)
    {
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_OBJECT_SET_FIELDS);
        resp->add_doid(do_id);
        resp->add_uint16(num_fields);
        resp->add_data(dgi.read_remainder());
        m_client->send_datagram(resp);
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

    // Method for packing an int into a field.
    virtual void pack_int(DCPacker &packer, DCField *field, int value)
    {
        packer.raw_pack_uint16(field->get_number());
        packer.begin_pack(field);
        packer.push();
        packer.pack_int(value);
        packer.pop();
        packer.end_pack();
    }

    // Method for packing a uint into a field.
    virtual void pack_uint(DCPacker &packer, DCField *field, unsigned int value)
    {
        packer.raw_pack_uint16(field->get_number());
        packer.begin_pack(field);
        packer.push();
        packer.pack_uint(value);
        packer.pop();
        packer.end_pack();
    }

    // Method for packing a string into a field.
    virtual void pack_string(DCPacker &packer, DCField *field, const string value)
    {
        packer.raw_pack_uint16(field->get_number());
        packer.begin_pack(field);
        packer.push();
        packer.pack_string(value);
        packer.pop();
        packer.end_pack();
    }

    // Toontown client wants to login.
    virtual void handle_client_login_toontown(DatagramIterator &dgi)
    {
        string play_token = dgi.read_string();
        string version = dgi.read_string();
        uint32_t dc_hash = dgi.read_uint32();
        int32_t token_type = dgi.read_int32();
        string want_magic_words = dgi.read_string();

        lock_guard<recursive_mutex> lock(m_client_lock);

        // Before we worry about the play token, we need to compare
        // the DC hash and server version we received.
        if(version != m_client_agent->get_version()) {
            stringstream ss;
            ss << "Bad DC Version Compare: client=" << version;
            if(m_send_version) {
                ss << ", server=" << m_client_agent->get_version();
            }

            login_toontown_response(-1, ss.str(), "", 0);

            return;
        }

        // Now, check the DC hash.
        const static uint32_t expected_hash = m_client_agent->get_hash();
        if(dc_hash != expected_hash) {
            stringstream ss;
            ss << "Bad DC Version Compare: client=0x" << hex << dc_hash;
            if(m_send_hash) {
                ss << ", server=0x" << expected_hash;
            }

            login_toontown_response(-1, ss.str(), "", 0);

            return;
        }

        // Read our JSON account bridge.
        ifstream acc_file(m_database_file);
        if(acc_file) {
            // Our account bridge is valid, so let's parse:
            json acc_bridge = json::parse(acc_file);

            // Check if the play token exists in the account bridge.
            if(acc_bridge.find(play_token) != acc_bridge.end()) {
                // We already have an Account, so let's go ahead and login.
                string strid = acc_bridge[play_token].get<string>();
                stringstream ss(strid);
                doid_t do_id;
                ss >> do_id;
                login_account(play_token, do_id);
                return;
            }
        }

        // We'll need to create an Account object.
        // Instantiate a packer and start packing Account fields.
        DCPacker packer;
        DCClass *account = g_dcf->get_class_by_name("Account");

        // First, pack default values for ACCOUNT_AV_SET.
        DCField *av_set_field = account->get_field_by_name("ACCOUNT_AV_SET");
        packer.raw_pack_uint16(av_set_field->get_number());
        packer.begin_pack(av_set_field);
        packer.push();
        packer.pack_uint(0);
        packer.pack_uint(0);
        packer.pack_uint(0);
        packer.pack_uint(0);
        packer.pack_uint(0);
        packer.pack_uint(0);
        packer.pop();
        packer.end_pack();

        // Pack default values for ACCOUNT_AV_SET_DEL.
        DCField *av_del_field = account->get_field_by_name("ACCOUNT_AV_SET_DEL");
        packer.raw_pack_uint16(av_del_field->get_number());
        packer.begin_pack(av_del_field);
        packer.pack_default_value();
        packer.end_pack();

        // Pack the default value for ESTATE_ID.
        DCField *estate_field = account->get_field_by_name("ESTATE_ID");
        pack_uint(packer, estate_field, 0);

        // Calculate the current system time.
        auto end = chrono::system_clock::now();
        std::time_t end_time = chrono::system_clock::to_time_t(end);

        // Pack the time values for CREATED and LAST_LOGIN.
        DCField *created_field = account->get_field_by_name("CREATED");
        packer.raw_pack_uint16(created_field->get_number());
        packer.begin_pack(created_field);
        packer.pack_string(ctime(&end_time));
        packer.end_pack();
        DCField *login_field = account->get_field_by_name("LAST_LOGIN");
        packer.raw_pack_uint16(login_field->get_number());
        packer.begin_pack(login_field);
        packer.pack_string(ctime(&end_time));
        packer.end_pack();

        // Get a new context for when we receive the response.
        uint32_t context = m_context++;
        m_token_contexts[context] = play_token;

        // Create the Account object through the database server.
        DatagramPtr dg = Datagram::create();
        dg->add_server_header(m_database_id, m_channel, DBSERVER_CREATE_OBJECT);
        dg->add_uint32(context);
        dg->add_uint16(account->get_number());
        dg->add_uint16(5);
        dg->add_data(packer.get_string());
        route_datagram(dg);
    }

    // Client is being logged in.
    virtual void login_account(string play_token, doid_t do_id)
    {
        // Get the account connection channel.
        int64_t acc_channel = static_cast<unsigned long long>(do_id) << 32;

        // If somebody is already logged in, eject them.
        DatagramPtr eject_dg = Datagram::create();
        eject_dg->add_server_header(acc_channel, m_channel, CLIENTAGENT_EJECT);
        eject_dg->add_uint16(100);
        eject_dg->add_string("This account has been logged into elsewhere.");
        route_datagram(eject_dg);

        // Now we'll add ourselves to the account channel.
        subscribe_channel(acc_channel);

        // Unsubscribe from our current channel if it isn't our allocated channel.
        if(m_channel != m_allocated_channel) {
            unsubscribe_channel(m_channel);
        }

        // Set our sender channel to represent our account affiliation.
        m_channel = acc_channel;
        subscribe_channel(m_channel);

        // Set our client state to established.
        m_state = CLIENT_STATE_ESTABLISHED;

        // Calculate the current system time.
        auto end = chrono::system_clock::now();
        std::time_t end_time = chrono::system_clock::to_time_t(end);

        // Pack a new LAST_LOGIN field timestamp.
        DCPacker packer;
        DCClass *account = g_dcf->get_class_by_name("Account");
        DCField *login_field = account->get_field_by_name("LAST_LOGIN");
        packer.raw_pack_uint16(login_field->get_number());
        packer.begin_pack(login_field);
        packer.pack_string(ctime(&end_time));
        packer.end_pack();

        // Update the Account object's LAST_LOGIN field.
        DatagramPtr dg = Datagram::create();
        dg->add_server_header(m_database_id, m_channel, DBSERVER_OBJECT_SET_FIELD);
        dg->add_uint32(do_id);
        dg->add_data(packer.get_string());
        route_datagram(dg);

        // Finally, send a login response to the client.
        login_toontown_response(0, "", play_token, do_id);
    }

    // A DB object related to this Client has been created.
    virtual void handle_create_object_resp(DatagramIterator &dgi)
    {
        uint32_t context = dgi.read_uint32();
        doid_t do_id = dgi.read_uint32();

        lock_guard<recursive_mutex> lock(m_client_lock);

        if(m_token_contexts.find(context) == m_token_contexts.end() &&
           m_creation_av_contexts.find(context) == m_creation_av_contexts.end()) {
            // Not a context we're aware of...
            stringstream ss;
            ss << "Received an object creation response with an unknown context.";
            send_disconnect(CLIENT_DISCONNECT_MISSING_OBJECT, ss.str(), true);
            return;
        }

        if(m_token_contexts.find(context) != m_token_contexts.end()) {
            // This is for post-Account creation, so call that method.
            handle_account_created(context, do_id);
        }
        else if(m_creation_av_contexts.find(context) != m_creation_av_contexts.end()) {
            // This is for post-Avatar creation, so call that method.
            handle_avatar_created(context, do_id);
        }
    }

    // Client's Account object has been created.
    virtual void handle_account_created(uint32_t context, doid_t do_id)
    {
        // Grab the play token.
        string play_token = m_token_contexts[context];

        // Load our JSON file.
        ifstream acc_read(m_database_file);
        if(!acc_read) {
            // Uh oh, our account bridge doesn't exist or isn't valid.
            // We'll make a new one.
            json acc_bridge;

            // Store the play token and account ID in the new account bridge.
            acc_bridge[play_token] = to_string(do_id);

            // Close the read stream.
            acc_read.close();

            // Save the account bridge.
            fstream acc_write;
            acc_write.open(m_database_file, ios_base::out);
            acc_write << setw(4) << acc_bridge << endl;

            // Close the write stream.
            acc_write.close();
        }
        else {
            // Our account bridge exists, so let's read it.
            json acc_bridge = json::parse(acc_read);

            // Store the play token and account ID in the account bridge.
            acc_bridge[play_token] = to_string(do_id);

            // Close the read stream.
            acc_read.close();

            // Save the account bridge.
            fstream acc_write;
            acc_write.open(m_database_file, ios_base::out);
            acc_write << setw(4) << acc_bridge << endl;

            // Close the write stream.
            acc_write.close();
        }

        // Erase the token context.
        m_token_contexts.erase(context);

        // Load the account.
        login_account(play_token, do_id);
    }

    // Client's Avatar object has been created.
    virtual void handle_avatar_created(uint32_t context, doid_t do_id)
    {
        // Grab the information from the context map.
        uint16_t echo_context = m_creation_av_contexts[context].echo_context;
        string dna_string = m_creation_av_contexts[context].dna_string;
        uint8_t index = m_creation_av_contexts[context].index;
        set<uint32_t> av_set = m_creation_av_contexts[context].av_set;

        // Prepare to pack a new ACCOUNT_AV_SET field.
        DCPacker packer;
        DCClass *account = g_dcf->get_class_by_name("Account");
        DCField *field = account->get_field_by_name("ACCOUNT_AV_SET");
        packer.raw_pack_uint16(field->get_number());

        // First, pack the old Avatar set.
        packer.begin_pack(field);
        packer.push();

        // Loop over our Avatar set and pack each of the six IDs.
        for(uint16_t x{}; x < 6; ++x) {
            uint32_t av_id = *next(av_set.begin(), x);

            // Pack the AV ID.
            packer.pack_uint(av_id);
        }

        // Stop packing.
        packer.pop();
        packer.end_pack();

        // Now, pack the new one...
        packer.begin_pack(field);
        packer.push();

        // Loop over our Avatar set and pack each of the six IDs.
        set<uint32_t> new_av_set;
        for(uint16_t x{}; x < 6; ++x) {
            uint32_t av_id = *next(av_set.begin(), x);
            if(x == index) {
                // This is our index. Replace the old ID with the new one.
                av_id = (uint32_t)do_id;
            }

            // Pack the AV ID.
            packer.pack_uint(av_id);

            // Update the new Avatar set as well.
            new_av_set.insert(av_id);
        }

        // Update the Avatar set in the context struct.
        m_creation_av_contexts[context].av_set = new_av_set;

        // Stop packing.
        packer.pop();
        packer.end_pack();

        // Update the Account object's ACCOUNT_AV_SET field.
        DatagramPtr dg = Datagram::create();
        dg->add_server_header(m_database_id, m_channel, DBSERVER_OBJECT_SET_FIELD_IF_EQUALS);
        dg->add_uint32(context);
        dg->add_uint32(m_channel >> 32);
        dg->add_data(packer.get_string());
        route_datagram(dg);
    }

    // The ACCOUNT_AV_SET field was updated.
    virtual void handle_set_field_resp(DatagramIterator &dgi)
    {
        // Collect the context and success.
        uint32_t context = dgi.read_uint32();
        uint8_t success = dgi.read_uint8();

        // Check the context to make sure it's valid.
        if(m_creation_av_contexts.find(context) == m_creation_av_contexts.end()) {
            // We aren't aware of this context...
            stringstream ss;
            ss << "Received a set field response with an unknown context.";
            send_disconnect(CLIENT_DISCONNECT_MISSING_OBJECT, ss.str(), true);
            return;
        }

        // Grab the information from the context struct.
        uint16_t echo_context = m_creation_av_contexts[context].echo_context;
        string dna_string = m_creation_av_contexts[context].dna_string;
        uint8_t index = m_creation_av_contexts[context].index;
        set<uint32_t> av_set = m_creation_av_contexts[context].av_set;

        // Grab the AV ID from the AV set.
        uint32_t av_id = 0;
        for(uint16_t x{}; x < 6; ++x) {
            if(x == index) {
                av_id = *next(av_set.begin(), x);
            }
        }

        // Our return code should be zero by default.
        uint8_t return_code = 0;

        // Check if the set field succeeded.
        if(!success) {
            // The field was not set successfully. Our return code will be 1 to indicate failure.
            return_code = 1;
        }

        // Check if our AV ID is valid.
        if(!av_id) {
            // This should be impossible...
            return_code = 1;
        }

        // Erase the context.
        m_creation_av_contexts.erase(context);

        // Finally, send the avatar creation response to the client.
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_CREATE_AVATAR_RESP);
        resp->add_uint16(echo_context);
        resp->add_uint8(return_code);
        resp->add_uint32(av_id);
        m_client->send_datagram(resp);
    }

    virtual void login_toontown_response(uint8_t return_code, string return_str,
                                         string play_token, uint32_t do_id)
    {
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_LOGIN_TOONTOWN_RESP);
        resp->add_uint8(0);
        resp->add_string("");
        resp->add_uint32(do_id);
        resp->add_string(play_token);
        resp->add_uint8(1);
        resp->add_string("YES");
        resp->add_string("YES");
        resp->add_string("NULL");
        resp->add_uint32(time(0));
        resp->add_uint32(clock());
        resp->add_string("FULL");
        resp->add_string("YES");
        resp->add_string("-1");
        resp->add_int32(0);
        resp->add_string("NO_PARENT_ACCOUNT");
        resp->add_string(play_token);
        m_client->send_datagram(resp);
    }

    // Custom handling for internally received datagrams.
    virtual bool handle_cluster_datagram(DatagramHandle in_dg, DatagramIterator &dgi,
                                         channel_t sender, uint16_t msgtype)
    {
        switch(msgtype) {
        case DBSERVER_CREATE_OBJECT_RESP:
            handle_create_object_resp(dgi);
            break;
        case DBSERVER_OBJECT_GET_FIELD_RESP:
            handle_get_field_resp(dgi);
            break;
        case DBSERVER_OBJECT_SET_FIELD_IF_EQUALS_RESP:
            handle_set_field_resp(dgi);
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
        case CLIENT_LOGIN_TOONTOWN:
            handle_client_login_toontown(dgi);
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
        case CLIENT_SET_AVATAR:
            handle_client_set_avatar(dgi);
            break;
        case CLIENT_GET_FRIEND_LIST:
            break;
        case CLIENT_ADD_INTEREST:
            handle_client_add_interest(dgi, false);
            break;
        case CLIENT_REMOVE_INTEREST:
            handle_client_remove_interest(dgi);
            break;
        case CLIENT_CREATE_AVATAR:
            handle_client_create_avatar(dgi);
            break;
        case CLIENT_SET_NAME_PATTERN:
            handle_client_set_name_pattern(dgi);
            break;
        case CLIENT_SET_WISHNAME:
            handle_client_set_wishname(dgi);
            break;
        case CLIENT_GET_AVATARS:
            handle_client_get_avatars(dgi);
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

    // handle_client_get_avatars occurs when a client sends a CLIENT_GET_AVATARS.
    virtual void handle_client_get_avatars(DatagramIterator &dgi)
    {
        // Begin constructing a response datagram of avatars.
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_GET_AVATARS_RESP);

        // TODO: Add avatar creation.
        resp->add_uint8(0);
        resp->add_uint16(0);
        m_client->send_datagram(resp);
    }

    // handle_client_set_avatar occurs when a client sends a CLIENT_SET_AVATAR.
    virtual void handle_client_set_avatar(DatagramIterator &dgi)
    {
        // Get the Avatar ID.
        uint32_t av_id = dgi.read_uint32();

        // Activate the Avatar on the DBSS.
        DatagramPtr dg = Datagram::create();
        dg->add_server_header(av_id, m_channel, DBSS_OBJECT_ACTIVATE_WITH_DEFAULTS);
        dg->add_uint32(av_id);
        dg->add_uint32(0);
        dg->add_uint32(0);
        route_datagram(dg);

        DatagramPtr ldg = Datagram::create();
        ldg->add_server_header(av_id, m_channel, STATESERVER_OBJECT_SET_LOCATION);
        ldg->add_uint32(0);
        ldg->add_uint32(0);
        route_datagram(ldg);

        // Add the Avatar as a session object.
        m_session_objects.insert(av_id);

        // Set ourselves as the owner of the Avatar.
        DatagramPtr own_dg = Datagram::create();
        own_dg->add_server_header(av_id, m_channel, STATESERVER_OBJECT_SET_OWNER);
        own_dg->add_uint64(m_channel);
        route_datagram(own_dg);
    }

    // handle_client_set_name_pattern occurs when a client sends a CLIENT_SET_NAME_PATTERN.
    virtual void handle_client_set_name_pattern(DatagramIterator &dgi)
    {
        // Collect the information we need from the datagram.
        uint32_t av_id = dgi.read_uint32();
        int16_t first_index = dgi.read_int16();
        int16_t first_flag = dgi.read_int16();
        int16_t second_index = dgi.read_int16();
        int16_t second_flag = dgi.read_int16();
        int16_t third_index = dgi.read_int16();
        int16_t third_flag = dgi.read_int16();
        int16_t fourth_index = dgi.read_int16();
        int16_t fourth_flag = dgi.read_int16();

        // Send the response.
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_SET_NAME_PATTERN_ANSWER);
        resp->add_uint32(av_id);
        resp->add_uint8(0);
        m_client->send_datagram(resp);
    }

    // handle_client_set_wishname occurs when a client sends a CLIENT_SET_WISHNAME.
    virtual void handle_client_set_wishname(DatagramIterator &dgi)
    {
        // Collect the information we need from the datagram.
        uint32_t av_id = dgi.read_uint32();
        string wish_name = dgi.read_string();

        // Send the response.
        DatagramPtr resp = Datagram::create();
        resp->add_uint16(CLIENT_SET_WISHNAME_RESP);
        resp->add_uint32(av_id);
        resp->add_uint16(0);
        resp->add_string("");
        resp->add_string("Toon");
        resp->add_string("");
        m_client->send_datagram(resp);
    }

    // handle_client_create_avatar occurs when a client sends a CLIENT_CREATE_AVATAR.
    virtual void handle_client_create_avatar(DatagramIterator &dgi)
    {
        // Collect the information we need from the datagram.
        uint16_t echo_context = dgi.read_uint16();
        string dna_string = dgi.read_string();
        uint8_t index = dgi.read_uint8();

        // Before anything else, we need to query the Account through the DB server.
        // We will need to create a context for when we receive a response
        // from the DB server.
        uint32_t context = m_context++;
        m_create_av_contexts[context];
        m_create_av_contexts[context].echo_context = echo_context;
        m_create_av_contexts[context].dna_string = dna_string;
        m_create_av_contexts[context].index = index;

        // Grab the DC field for ACCOUNT_AV_SET, which contains the list of avatar IDs
        // belonging to this Account.
        DCClass *account = g_dcf->get_class_by_name("Account");
        DCField *field = account->get_field_by_name("ACCOUNT_AV_SET");

        // Now, query the Account for the ACCOUNT_AV_SET field.
        DatagramPtr dg = Datagram::create();
        dg->add_server_header(m_database_id, m_channel, DBSERVER_OBJECT_GET_FIELD);
        dg->add_uint32(context);
        dg->add_uint32(m_channel >> 32);
        dg->add_uint16(field->get_number());
        route_datagram(dg);
    }

    // handle_get_field_resp occurs when the DB server sends a DBSERVER_OBJECT_GET_FIELD_RESP.
    virtual void handle_get_field_resp(DatagramIterator &dgi)
    {
        // Collect the information the DB server sent us.
        uint32_t context = dgi.read_uint32();
        uint8_t success = dgi.read_uint8();

        // Make sure we're aware of this context.
        if(m_create_av_contexts.find(context) == m_create_av_contexts.end()) {
            // Not a context we're aware of...
            stringstream ss;
            ss << "Failed to fetch Avatar fields. (unknown context)";
            send_disconnect(CLIENT_DISCONNECT_MISSING_OBJECT, ss.str(), true);
            return;
        }

        if(!success) {
            // The DB server couldn't successfully fetch the fields. Give up.
            stringstream ss;
            ss << "Failed to fetch Avatar fields. (database failure)";
            send_disconnect(CLIENT_DISCONNECT_MISSING_OBJECT, ss.str(), false);
            return;
        }

        // Fetch the index from the context set.
        uint16_t echo_context = m_create_av_contexts[context].echo_context;
        string dna_string = m_create_av_contexts[context].dna_string;
        uint8_t index = m_create_av_contexts[context].index;

        // Now, unpack the ACCOUNT_AV_SET we were given using a DCPacker.
        DCPacker packer;
        DCClass *account = g_dcf->get_class_by_name("Account");
        DCClass *avatar = g_dcf->get_class_by_name("DistributedToon");
        packer.set_unpack_data(dgi.get_remaining_bytes());
        int16_t field_id = packer.raw_unpack_int16();
        DCField *field = account->get_field_by_index(field_id);
        packer.begin_unpack(field);

        // Fetch each Avatar index.
        set<uint32_t> av_set;
        for(uint16_t x{}; x < 6; ++x) {
            uint32_t av_id = packer.unpack_uint();
            if(av_id > 0 && x == index) {
                // The index which is wanted has already been taken...
                stringstream ss;
                ss << "Avatar index " << index << " has already been taken.";
                send_disconnect(CLIENT_DISCONNECT_FIELD_CONSTRAINT, ss.str(), false);
                return;
            }

            av_set.insert(av_id);
        }

        // We're done unpacking.
        packer.end_unpack();

        // Make a new DCPacker for the Avatar fields.
        DCPacker av_packer;

        // Get the Toon head color and animal type from the DNA string.
        uint8_t head_index = 0;
        uint8_t head_color = 0;
        try {
            DatagramPtr dna_dg = Datagram::create(dna_string);
            DatagramIterator dna_dgi = DatagramIterator(dna_dg);
            dna_dgi.get_fixed_string(1);
            uint8_t head_index = dna_dgi.read_uint8();
            dna_dgi.skip(sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) +
                         sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) +
                         sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) +
                         sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t));
            uint8_t head_color = dna_dgi.read_uint8();
        } catch(const DatagramIteratorEOF&) {
            // Our net string is not valid. Disconnect.
            stringstream ss;
            ss << "Received an invalid DNA net string.";
            send_disconnect(CLIENT_DISCONNECT_FIELD_CONSTRAINT, ss.str(), true);
            return;
        }
        string animal = "";
        string color = "";

        // Determine the animal.
        if(head_index < 4) {
            animal = " Dog";
        }
        else if(head_index < 8) {
            animal = " Cat";
        }
        else if(head_index < 12) {
            animal = " Horse";
        }
        else if(head_index < 14) {
            animal = " Mouse";
        }
        else if(head_index < 18) {
            animal = " Rabbit";
        }
        else if(head_index < 22) {
            animal = " Duck";
        }
        else if(head_index < 26) {
            animal = " Monkey";
        }
        else if(head_index < 30) {
            animal = " Bear";
        }
        else if(head_index > 30) {
            animal = " Pig";
        }

        // Determine the color.
        if(head_color == 0) {
            color = "White";
        }
        else if(head_color == 1) {
            color = "Peach";
        }
        else if(head_color == 2) {
            color = "Bright Red";
        }
        else if(head_color == 3) {
            color = "Red";
        }
        else if(head_color == 4) {
            color = "Maroon";
        }
        else if(head_color == 5) {
            color = "Sienna";
        }
        else if(head_color == 6) {
            color = "Brown";
        }
        else if(head_color == 7) {
            color = "Tan";
        }
        else if(head_color == 8) {
            color = "Coral";
        }
        else if(head_color == 9) {
            color = "Orange";
        }
        else if(head_color == 10) {
            color = "Yellow";
        }
        else if(head_color == 11) {
            color = "Cream";
        }
        else if(head_color == 12) {
            color = "Citrine";
        }
        else if(head_color == 13) {
            color = "Lime";
        }
        else if(head_color == 14) {
            color = "Sea Green";
        }
        else if(head_color == 15) {
            color = "Green";
        }
        else if(head_color == 16) {
            color = "Light Blue";
        }
        else if(head_color == 17) {
            color = "Aqua";
        }
        else if(head_color == 18) {
            color = "Blue";
        }
        else if(head_color == 19) {
            color = "Periwinkle";
        }
        else if(head_color == 20) {
            color = "Royal Blue";
        }
        else if(head_color == 21) {
            color = "Slate Blue";
        }
        else if(head_color == 22) {
            color = "Purple";
        }
        else if(head_color == 23) {
            color = "Lavender";
        }
        else if(head_color == 24) {
            color = "Pink";
        }
        else if(head_color == 25) {
            color = "Plum";
        }
        else if(head_color == 26) {
            color = "Black";
        }

        // Add the color and animal together to create the temporary name.
        string av_name = color + animal;

        // Make sure our name is valid.
        if(av_name.length() < 1) {
            stringstream ss;
            ss << "Received an invalid DNA net string (when setting the temporary name).";
            send_disconnect(CLIENT_DISCONNECT_FIELD_CONSTRAINT, ss.str(), true);
            return;
        }

        // Pack the default value for setName.
        DCField *name_field = avatar->get_field_by_name("setName");
        pack_string(av_packer, name_field, av_name);

        // Pack the default value for WishNameState.
        DCField *wish_state_field = avatar->get_field_by_name("WishNameState");
        pack_string(av_packer, wish_state_field, "OPEN");

        // Pack the default value for WishName.
        DCField *wish_name_field = avatar->get_field_by_name("WishName");
        pack_string(av_packer, wish_name_field, "");

        // Pack the DNA string.
        DCField *dna_field = avatar->get_field_by_name("setDNAString");
        pack_string(av_packer, dna_field, dna_string);

        // Pack the account ID.
        DCField *disl_field = avatar->get_field_by_name("setDISLid");
        pack_uint(av_packer, disl_field, m_channel >> 32);

        // Now, in comes an excessive amount of code.
        // Iterating over DB fields and packing their default value will
        // not work, as some fields require custom values.
        // We need to pack custom default values for each avatar field.
        pack_string(av_packer, avatar->get_field_by_name("setAccountName"), "");
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setFriendsList")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setFriendsList"));
        av_packer.push();
        av_packer.pack_default_value();
        av_packer.pop();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setPreviousAccess"), 0);
        pack_uint(av_packer, avatar->get_field_by_name("setGM"), 0);
        pack_int(av_packer, avatar->get_field_by_name("setMaxBankMoney"), 12000);
        pack_int(av_packer, avatar->get_field_by_name("setBankMoney"), 0);
        pack_int(av_packer, avatar->get_field_by_name("setMaxMoney"), 40);
        pack_int(av_packer, avatar->get_field_by_name("setMoney"), 0);
        pack_int(av_packer, avatar->get_field_by_name("setMaxHp"), 15);
        pack_int(av_packer, avatar->get_field_by_name("setHp"), 15);
        pack_string(av_packer, avatar->get_field_by_name("setExperience"), "\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00");
        pack_uint(av_packer, avatar->get_field_by_name("setMaxCarry"), 20);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setTrackAccess")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setTrackAccess"));
        av_packer.push();
        av_packer.push();
        av_packer.pack_uint(0);
        av_packer.pack_uint(0);
        av_packer.pack_uint(0);
        av_packer.pack_uint(0);
        av_packer.pack_uint(1);
        av_packer.pack_uint(1);
        av_packer.pack_uint(0);
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setTrackProgress")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setTrackProgress"));
        av_packer.push();
        av_packer.pack_int(-1);
        av_packer.pack_uint(0);
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setTrackBonusLevel")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setTrackBonusLevel"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 7; ++x) {
            av_packer.pack_int(-1);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        pack_string(av_packer, avatar->get_field_by_name("setInventory"), "\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00");
        pack_uint(av_packer, avatar->get_field_by_name("setMaxNPCFriends"), 16);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setNPCFriendsDict")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setNPCFriendsDict"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setDefaultShard"), 0);
        pack_uint(av_packer, avatar->get_field_by_name("setDefaultZone"), 0);
        pack_string(av_packer, avatar->get_field_by_name("setShtickerBook"), "");
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setZonesVisited")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setZonesVisited"));
        av_packer.push();
        av_packer.push();
        av_packer.pack_uint(2000);
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setHoodsVisited")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setHoodsVisited"));
        av_packer.push();
        av_packer.push();
        av_packer.pack_uint(2000);
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        pack_string(av_packer, avatar->get_field_by_name("setInterface"), "");
        pack_uint(av_packer, avatar->get_field_by_name("setLastHood"), 0);
        pack_uint(av_packer, avatar->get_field_by_name("setTutorialAck"), 1);
        pack_uint(av_packer, avatar->get_field_by_name("setMaxClothes"), 10);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setClothesTopsList")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setClothesTopsList"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setClothesBottomsList")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setClothesBottomsList"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setMaxAccessories"), 0);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setHatList")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setHatList"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setGlassesList")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setGlassesList"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setBackpackList")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setBackpackList"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setShoesList")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setShoesList"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setHat")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setHat"));
        av_packer.push();
        for(uint16_t x{}; x < 3; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setGlasses")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setGlasses"));
        av_packer.push();
        for(uint16_t x{}; x < 3; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setBackpack")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setBackpack"));
        av_packer.push();
        for(uint16_t x{}; x < 3; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setShoes")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setShoes"));
        av_packer.push();
        for(uint16_t x{}; x < 3; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setGardenSpecials")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setGardenSpecials"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setEmoteAccess")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setEmoteAccess"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 5; ++x) {
            av_packer.pack_uint(1);
        }
        for(uint16_t x{}; x < 15; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCustomMessages")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCustomMessages"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setResistanceMessages")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setResistanceMessages"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setPetTrickPhrases")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setPetTrickPhrases"));
        av_packer.push();
        av_packer.push();
        av_packer.pack_uint(0);
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCatalogSchedule")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCatalogSchedule"));
        av_packer.push();
        for(uint16_t x{}; x < 2; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCatalog")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCatalog"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        pack_string(av_packer, avatar->get_field_by_name("setMailboxContents"), "");
        pack_string(av_packer, avatar->get_field_by_name("setDeliverySchedule"), "");
        pack_string(av_packer, avatar->get_field_by_name("setGiftSchedule"), "");
        pack_string(av_packer, avatar->get_field_by_name("setAwardMailboxContents"), "");
        pack_string(av_packer, avatar->get_field_by_name("setAwardSchedule"), "");
        pack_uint(av_packer, avatar->get_field_by_name("setAwardNotify"), 0);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCatalogNotify")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCatalogNotify"));
        av_packer.push();
        av_packer.pack_uint(0);
        av_packer.pack_uint(0);
        av_packer.pop();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setSpeedChatStyleIndex"), 1);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setTeleportAccess")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setTeleportAccess"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCogStatus")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCogStatus"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 32; ++x) {
            av_packer.pack_uint(1);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCogCount")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCogCount"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 32; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCogRadar")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCogRadar"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 4; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setBuildingRadar")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setBuildingRadar"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 4; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCogLevels")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCogLevels"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 4; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCogTypes")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCogTypes"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 4; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCogParts")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCogParts"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 4; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCogMerits")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCogMerits"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 4; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setHouseId"), 0);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setQuests")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setQuests"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setQuestHistory")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setQuestHistory"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setRewardHistory")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setRewardHistory"));
        av_packer.push();
        av_packer.pack_uint(0);
        av_packer.push();
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setQuestCarryLimit"), 1);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCheesyEffect")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCheesyEffect"));
        av_packer.push();
        av_packer.pack_int(0);
        for(uint16_t x{}; x < 2; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setPosIndex"), 0);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setFishCollection")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setFishCollection"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setMaxFishTank"), 20);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setFishTank")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setFishTank"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setFishingRod"), 0);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setFishingTrophies")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setFishingTrophies"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setFlowerCollection")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setFlowerCollection"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setFlowerBasket")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setFlowerBasket"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setMaxFlowerBasket"), 20);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setGardenTrophies")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setGardenTrophies"));
        av_packer.pack_default_value();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setShovel"), 0);
        pack_uint(av_packer, avatar->get_field_by_name("setShovelSkill"), 0);
        pack_uint(av_packer, avatar->get_field_by_name("setWateringCan"), 0);
        pack_uint(av_packer, avatar->get_field_by_name("setWateringCanSkill"), 0);
        pack_uint(av_packer, avatar->get_field_by_name("setPetId"), 0);
        pack_uint(av_packer, avatar->get_field_by_name("setPetTutorialDone"), 0);
        pack_uint(av_packer, avatar->get_field_by_name("setFishBingoTutorialDone"), 0);
        pack_uint(av_packer, avatar->get_field_by_name("setFishBingoMarkTutorialDone"), 0);
        pack_int(av_packer, avatar->get_field_by_name("setKartBodyType"), -1);
        pack_int(av_packer, avatar->get_field_by_name("setKartBodyColor"), -1);
        pack_int(av_packer, avatar->get_field_by_name("setKartAccessoryColor"), -1);
        pack_int(av_packer, avatar->get_field_by_name("setKartEngineBlockType"), -1);
        pack_int(av_packer, avatar->get_field_by_name("setKartSpoilerType"), -1);
        pack_int(av_packer, avatar->get_field_by_name("setKartFrontWheelWellType"), -1);
        pack_int(av_packer, avatar->get_field_by_name("setKartBackWheelWellType"), -1);
        pack_int(av_packer, avatar->get_field_by_name("setKartRimType"), -1);
        pack_int(av_packer, avatar->get_field_by_name("setKartDecalType"), -1);
        pack_uint(av_packer, avatar->get_field_by_name("setTickets"), 200);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setKartingHistory")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setKartingHistory"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 16; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setKartingTrophies")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setKartingTrophies"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 33; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setKartingPersonalBest")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setKartingPersonalBest"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 6; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setKartingPersonalBest2")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setKartingPersonalBest2"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 12; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setKartAccessoriesOwned")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setKartAccessoriesOwned"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 16; ++x) {
            av_packer.pack_int(-1);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setCogSummonsEarned")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setCogSummonsEarned"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 32; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setGardenStarted"), 0);
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setGolfHistory")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setGolfHistory"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 18; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setPackedGolfHoleBest")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setPackedGolfHoleBest"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 18; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        av_packer.raw_pack_uint16(avatar->get_field_by_name("setGolfCourseBest")->get_number());
        av_packer.begin_pack(avatar->get_field_by_name("setGolfCourseBest"));
        av_packer.push();
        av_packer.push();
        for(uint16_t x{}; x < 3; ++x) {
            av_packer.pack_uint(0);
        }
        av_packer.pop();
        av_packer.pop();
        av_packer.end_pack();
        pack_uint(av_packer, avatar->get_field_by_name("setPinkSlips"), 0);
        pack_uint(av_packer, avatar->get_field_by_name("setNametagStyle"), 0);

        // Create a new context.
        uint32_t new_context = m_context++;
        m_creation_av_contexts[new_context];
        m_creation_av_contexts[new_context].echo_context = echo_context;
        m_creation_av_contexts[new_context].dna_string = dna_string;
        m_creation_av_contexts[new_context].index = index;
        m_creation_av_contexts[new_context].av_set = av_set;

        // Erase the old context.
        m_create_av_contexts.erase(context);

        // Create the Avatar object.
        DatagramPtr dg = Datagram::create();
        dg->add_server_header(m_database_id, m_channel, DBSERVER_CREATE_OBJECT);
        dg->add_uint32(new_context);
        dg->add_uint16(avatar->get_number());
        dg->add_uint16(112);
        dg->add_data(av_packer.get_string());
        route_datagram(dg);
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

        // If the class couldn't be found, error out:
        if(!dcc) {
            if(is_historical_object(do_id)) {
                // The client isn't disconnected in this case because it could be a delayed
                // message, we also have to skip to the end so a disconnect overside_datagram
                // is not sent.
                // TODO: Allow configuration to limit how long historical objects remain,
                //       for example with a timeout or bad-message limit.
                dgi.skip(dgi.get_remaining());
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
