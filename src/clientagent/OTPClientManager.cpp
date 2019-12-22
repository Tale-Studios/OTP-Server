#include "OTPClientManager.h"
#include "DisneyClient.cpp"
#include "ClientMessages.h"
#include "core/global.h"
#include "core/msgtypes.h"
#include "json/json.hpp"

using namespace std;
using json = nlohmann::json;

unsigned int Operator::m_context = 0;

Operator::Operator(OTPClientManager *manager, DisneyClient& client) : m_manager(manager), m_client(client)
{
}

Operator::~Operator()
{
}

void Operator::create_object(uint32_t database_id, DCClass *dclass, DCPacker &packer, uint16_t field_count)
{
    // Save the context:
    unsigned int ctx = get_context();
    m_contexts.push_back(ctx);
    m_manager->m_context_operator[ctx] = this;

    // Now generate and send the datagram:
    DatagramPtr dg = Datagram::create();
    dg->add_server_header(m_manager->m_database_id, m_client.get_client_channel(), DBSERVER_CREATE_OBJECT);
    dg->add_uint32(ctx);
    dg->add_uint16(dclass->get_number());
    dg->add_uint16(field_count);
    dg->add_data(packer.get_string());
    m_client.dispatch_datagram(dg);
}

void Operator::handle_create(uint32_t ctx, uint32_t do_id)
{
    if(find(m_contexts.begin(), m_contexts.end(), ctx) == m_contexts.end()) {
        warning(string("Received unexpected DBSERVER_CREATE_OBJECT_RESP (ctx ") + to_string(ctx) + ", do_id " + to_string(do_id) + ")");
        return;
    }

    m_contexts.erase(remove(m_contexts.begin(), m_contexts.end(), ctx), m_contexts.end());
    m_manager->m_context_operator.erase(ctx);
}

void Operator::query_object(uint32_t database_id, uint32_t do_id)
{
    // Save the context:
    unsigned int ctx = get_context();
    m_contexts.push_back(ctx);
    m_context_id[ctx] = do_id;
    m_manager->m_context_operator[ctx] = this;

    // Now generate and send the datagram:
    DatagramPtr dg = Datagram::create();
    dg->add_server_header(m_manager->m_database_id, m_client.get_client_channel(), DBSERVER_OBJECT_GET_ALL);
    dg->add_uint32(ctx);
    dg->add_uint32(do_id);
    m_client.dispatch_datagram(dg);
}

void Operator::handle_query(uint32_t ctx, uint16_t dclass_id, DCPacker &unpacker)
{
    if(find(m_contexts.begin(), m_contexts.end(), ctx) == m_contexts.end()) {
        warning(string("Received unexpected DBSERVER_OBJECT_GET_ALL_RESP (ctx ") + to_string(ctx) + ")");
        return;
    }

    m_contexts.erase(remove(m_contexts.begin(), m_contexts.end(), ctx), m_contexts.end());
    m_manager->m_context_operator.erase(ctx);
}

void Operator::update_object(uint32_t database_id, uint32_t do_id, vector<DCField*> fields, vector<DCPacker> new_fields, vector<DCPacker> old_fields)
{
    // Make sure if we have anything in our old fields vector the size is the same as the new fields vector:
    if(new_fields.size() != old_fields.size() && old_fields.size() != 0) {
        warning("Update object call was given mismatching new/old fields!");
        return;
    }

    DCPacker field_packer;
    uint8_t field_count = 0;
    for(DCField* f : fields) {
        ++field_count;

        field_packer.raw_pack_uint16(f->get_number());

        if(old_fields.size() >= field_count) {
            DCPacker old_field = old_fields[field_count];

            // Pack the old field:
            field_packer.append_data(old_field.get_data(), old_field.get_length());
        }

        DCPacker new_field = new_fields[field_count];

        field_packer.append_data(new_field.get_data(), new_field.get_length());
    }

    // Generate and send the datagram:
    DatagramPtr dg = Datagram::create();
    if(old_fields.size() > 0) {
        unsigned int ctx = get_context();
        m_contexts.push_back(ctx);
        m_manager->m_context_operator[ctx] = this;
        if(field_count == 1) {
            dg->add_server_header(m_manager->m_database_id, m_client.get_client_channel(), DBSERVER_OBJECT_SET_FIELD_IF_EQUALS);
        } else {
            dg->add_server_header(m_manager->m_database_id, m_client.get_client_channel(), DBSERVER_OBJECT_SET_FIELDS_IF_EQUALS);
        }

        dg->add_uint32(ctx);
    } else {
        if(field_count == 1) {
            dg->add_server_header(m_manager->m_database_id, m_client.get_client_channel(), DBSERVER_OBJECT_SET_FIELD);
        } else {
            dg->add_server_header(m_manager->m_database_id, m_client.get_client_channel(), DBSERVER_OBJECT_SET_FIELDS);
        }
    }

    dg->add_uint32(do_id);
    if(field_count != 1) {
        dg->add_uint16(field_count);
    }
    dg->add_data(field_packer.get_string());
    m_client.dispatch_datagram(dg);
}

void Operator::handle_update(uint32_t ctx, uint8_t success)
{
    if(find(m_contexts.begin(), m_contexts.end(), ctx) == m_contexts.end()) {
        warning(string("Received unexpected DBSERVER_OBJECT_SET_FIELD(S)_IF_EQUALS_RESP (ctx ") + to_string(ctx) + ")");
        return;
    }

    m_contexts.erase(remove(m_contexts.begin(), m_contexts.end(), ctx), m_contexts.end());
    m_manager->m_context_operator.erase(ctx);
}

