#pragma once
#include "Client.h"
#include "dclass/dcPacker.h"
#include "dclass/dcClass.h"
#include "dclass/dcField.h"
#include "core/global.h"
#include "json/json.hpp"

class OTPClientManager; // forward declaration
class DisneyClient;

// PotentialAvatar contains the data needed by the client for avatar choosing.
struct PotentialAvatar
{
    uint32_t av_id = 0;
    std::string name = "";
    std::string dna_string = "";
    uint8_t index = 0;
};

// AvatarBasicInfo contains the basic info for an avatar.
struct AvatarBasicInfo
{
    uint32_t av_id;
    std::string name;
    std::string dna_string;
    uint32_t pet_id;
};

// AvatarBasicInfoCache contains an avatar's basic info and an expiration time.
struct AvatarBasicInfoCache
{
    time_t expire;
    AvatarBasicInfo info;
};

// Operator is a base class for any class defined here.
class Operator
{
  public:
    OTPClientManager *m_manager;
    DisneyClient& m_client;

    Operator(OTPClientManager *manager, DisneyClient& client);
    virtual ~Operator();

    virtual void handle_create(uint32_t ctx, uint32_t do_id);
    virtual void handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id);
    virtual void handle_update(uint32_t ctx, uint8_t success);
    virtual void handle_lookup(bool success, uint32_t account_id, std::string play_token);
    virtual void get_activated_resp(uint32_t do_id, uint32_t ctx, bool activated);
    virtual void friend_callback(bool success = 0, uint32_t av_id = 0,
                                 nlohmann::json &fields = nlohmann::json({}),
                                 DatagramHandle dg = Datagram::create(), bool is_pet = 0,
                                 std::vector<AvatarBasicInfo> friend_details = std::vector<AvatarBasicInfo>{},
                                 std::vector<uint32_t> online_friends = std::vector<uint32_t>{},
                                 bool online = 0);

  protected:

    INLINE int64_t get_puppet_connection_channel(uint32_t do_id)
    {
        return do_id + ((int64_t)1001L << 32);
    }

    INLINE int64_t get_account_connection_channel(uint32_t do_id)
    {
        return do_id + ((int64_t)1003L << 32);
    }

    // Packs a particular JSON object into a DCPacker.
    void pack_json_object(DCPacker &packer, nlohmann::json &object);

    // Packs all given fields in a JSON object into a DCPacker.
    void pack_json_objects(DCPacker &packer, DCClass *dclass, nlohmann::json &object, bool raw = 1);

    // Unpacks a particular element in a DCPacker into a JSON object.
    void unpack_json_object(DCPacker &unpacker, std::string field_name,
                            std::string index, nlohmann::json &object);

    // Unpacks all packed fields in a DCPacker into a JSON object.
    nlohmann::json unpack_json_objects(DatagramIterator &dgi, DCClass *dclass, size_t field_count, int16_t set_field_id = 0);

    // Creates an object in the specified database.
    // database_id specifies the control channel of the target database.
    // dclass specifies the class of the object to be created.
    // fields is a JSON object containing all field names by their values.
    void create_object(uint32_t database_id, DCClass *dclass, nlohmann::json &fields);

    // Query object `do_id` out of the database to retrieve the dclass & fields.
    void query_object(uint32_t database_id, uint32_t do_id);

    // Update field(s) on an object, optionally with the requirement that the
    // fields must match some old value.
    void update_object(uint32_t database_id, uint32_t do_id, DCClass *dclass, nlohmann::json &new_fields, nlohmann::json &old_fields);

    // Checks if a database object has been activated.
    void get_activated(uint32_t do_id);

    // Dispatches a field update to an object.
    void send_update(uint32_t do_id, DCClass *dclass, DCField *field, nlohmann::json &values);

    void warning(std::string text);

    std::vector<unsigned int> m_contexts;
    std::map<uint32_t, uint32_t> m_context_id;

  private:
    unsigned int get_context();

    static unsigned int m_context;
};

// AccountDB is a base class for account database interface implementations.
class AccountDB
{
  public:
    AccountDB(OTPClientManager *manager, std::string database_file);
    virtual ~AccountDB();

    virtual void lookup(Operator *op, std::string play_token) = 0;
    void store_account_id(std::string database_id, uint32_t account_id);

  protected:
    OTPClientManager *m_manager;
    nlohmann::json m_bridge;
    std::string m_database_file;
};

// DeveloperAccountDB is an account database interface implementation meant
// for local usage (not for production).
class DeveloperAccountDB : virtual public AccountDB
{
  public:
    DeveloperAccountDB(OTPClientManager *manager, std::string database_file);
    virtual ~DeveloperAccountDB();

    INLINE std::string get_wish_name_status(uint32_t av_id)
    {
        return "APPROVED"; // Default to approved, this really only needs to be implemented for production databases.
    };