void Operator::handle_lookup(bool success, uint32_t account_id, string play_token)
{
    // Must be handled by inheritor.
}

void Operator::warning(string text)
{
    m_client.log_warning(text);
}

unsigned int Operator::get_context()
{
    Operator::m_context = (Operator::m_context + 1) & 0xFFFFFFFF;
    return Operator::m_context;
}

AccountDB::AccountDB(OTPClientManager *manager, string database_file) :
    m_manager(manager), m_database_file(database_file)
{
    // We're using JSON, so open the bridge file:
    ifstream bridge_file(database_file);
    if(bridge_file.good()) {
        // JSON parse our bridge file.
        m_bridge = json::parse(bridge_file);
    } else {
        // Huh, our bridge file doesn't exist or is corrupt.
        // Let's make a new one. First, open a write stream.
        bridge_file.close();
        fstream write;
        write.open(database_file, ios_base::out | ios_base::trunc);

        // Make an empty JSON bridge.
        m_bridge = json({});

        // Write the empty bridge to the file.
        write << setw(4) << m_bridge << endl;
    }
}

AccountDB::~AccountDB()
{
}

void AccountDB::store_account_id(string database_id, uint32_t account_id)
{
    // Store the account id under the database id in our JSON bridge.
    m_bridge[database_id] = to_string(account_id);

    // Open a write stream to save the bridge to file.
    fstream write;
    write.open(m_database_file, ios_base::out | ios_base::trunc);

    // Write the bridge to the file.
    write << setw(4) << m_bridge << endl;
}

DeveloperAccountDB::DeveloperAccountDB(OTPClientManager *manager, string database_file) :
    AccountDB(manager, database_file)
{
}

DeveloperAccountDB::~DeveloperAccountDB()
{
}

void DeveloperAccountDB::lookup(Operator *op, string play_token)
{
    // Check if this play token exists in the bridge:
    if(m_bridge.find(play_token) == m_bridge.end()) {
        // It does not, so we'll associate them with a brand new account object.
        op->handle_lookup(true, 0, play_token);
    } else {
        string strid = m_bridge[play_token].get<string>();
        stringstream ss(strid);
        uint32_t do_id;
        ss >> do_id;
        op->handle_lookup(true, do_id, play_token);
    }
}

GameOperation::GameOperation(OTPClientManager *manager, DisneyClient& client, channel_t target) :
    Operator(manager, client), m_target(target), m_target_connection(false)
{
}

GameOperation::~GameOperation()
{
}

void GameOperation::off()
{
    if(m_target_connection) {
        m_manager->m_connection2operation.erase(m_target);
    } else {
        m_manager->m_account2operation.erase((uint32_t)m_target);
    }
}

void GameOperation::kill(string reason)
{
    if(m_target_connection) {
        m_manager->kill_connection(m_target, reason);
    } else {
        m_manager->kill_account((uint32_t)m_target, reason);
    }

    off();
}

void GameOperation::kill()
{
    kill("");
}

LoginOperation::LoginOperation(OTPClientManager *manager, DisneyClient& client, channel_t target) :
    GameOperation(manager, client, target), Operator(manager, client),
    m_creating(false), m_account_id(0)
{
}

LoginOperation::~LoginOperation()
{
}

void LoginOperation::start(string play_token, string version, uint32_t dc_hash, int32_t token_type, string want_magic_words)
{
    m_play_token = play_token;
    m_version = version;
    m_dc_hash = dc_hash;
    m_token_type = token_type;
    m_want_magic_words = want_magic_words;

    // Before we start anything, make sure our hash & version match up.
    string server_version = m_client.get_server_version();
    uint32_t server_hash = m_client.get_dc_hash();

    if(version != server_version) {
        // Bad server version compare. Send a login failure response & shut down this operation.
        stringstream ss;
        ss << "Bad Server Version Compare: client=" << version;
        if(m_client.get_send_version()) {
            ss << ", server=" << server_version;
        }
        login_response(1, ss.str(), play_token, 0);
        off();
        return;
    }

    if(dc_hash != server_hash) {
        // Bad DC hash compare. Send a login failure response & shut down this operation.
        stringstream ss;
        ss << "Bad DC Version Compare: client=" << dc_hash;
        if(m_client.get_send_hash()) {
            ss << ", server=" << server_hash;
        }
        login_response(2, ss.str(), play_token, 0);
        off();
        return;
    }

    query_account_db();
}

void LoginOperation::query_account_db()
{
    m_manager->m_account_db->lookup(this, m_play_token);
}

void LoginOperation::handle_lookup(bool success, uint32_t account_id, string play_token)
{
    if(!success) {
        // The play token was rejected! Kill the connection.
        LoggedEvent event("play-token-rejected");
        event.add("play_token", play_token);
        m_client.write_server_event(event);
        kill("The accounts database rejected your login cookie.");
        return;
    }

    // Make sure the play token is the same as what was given.
    m_play_token = play_token;

    if(account_id > 0) {
        // There is an account ID, so let's retrieve the associated account.
        m_account_id = account_id;
        retrieve_account();
    } else {
        // There is no account ID, so let's create a new account.
        m_creating = true;
        create_account();
    }
}

void LoginOperation::create_account()
{
    DCPacker packer;
    DCClass *account = g_dcf->get_class_by_name("Account");
    DCField *av_set_field = account->get_field_by_name("ACCOUNT_AV_SET");

    // Pack ACCOUNT_AV_SET.
    pack_int_field(packer, av_set_field, vector<uint32_t> {0, 0, 0, 0, 0, 0}, true);

    // Pack ACCOUNT_AV_SET_DEL.
    DCField *av_del_field = account->get_field_by_name("ACCOUNT_AV_SET_DEL");
    pack_default_field(packer, av_del_field);

    // Pack ESTATE_ID.
    DCField *estate_field = account->get_field_by_name("ESTATE_ID");
    pack_int_field(packer, estate_field, vector<uint32_t> {0}, false);

    // Calculate the current system time.
    auto end = chrono::system_clock::now();
    time_t end_time = chrono::system_clock::to_time_t(end);

    // Pack CREATED.
    DCField *created_field = account->get_field_by_name("CREATED");
    pack_string_field(packer, created_field, vector<string> {ctime(&end_time)}, false);

    // Pack LAST_LOGIN.
    DCField *login_field = account->get_field_by_name("LAST_LOGIN");
    pack_string_field(packer, created_field, vector<string> {ctime(&end_time)}, false);

    // Create the Account object with the packed fields.
    create_object(m_manager->m_database_id, account, packer, 5);
}

void LoginOperation::handle_create(uint32_t ctx, uint32_t do_id)
{
    Operator::handle_create(ctx, do_id);

    if(!m_creating) {
        // If we're not supposed to be creating an account, then this is invalid.
        warning("Received CreateAccount response when not creating an account.");
        return;
    }

    if(!do_id) {
        // If we don't have an account id, then that means the database was unable
        // to create an account object for us, for whatever reason. Kill the connection.
        warning("Database failed to create an account object!");
        kill("Your account object could not be created in the game database.");
        return;
    }

    // Otherwise, the account object was created successfully!
    LoggedEvent event("account-created");
    event.add("account_id", to_string(do_id));
    m_client.write_server_event(event);

    // We can now store the account ID.
    m_account_id = do_id;
    m_manager->m_account_db->store_account_id(m_play_token, m_account_id);

    // Finally, we can set the account.
    set_account();
}

void LoginOperation::retrieve_account()
{
    query_object(m_manager->m_database_id, m_account_id);
}

void LoginOperation::handle_query(uint32_t ctx, uint16_t dclass_id, DCPacker &unpacker)
{
    Operator::handle_query(ctx, dclass_id, unpacker);

    DCClass *account = g_dcf->get_class_by_name("Account");
    if(account->get_number() != dclass_id) {
        // This is not an account object! Kill the connection.
        kill("Your account object was not found in the database!");
        return;
    }

    // We can now set the account.
    set_account();
}

void LoginOperation::set_account()
{
    // If somebody's already logged into this account, disconnect them.
    DatagramPtr eject_dg = Datagram::create();
    eject_dg->add_server_header(get_account_connection_channel(m_account_id), 0, CLIENTAGENT_EJECT);
    eject_dg->add_uint16(100);
    eject_dg->add_string("This account has been logged into elsewhere.");
    m_client.dispatch_datagram(eject_dg);

    // Now we'll add this connection to the account channel.
    m_client.subscribe_to_channel(get_account_connection_channel(m_account_id));

    // Set their sender channel to represent their account affiliation.
    m_client.set_client_channel((uint64_t)m_account_id << 32); // accountId in high 32 bits, 0 in low (no avatar).
    m_client.subscribe_to_channel(m_client.get_client_channel());

    // We can now un-sandbox the sender.
    m_client.set_client_state(CLIENT_STATE_ESTABLISHED);

    // Calculate the current system time.
    auto end = chrono::system_clock::now();
    time_t end_time = chrono::system_clock::to_time_t(end);

    // Update the last login timestamp.
    DCPacker packer;
    DCClass *account = g_dcf->get_class_by_name("Account");
    DCField *login_field = account->get_field_by_name("LAST_LOGIN");
    pack_string_field(packer, login_field, vector<string> {ctime(&end_time)}, false);

    // Set the field.
    DatagramPtr dg = Datagram::create();
    dg->add_server_header(m_manager->m_database_id, m_client.get_client_channel(), DBSERVER_OBJECT_SET_FIELD);
    dg->add_uint32(m_account_id);
    dg->add_data(packer.get_string());
    m_client.dispatch_datagram(dg);

    // We're done.
    LoggedEvent event("account-login");
    event.add("client_id", to_string(m_target));
    event.add("acc_id", to_string(m_account_id));
    event.add("play_token", m_play_token);
    m_client.write_server_event(event);

    // Send a login response back to the client.
    login_response(0, "", m_play_token, m_account_id);
}

void LoginOperation::login_response(uint8_t return_code, string return_str,
                                    string play_token, uint32_t do_id)
{
    // Send the login response to the client.
    DatagramPtr resp = Datagram::create();
    resp->add_uint16(CLIENT_LOGIN_2_RESP);
    resp->add_uint8(return_code);
    resp->add_string(return_str);
    resp->add_string(play_token);
    resp->add_uint8(1);
    resp->add_uint32((uint32_t)time(0));
    resp->add_uint32(clock());
    resp->add_uint8(1);
    resp->add_string("");
    resp->add_string("YES");
    m_client.forward_datagram(resp);

    // Shut down this operation.
    off();
}

AvatarOperation::AvatarOperation(OTPClientManager *manager, DisneyClient& client, channel_t target) :
    GameOperation(manager, client, target), Operator(manager, client),
    m_past_acc_query(false)
{
}

AvatarOperation::~AvatarOperation()
{
}

void AvatarOperation::retrieve_account()
{
    query_object(m_manager->m_database_id, (uint32_t)m_target);
}