    void lookup(Operator *op, std::string play_token);
};

// TODO: Add MongoAccountDB

// GameOperation is the base class for all other operations used by the OTPClientManager.
class GameOperation : virtual public Operator
{
  public:
    GameOperation(OTPClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~GameOperation();

    // Deletes the target from either connection2operation or account2operation
    // depending on whether our target is a connection or account.
    void off();

    // Kills either the target connection or the target account
    // depending on whether our target is a connection or account,
    // and then turns off the operation.
    void kill(std::string reason);
    void kill();

  protected:
    channel_t m_target;
    bool m_target_connection;
};

// LoginOperation is the operation which manages account creation & loading.
class LoginOperation : virtual public GameOperation
{
  public:
    LoginOperation(OTPClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~LoginOperation();

    // Sets m_play_token, then queries the account database.
    void start(std::string play_token, std::string version, uint32_t dc_hash, int32_t token_type, std::string want_magic_words);

    // Calls the lookup function on the defined account DB interface.
    void query_account_db();

    // This is a callback method that will be called by the lookup function
    // of the account database interface. It processes the lookup function's
    // result & determines which operation should run next.
    void handle_lookup(bool success, uint32_t account_id, std::string play_token);

    // Creates a brand new account & stores it in the database.
    void create_account();

    // This function handles successful & unsuccessful account creations.
    void handle_create(uint32_t ctx, uint32_t do_id);

    // Query the database object associated with the account ID.
    void retrieve_account();

    // Checks if the queried object is valid and if it is, sets the account.
    // Otherwise, the connection is killed.
    void handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id);

    // Associates the account with the client and establishes the client.
    void set_account();

    // Sends a response back to the client with the necessary login information.
    virtual void login_response(uint8_t return_code, std::string return_str,
                                std::string play_token, uint32_t do_id);

  protected:
    std::string m_play_token;
    uint32_t m_account_id;
    bool m_creating;
    std::string m_version;
    std::string m_dc_hash;
    int32_t m_token_type;
    std::string m_want_magic_words;
};

// An AvatarOperation is the base class for any operation which manages an avatar.
class AvatarOperation : virtual public GameOperation
{
  public:
    AvatarOperation(OTPClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~AvatarOperation();

    // Queries the account.
    void retrieve_account();

    // Meant to be inherited. Receives the dclass & fields from a query.
    virtual void handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id);

    // Meant to be inherited. Handles any operations that must occur post-query.
    virtual void post_account_func();

  protected:
    bool m_past_acc_query;
    nlohmann::json m_account;
    std::vector<uint32_t> m_av_set;

    void handle_retrieve(DCClass *dclass);
};

// A GetAvatarsOperation queries the avatars on a target account and collects each potential avatar.
class GetAvatarsOperation : virtual public AvatarOperation
{
  public:
    GetAvatarsOperation(OTPClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~GetAvatarsOperation();

    // Retrieves the account.
    virtual void start();

    // Queries each avatar on the account.
    virtual void post_account_func();

    // Collects important potential avatar fields from an avatar object.
    void handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id);

    // Here is where we'll fetch a list of potential avatars and send them to the client.
    void send_avatars();

  protected:
    std::vector<uint32_t> m_pending_avatars;
    std::map<uint32_t, nlohmann::json> m_avatar_fields;
    uint8_t m_av_amount;
};

// A CreateAvatarOperation takes DNA & index and creates an avatar in the database.
class CreateAvatarOperation : virtual public GameOperation
{
  public:
    CreateAvatarOperation(OTPClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~CreateAvatarOperation();

    // Stores the index & DNA values.
    void start(std::string dna_string, uint8_t index);

    // Queries the account.
    void retrieve_account();

    // Prepares the account for creation.
    void handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id);

    // Creates the avatar in the database (must be inherited).
    virtual void create_avatar();

    // This function handles successful & unsuccessful avatar creation.
    void handle_create(uint32_t ctx, uint32_t do_id);

    // Stores the avatar in the Account object.
    void store_avatar();

    // Sends the avatar creation response to the client.
    void handle_update(uint32_t ctx, uint8_t success);

  protected:
    uint8_t m_index;
    uint32_t m_av_id;
    std::string m_dna_string;
    std::vector<uint32_t> m_av_set;
    nlohmann::json m_account;
};

// AcknowledgeNameOperation updates the wish name, name state, and name of an avatar.
class AcknowledgeNameOperation : virtual public AvatarOperation
{
  public:
    AcknowledgeNameOperation(OTPClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~AcknowledgeNameOperation();

    // Stores the avatar id and queries the account.
    void start(uint32_t av_id);

    // Queries the avatar in the database.
    void post_account_func();

    // Updates the avatar's name fields and sends a name response to the client.
    void handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id);