void AvatarOperation::handle_query(uint32_t ctx, uint16_t dclass_id, DCPacker &unpacker)
{
    Operator::handle_query(ctx, dclass_id, unpacker);

    if(m_past_acc_query) {
        // We have already done this.
        return;
    }

    DCClass *account = g_dcf->get_class_by_name("Account");
    if(account->get_number() != dclass_id) {
        // This is not an account object! Kill the connection.
        kill("Your account object was not found in the database!");
        return;
    }

    // Save the account field unpacker.
    m_field_unpacker = unpacker;

    // Unpack ACCOUNT_AV_SET.
    DCField *av_field = account->get_field_by_name("ACCOUNT_AV_SET");
    m_av_set = unpack_int_field(m_field_unpacker, av_field, 6);

    // We're done; run the post account function.
    m_past_acc_query = true;
    post_account_func();
}

void AvatarOperation::post_account_func()
{
}

GetAvatarsOperation::GetAvatarsOperation(OTPClientManager *manager, DisneyClient& client, channel_t target) :
    AvatarOperation(manager, client, target), GameOperation(manager, client, target),
    Operator(manager, client), m_av_amount(0)
{
}

GetAvatarsOperation::~GetAvatarsOperation()
{
}

void GetAvatarsOperation::start()
{
    // First, retrieve the account.
    retrieve_account();
}

void GetAvatarsOperation::post_account_func()
{
    // Loop through the list of avatars:
    for(uint32_t av_id : m_av_set) {
        if(av_id > 0) {
            // This index contains an avatar! Add it to the pending avatars.
            m_pending_avatars.push_back(av_id);
            m_av_amount++;

            // Query the avatar object.
            query_object(m_manager->m_database_id, av_id);
        }
    }

    if(m_pending_avatars.size() < 1) {
        // No pending avatars! Send the avatar list.
        send_avatars();
    }
}

void GetAvatarsOperation::handle_query(uint32_t ctx, uint16_t dclass_id, DCPacker &unpacker)
{
    AvatarOperation::handle_query(ctx, dclass_id, unpacker);

    if(find(m_contexts.begin(), m_contexts.end(), ctx) == m_contexts.end()) {
        // We don't know this context.
        return;
    }

    uint32_t av_id = m_context_id[ctx];
    m_context_id.erase(ctx);

    if(m_manager->m_player_class->get_number() != dclass_id) {
        // The dclass is invalid! Kill the connection.
        kill(string("One of the account's avatars is invalid! dclass = ") + to_string(dclass_id) + ", expected = " + to_string(m_manager->m_player_class->get_number()));
        return;
    }

    // Otherwise, we're all set!
    // Store the unpacker, remove the avatar from the pending list,
    // and send the avatar list.
    m_packed_fields[av_id] = unpacker;
    m_pending_avatars.erase(remove(m_pending_avatars.begin(), m_pending_avatars.end(), av_id), m_pending_avatars.end());
    if(m_pending_avatars.size() < 1) {
        send_avatars();
    }
}

void GetAvatarsOperation::send_avatars()
{
    // Get PotentialAvatar structs from the client manager's potential avatar function.
    vector<PotentialAvatar> potential_avatars = m_manager->get_potential_avatars(m_packed_fields, m_av_set);

    // We're done; begin constructing a response datagram of potential avatars,
    // and then we can shut down this operation.
    DatagramPtr resp = Datagram::create();
    resp->add_uint16(CLIENT_GET_AVATARS_RESP);
    resp->add_uint8(0);
    resp->add_uint16(m_av_amount);

    for(int i=0; i<m_av_amount; ++i) {
        resp->add_uint32(potential_avatars[i].av_id);
        resp->add_string(potential_avatars[i].name);
        resp->add_string("");
        resp->add_string("");
        resp->add_string("");
        resp->add_string(potential_avatars[i].dna_string);
        resp->add_uint8(potential_avatars[i].index);
        resp->add_uint8(0);
    }

    m_client.forward_datagram(resp);

    // Shut down the operation.
    off();
}

CreateAvatarOperation::CreateAvatarOperation(OTPClientManager *manager, DisneyClient& client, channel_t target) :
    GameOperation(manager, client, target), Operator(manager, client),
    m_index(0), m_av_id(0)
{
}

CreateAvatarOperation::~CreateAvatarOperation()
{
}

void CreateAvatarOperation::start(string dna_string, uint8_t index)
{
    // First, perform some basic sanity checking.
    if(index >= m_manager->m_valid_index) {
        kill("Invalid index specified!");
        return;
    }

    // Store these values.
    m_index = index;
    m_dna_string = dna_string;

    // Now we can query their account.
    retrieve_account();
}

void CreateAvatarOperation::retrieve_account()
{
    query_object(m_manager->m_database_id, (uint32_t)m_target);
}

void CreateAvatarOperation::handle_query(uint32_t ctx, uint16_t dclass_id, DCPacker &unpacker)
{
    Operator::handle_query(ctx, dclass_id, unpacker);

    DCClass *account = g_dcf->get_class_by_name("Account");
    if(account->get_number() != dclass_id) {
        // This is not an account object! Kill the connection.
        kill("Your account object was not found in the database!");
        return;
    }

    // Save the account field unpacker.
    m_field_unpacker = unpacker;

    // Unpack ACCOUNT_AV_SET.
    DCField *av_field = account->get_field_by_name("ACCOUNT_AV_SET");
    m_av_set = unpack_int_field(m_field_unpacker, av_field, 6);

    // Check if the index is open:
    if(m_av_set[m_index] > 0) {
        // This index is not open! Kill the connection.
        kill("This avatar slot is already taken by another avatar!");
        return;
    }

    // All set, now let's create the avatar!
    create_avatar();
}

void CreateAvatarOperation::create_avatar()
{
    // Must be inherited.
}

void CreateAvatarOperation::handle_create(uint32_t ctx, uint32_t do_id)
{
    Operator::handle_create(ctx, do_id);

    if(do_id < 1) {
        // The database was unable to create a new avatar object! Kill the connection.
        kill("Database failed to create the new avatar object!");
        return;
    }

    // We can now store the avatar.
    m_av_id = do_id;
    store_avatar();
}

void CreateAvatarOperation::store_avatar()
{
    // We will now associate the avatar with the account.
    vector<uint32_t> old_av_set(m_av_set);
    m_av_set[m_index] = m_av_id;

    DCClass *account = g_dcf->get_class_by_name("Account");
    DCField *field = account->get_field_by_name("ACCOUNT_AV_SET");

    vector<DCField*> fields {field};
    vector<DCPacker> new_fields;
    vector<DCPacker> old_fields;

    DCPacker new_packer;
    DCPacker old_packer;

    pack_int_field(new_packer, field, m_av_set, true, false);
    pack_int_field(old_packer, field, old_av_set, true, false);

    new_fields.push_back(new_packer);
    old_fields.push_back(old_packer);

    // Update the object.
    update_object(m_manager->m_database_id, (uint32_t)m_target, fields, new_fields, old_fields);
}

void CreateAvatarOperation::handle_update(uint32_t ctx, uint8_t success)
{
    Operator::handle_update(ctx, success);

    if(!success) {
        // The new avatar was not associated with the account! Kill the connection.
        kill("Database failed to associate the new avatar to your account!");
        return;
    }

    // Otherwise, we're done! We can now send the avatar creation response back to the client.
    // First, log the avatar creation event.
    LoggedEvent event("avatar-created");
    event.add("av_id", to_string(m_av_id));
    event.add("target", to_string(m_target));
    event.add("dna", m_dna_string);
    event.add("index", to_string(m_index));
    m_client.write_server_event(event);

    // Send the response to the client.
    DatagramPtr resp = Datagram::create();
    resp->add_uint16(CLIENT_CREATE_AVATAR_RESP);
    resp->add_uint16(0);
    resp->add_uint8(0);
    resp->add_uint32(m_av_id);
    m_client.forward_datagram(resp);

    // Finally, shut down the operation.
    off();
}

AcknowledgeNameOperation::AcknowledgeNameOperation(OTPClientManager *manager, DisneyClient& client, channel_t target) :
    AvatarOperation(manager, client, target), GameOperation(manager, client, target),
    Operator(manager, client), m_av_id(0)
{
}

AcknowledgeNameOperation::~AcknowledgeNameOperation()
{
}

void AcknowledgeNameOperation::start(uint32_t av_id)
{
    // Store this value & move on to retrieve account.
    m_av_id = av_id;
    retrieve_account();
}

void AcknowledgeNameOperation::post_account_func()
{
    // Make sure that the target avatar is part of the account:
    if(find(m_av_set.begin(), m_av_set.end(), m_av_id) == m_av_set.end()) {
        // The sender tried to acknowledge name on an avatar not on the account!
        // Kill the connection.
        kill("Tried to acknowledge name on an avatar not in the account!");
        return;
    }

    // We can now query the database for the avatar.
    query_object(m_manager->m_database_id, m_av_id);
}

void AcknowledgeNameOperation::handle_query(uint32_t ctx, uint16_t dclass_id, DCPacker &unpacker)
{
    AvatarOperation::handle_query(ctx, dclass_id, unpacker);

    if(m_manager->m_player_class->get_number() != dclass_id) {
        // This dclass is not a valid avatar! Kill the connection.
        kill("One of the account's avatars is invalid!");
        return;
    }

    // Process the WishNameState change.
    DCField *wish_name_state_field = m_manager->m_player_class->get_field_by_name("WishNameState");
    string wish_name_state = unpack_string_field(unpacker, wish_name_state_field, 1).front();
    DCField *wish_name_field = m_manager->m_player_class->get_field_by_name("WishName");
    string wish_name = unpack_string_field(unpacker, wish_name_field, 1).front();
    DCField *set_name_field = m_manager->m_player_class->get_field_by_name("setName");
    string name = unpack_string_field(unpacker, set_name_field, 1).front();

    string old_wish_name_state = wish_name_state;
    string old_wish_name = wish_name;
    string old_name = name;

    if(wish_name_state == "APPROVED") {
        wish_name_state = "LOCKED";
        name = wish_name;
        wish_name = "";
    } else if(wish_name_state == "REJECTED") {
        wish_name_state = "OPEN";
        wish_name = "";
    } else {
        // The sender is trying to acknowledge name on avatar in an invalid state! Kill the connection.
        kill(string("Tried to acknowledge name on an avatar in invalid state (") + wish_name_state + ") !");
        return;
    }

    // We can now update the avatar object in the database with the changes!
    vector<DCField*> fields {wish_name_state_field, wish_name_field, set_name_field};
    vector<DCPacker> new_fields;
    vector<DCPacker> old_fields;

    DCPacker new_wish_name_state_packer;
    DCPacker new_wish_name_packer;
    DCPacker new_name_packer;
    DCPacker old_wish_name_state_packer;
    DCPacker old_wish_name_packer;
    DCPacker old_name_packer;

    vector<string> nwns {wish_name_state};
    vector<string> nwn {wish_name};
    vector<string> nn {name};
    vector<string> owns {old_wish_name_state};
    vector<string> own {old_wish_name};
    vector<string> on {old_name};

    pack_string_field(new_wish_name_state_packer, wish_name_state_field, nwns, false, false);
    pack_string_field(new_wish_name_packer, wish_name_field, nwn, false, false);
    pack_string_field(new_name_packer, set_name_field, nn, false, false);
    pack_string_field(old_wish_name_state_packer, wish_name_state_field, owns, false, false);
    pack_string_field(old_wish_name_packer, wish_name_field, own, false, false);
    pack_string_field(old_name_packer, set_name_field, on, false, false);

    new_fields.push_back(new_wish_name_state_packer);
    new_fields.push_back(new_wish_name_packer);
    new_fields.push_back(new_name_packer);
    old_fields.push_back(old_wish_name_state_packer);
    old_fields.push_back(old_wish_name_packer);
    old_fields.push_back(old_name_packer);

    // Update the object.
    update_object(m_manager->m_database_id, m_av_id, fields, new_fields, old_fields);

    // We're done. We can now shut down this operation.
    off();
}