  protected:
    uint32_t m_av_id;
};

// RemoveAvatarOperation deletes an avatar from an account in the database.
// Inherits from GetAvatarsOperation as the avatar list is needed after the operation.
class RemoveAvatarOperation : virtual public GetAvatarsOperation
{
  public:
    RemoveAvatarOperation(OTPClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~RemoveAvatarOperation();

    // Stores the avatar id and queries the account.
    void start(uint32_t av_id);

    // Deletes the avatar from the database.
    void post_account_func();

    // Finishes up the removal operation and runs the normal avatar list operation.
    void handle_update(uint32_t ctx, uint8_t success);

  protected:
    uint32_t m_av_id;
};

// LoadAvatarOperation activates an avatar on the DBSS & launches it into the game.
class LoadAvatarOperation : virtual public AvatarOperation
{
  public:
    LoadAvatarOperation(OTPClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~LoadAvatarOperation();

    // Stores the avatar ID and retrives the account.
    void start(uint32_t av_id);

    // Queries the avatar.
    void post_account_func();

    // Stores the avatar unpacker.
    void handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id);

    // Activates the avatar on the DBSS & associates it with the client.
    void set_avatar();

  protected:
    uint32_t m_av_id;
    nlohmann::json m_avatar;
};

// UnloadAvatarOperation cleanly unloads the avatar by removing the object
// and closing anything associated with it.
class UnloadAvatarOperation : virtual public GameOperation
{
  public:
    UnloadAvatarOperation(OTPClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~UnloadAvatarOperation();

    // Stores the avatar ID.
    void start(uint32_t av_id);

    // Cleanly unloads the avatar from the game.
    void unload_avatar();

  protected:
    uint32_t m_av_id;
};

// An OTPClientManager is a class created for each Client which
// handles numerous client functions such as avatar creation & loading,
// avatar name setting, account creation, etc.
class OTPClientManager
{
  public:
    DCClass* m_player_class;
    uint32_t m_database_id;
    uint8_t m_valid_index;

    // The purpose of connection2operation & account2operation are to
    // keep track of the connection & account IDs that are
    // currently running an operation on the OTPClientManager.
    // Ideally, this will help us prevent clients from running
    // more than one operation at a time which could potentially
    // lead to race conditions & the exploitation of them.
    std::map<channel_t, GameOperation*> m_connection2operation;
    std::map<uint32_t, GameOperation*> m_account2operation;

    // Keep an AccountDB pointer around.
    AccountDB *m_account_db;

    // The client needs to be able to get the Operator pointer from the context:
    std::map<uint32_t, Operator*> m_context_operator;

    OTPClientManager(DCClass *player_class, uint32_t database_id, uint8_t valid_index,
                     std::string database_type, std::string database_file);

    // Gathers together PotentialAvatar structs through packed fields & an avatar set. Must be inherited.
    virtual std::vector<PotentialAvatar> get_potential_avatars(std::map<uint32_t, nlohmann::json> packed_fields, std::vector<uint32_t> av_set);

    // Judges whether a name is valid or not.
    virtual bool judge_name(std::string name);

    // Forms name patterns into a full name string. Must be inherited.
    virtual std::string create_name(std::vector<std::pair<int16_t, uint8_t> > patterns);

    // Runs a LoginOperation.
    virtual void login(DisneyClient& client, std::string play_token, channel_t sender, std::string version,
                       uint32_t dc_hash, int32_t token_type, std::string want_magic_words);

    // Ejects the client with the given reason.
    void kill_connection(channel_t connection_id, std::string reason);

    // Kills the connection for duplicate operations.
    void kill_connection_operation(channel_t connection_id);

    // Kills the account's connection given an account_id & a reason.
    void kill_account(uint32_t account_id, std::string reason);

    // Kills the account for duplicate operations.
    void kill_account_operation(uint32_t account_id);

    // Runs an operation on the sender. Returns true/false for success.
    bool run_operation(GameOperation* operation_type, uint32_t sender);

    // Runs a GetAvatarsOperation.
    void request_avatar_list(DisneyClient& client, uint32_t sender);

    // Runs a CreateAvatarOperation.
    virtual void create_avatar(DisneyClient& client, uint32_t sender,
                               std::string dna_string, uint8_t index);

    // Runs an AcknowledgeNameOperation.
    void acknowledge_avatar_name(DisneyClient& client, uint32_t sender, uint32_t av_id);

    // Runs a RemoveAvatarOperation.
    void request_remove_avatar(DisneyClient& client, uint32_t sender, uint32_t av_id);

    // Runs either a LoadAvatarOperation or an UnloadAvatarOperation.
    void request_play_avatar(DisneyClient& client, uint32_t sender,
                             uint32_t av_id, uint32_t curr_av_id);
};