RemoveAvatarOperation::RemoveAvatarOperation(OTPClientManager *manager, DisneyClient& client, channel_t target) :
    GetAvatarsOperation(manager, client, target), AvatarOperation(manager, client, target),
    GameOperation(manager, client, target), Operator(manager, client), m_av_id(0)
{
}

RemoveAvatarOperation::~RemoveAvatarOperation()
{
}

void RemoveAvatarOperation::start(uint32_t av_id)
{
    // Store this value & call the base function.
    m_av_id = av_id;
    GetAvatarsOperation::start();
}

void RemoveAvatarOperation::post_account_func()
{
    // Make sure that the target avatar is part of the account:
    if(find(m_av_set.begin(), m_av_set.end(), m_av_id) == m_av_set.end()) {
        // The sender tried to remove an avatar not on the account! Kill the connection.
        kill("Tried to remove an avatar not on the account!");
        return;
    }

    // Get the index of this avatar.
    vector<uint32_t>::iterator itr = find(m_av_set.begin(), m_av_set.end(), m_av_id);
    uint8_t index = distance(m_av_set.begin(), itr);
    vector<uint32_t> old_av_set(m_av_set);
    m_av_set[index] = 0;

    // Calculate the current system time.
    auto end = chrono::system_clock::now();
    time_t end_time = chrono::system_clock::to_time_t(end);

    // We will now add this avatar to ACCOUNT_AV_SET_DEL.
    DCClass *account = g_dcf->get_class_by_name("Account");
    DCField *av_set_field = account->get_field_by_name("ACCOUNT_AV_SET");
    DCField *av_del_field = account->get_field_by_name("ACCOUNT_AV_SET_DEL");
    vector<uint32_t> avs_removed = unpack_int_field(m_field_unpacker, av_del_field, 0);
    vector<uint32_t> old_avs_removed(avs_removed);
    avs_removed.push_back(m_av_id);
    avs_removed.push_back((uint32_t)ctime(&end_time));

    // Get the estate ID of this account.
    uint32_t estate_id = unpack_int_field(m_field_unpacker, account->get_field_by_name("ESTATE_ID"), 1).front();

    if(estate_id != 0) {
        // The following will assume that the house already exists,
        // however it shouldn't be a problem if it doesn't.
        DCClass *estate = g_dcf->get_class_by_name("DistributedEstate");
        DCField *slot_toon_id_field = estate->get_field_by_name(string("setSlot") + to_string(index) + "ToonId");
        DCField *slot_items_field = estate->get_field_by_name(string("setSlot") + to_string(index) + "Items");
        vector<DCField*> fields {slot_toon_id_field, slot_items_field};
        vector<DCPacker> new_fields;

        DCPacker toon_id_packer;
        DCPacker items_packer;

        pack_default_field(toon_id_packer, slot_toon_id_field, false);
        pack_default_field(items_packer, slot_items_field, false);

        new_fields.push_back(toon_id_packer);
        new_fields.push_back(items_packer);

        update_object(m_manager->m_database_id, estate_id, fields, new_fields, vector<DCPacker>{});
    }

    // We can now update the account with the new data.
    vector<DCField*> fields {av_set_field, av_del_field};
    vector<DCPacker> new_fields;
    vector<DCPacker> old_fields;

    DCPacker new_set_packer;
    DCPacker new_del_packer;
    DCPacker old_set_packer;
    DCPacker old_del_packer;

    pack_int_field(new_set_packer, av_set_field, m_av_set, true, false);
    pack_int_field(new_del_packer, av_del_field, avs_removed, true, false);
    pack_int_field(old_set_packer, av_set_field, old_av_set, true, false);
    pack_int_field(old_del_packer, av_del_field, old_avs_removed, true, false);

    new_fields.push_back(new_set_packer);
    new_fields.push_back(new_del_packer);
    old_fields.push_back(old_set_packer);
    old_fields.push_back(old_del_packer);

    // Update the account.
    update_object(m_manager->m_database_id, (uint32_t)m_target, fields, new_fields, old_fields);
}

void RemoveAvatarOperation::handle_update(uint32_t ctx, uint8_t success)
{
    GetAvatarsOperation::handle_update(ctx, success);

    if(!success) {
        // The avatar was unable to be removed from the account! Kill the account.
        kill("Database failed to mark the avatar as removed!");
        return;
    }

    // Otherwise, we're done! We can now move on with the normal GetAvatarsOperation
    // and retrieve the avatar list, which sends the user back to the avatar chooser.
    LoggedEvent event("avatar-deleted");
    event.add("av_id", to_string(m_av_id));
    event.add("target", to_string(m_target));
    m_client.write_server_event(event);
    GetAvatarsOperation::post_account_func();
}

LoadAvatarOperation::LoadAvatarOperation(OTPClientManager *manager, DisneyClient& client, channel_t target) :
    AvatarOperation(manager, client, target), GameOperation(manager, client, target),
    Operator(manager, client), m_av_id(0)
{
}

LoadAvatarOperation::~LoadAvatarOperation()
{
}

void LoadAvatarOperation::start(uint32_t av_id)
{
    // Store this value & retrieve the account.
    m_av_id = av_id;
    retrieve_account();
}

void LoadAvatarOperation::post_account_func()
{
    // Make sure that the target avatar is part of the account:
    if(find(m_av_set.begin(), m_av_set.end(), m_av_id) == m_av_set.end()) {
        // The sender tried to play on an avatar not on the account! Kill the connection.
        kill("Tried to play on an avatar not on the account!");
        return;
    }

    // Query the database for the avatar.
    query_object(m_manager->m_database_id, m_av_id);
}

void LoadAvatarOperation::handle_query(uint32_t ctx, uint16_t dclass_id, DCPacker &unpacker)
{
    AvatarOperation::handle_query(ctx, dclass_id, unpacker);

    if(m_manager->m_player_class->get_number() != dclass_id) {
        // This dclass is not a valid avatar! Kill the connection.
        kill("One of the account's avatars is invalid!");
        return;
    }

    // Store the avatar & move on to setting the avatar.
    m_av_unpacker = unpacker;
    set_avatar();
}

void LoadAvatarOperation::set_avatar()
{
    // Get the client channel.
    channel_t channel = get_account_connection_channel((uint32_t)m_target);

    // We will first assign a POST_REMOVE that will unload the
    // avatar in the event of them disconnecting while we are working.
    DatagramPtr cleanup_datagram = Datagram::create();
    cleanup_datagram->add_server_header(m_av_id, channel, STATESERVER_OBJECT_DELETE_RAM);
    cleanup_datagram->add_uint32(m_av_id);
    m_client.create_post_remove(m_client.get_allocated_channel(), cleanup_datagram);

    // We will now activate the avatar on the DBSS.
    DatagramPtr dg = Datagram::create();
    dg->add_server_header(m_av_id, m_client.get_client_channel(), DBSS_OBJECT_ACTIVATE_WITH_DEFAULTS);
    dg->add_uint32(m_av_id);
    dg->add_uint32(0);
    dg->add_uint32(0);
    m_client.dispatch_datagram(dg);

    DatagramPtr ldg = Datagram::create();
    ldg->add_server_header(m_av_id, m_client.get_client_channel(), STATESERVER_OBJECT_SET_LOCATION);
    ldg->add_uint32(0);
    ldg->add_uint32(0);
    m_client.dispatch_datagram(ldg);

    // Next, we will add them to the avatar channel.
    m_client.subscribe_to_channel(get_puppet_connection_channel(m_av_id));

    // We will now set the avatar as the client's session object.
    DatagramPtr sdg = Datagram::create();
    sdg->add_server_header(channel, m_client.get_client_channel(), CLIENTAGENT_ADD_SESSION_OBJECT);
    sdg->add_uint32(m_av_id);
    m_client.dispatch_datagram(sdg);

    // Now we need to set their sender channel to represent their account affiliation.
    if(m_client.get_client_channel() != m_client.get_allocated_channel()) {
        m_client.unsubscribe_from_channel(m_client.get_client_channel());
    }

    m_client.set_client_channel(m_target << 32 | m_av_id); // accountId in high 32 bits, 0 in low (no avatar).
    m_client.subscribe_to_channel(m_client.get_client_channel());

    // We can now finally grant ownership.
    DatagramPtr odg = Datagram::create();
    odg->add_server_header(m_av_id, m_client.get_client_channel(), STATESERVER_OBJECT_SET_OWNER);
    odg->add_uint64(channel);
    m_client.dispatch_datagram(odg);

    // Finally, shut down this operation.
    LoggedEvent event("avatar-chosen");
    event.add("av_id", to_string(m_av_id));
    event.add("acc_id", to_string(m_target));
    m_client.write_server_event(event);
    off();
}

UnloadAvatarOperation::UnloadAvatarOperation(OTPClientManager *manager, DisneyClient& client, channel_t target) :
    GameOperation(manager, client, target), Operator(manager, client),
    m_av_id(0)
{
}

UnloadAvatarOperation::~UnloadAvatarOperation()
{
}

void UnloadAvatarOperation::start(uint32_t av_id)
{
    // Store the av id.
    m_av_id = av_id;

    // We actually don't even need to query the account, as we know
    // that the avatar is being played, so let's just unload the avatar.
    unload_avatar();
}

void UnloadAvatarOperation::unload_avatar()
{
    // Get the client channel.
    channel_t channel = get_account_connection_channel((uint32_t)m_target);

    // First, remove our post removes.
    m_client.wipe_post_removes(m_client.get_allocated_channel());

    // Next, remove the avatar channel.
    m_client.unsubscribe_from_channel(get_puppet_connection_channel(m_av_id));

    // Next, remove the sender channel.
    if(m_client.get_client_channel() != m_client.get_allocated_channel()) {
        m_client.unsubscribe_from_channel(m_client.get_client_channel());
    }

    m_client.set_client_channel(m_target << 32); // accountId in high 32 bits, no avatar in low.
    m_client.subscribe_to_channel(m_client.get_client_channel());

    // Reset the session object.
    DatagramPtr sdg = Datagram::create();
    sdg->add_server_header(channel, m_client.get_client_channel(), CLIENTAGENT_ADD_SESSION_OBJECT);
    sdg->add_uint32(m_av_id);
    m_client.dispatch_datagram(sdg);

    // Unload the avatar object.
    DatagramPtr dg = Datagram::create();
    dg->add_server_header(m_av_id, channel, STATESERVER_OBJECT_DELETE_RAM);
    dg->add_uint32(m_av_id);
    m_client.dispatch_datagram(dg);

    // We're done! We can now shut down this operation.
    LoggedEvent event("avatar-unloaded");
    event.add("av_id", to_string(m_av_id));
    m_client.write_server_event(event);
    off();
}

OTPClientManager::OTPClientManager(DCClass* player_class, uint32_t database_id, uint8_t valid_index,
                                   string database_type, string database_file) :
    m_player_class(player_class), m_database_id(database_id), m_valid_index(valid_index)
{
    // Instantiate the account database backend.
    // TODO: Add more database interfaces (mongo!) & make this configurable.
    if(database_type == "developer") {
        m_account_db = new DeveloperAccountDB(this, database_file);
    }
}

vector<PotentialAvatar> OTPClientManager::get_potential_avatars(map<uint32_t, DCPacker> packed_fields, vector<uint32_t> av_set)
{
    vector<PotentialAvatar> potential_avatars;
    return potential_avatars;
}

bool OTPClientManager::judge_name(string name)
{
    return true; // TODO: Make this useful.
}

string OTPClientManager::create_name(vector<pair<int16_t, uint8_t> > patterns)
{
    return "";
}

void OTPClientManager::login(DisneyClient& client, string play_token, channel_t sender, string version,
                             uint32_t dc_hash, int32_t token_type, string want_magic_words)
{
    if(sender >> 32) {
        // This account is already logged in.
        kill_connection(sender, "This account is already logged in.");
        return;
    }

    if(m_connection2operation.find(sender) != m_connection2operation.end()) {
        // This account is already currently running an operation. Kill this connection.
        kill_connection_operation(sender);
        return;
    }

    // Run the login operation.
    LoginOperation* op = new LoginOperation(this, client, sender);
    m_connection2operation[sender] = op;
    op->start(play_token, version, dc_hash, token_type, want_magic_words);
}

void OTPClientManager::kill_connection(channel_t connection_id, string reason)
{
    if(m_connection2operation.find(connection_id) != m_connection2operation.end()) {
        m_connection2operation[connection_id]->m_client.send_disconnect(122, reason);
    }
}

void OTPClientManager::kill_connection_operation(channel_t connection_id)
{
    if(m_connection2operation.find(connection_id) != m_connection2operation.end()) {
        kill_connection(connection_id, string("An operation is already running."));
    }
}

void OTPClientManager::kill_account(uint32_t account_id, string reason)
{
    kill_connection(account_id + ((int64_t)1003L << 32), reason);
}

void OTPClientManager::kill_account_operation(uint32_t account_id)
{
    if(m_account2operation.find(account_id) != m_account2operation.end()) {
        kill_account(account_id, string("An operation is already running."));
    }
}

bool OTPClientManager::run_operation(GameOperation* operation_type, uint32_t sender)
{
    // First, check the sender.
    if(sender < 1) {
        // If the sender doesn't exist, they're not
        // logged in, so kill the connection.
        kill_account(sender, "Client is not logged in.");
        return false;
    }

    if(m_account2operation.find(sender) != m_account2operation.end()) {
        // This account is already currently running an operation. Kill this connection.
        kill_account_operation(sender);
        return false;
    }

    m_account2operation[sender] = operation_type;

    return true;
}

void OTPClientManager::request_avatar_list(DisneyClient& client, uint32_t sender)
{
    GetAvatarsOperation* operation = new GetAvatarsOperation(this, client, sender);
    bool success = run_operation(operation, sender);
    if(success) {
        operation->start();
    }
}

void OTPClientManager::create_avatar(DisneyClient& client, uint32_t sender, string dna_string, uint8_t index)
{
    CreateAvatarOperation* operation = new CreateAvatarOperation(this, client, sender);
    bool success = run_operation(operation, sender);
    if(success) {
        operation->start(dna_string, index);
    }
}

void OTPClientManager::acknowledge_avatar_name(DisneyClient& client, uint32_t av_id)
{
    AcknowledgeNameOperation* operation = new AcknowledgeNameOperation(this, client, av_id);
    bool success = run_operation(operation, av_id);
    if(success) {
        operation->start(av_id);
    }
}

void OTPClientManager::request_remove_avatar(DisneyClient& client, uint32_t av_id)
{
    RemoveAvatarOperation* operation = new RemoveAvatarOperation(this, client, av_id);
    bool success = run_operation(operation, av_id);
    if(success) {
        operation->start(av_id);
    }
}

void OTPClientManager::request_play_avatar(DisneyClient& client, uint32_t sender, uint32_t av_id, bool unload)
{
    if(!unload) {
        // If unload is not set, then we're running a LoadAvatarOperation.
        LoadAvatarOperation* operation = new LoadAvatarOperation(this, client, sender);
        bool success = run_operation(operation, sender);
        if(success) {
            operation->start(av_id);
        }
    } else {
        // Otherwise, the client wants to unload the avatar; run an UnloadAvatarOperation.
        UnloadAvatarOperation* operation = new UnloadAvatarOperation(this, client, sender);
        bool success = run_operation(operation, sender);
        if(success) {
            operation->start(av_id);
        }
    }
}