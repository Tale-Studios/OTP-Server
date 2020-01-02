#pragma once
#include "OTPClientManager.h"
#include "dclass/dcPacker.h"
#include "dclass/dcClass.h"
#include "dclass/dcField.h"

class NameGenerator
{
  public:
    NameGenerator(std::string name_file);
    ~NameGenerator();

    std::string make_name(std::vector<std::pair<int16_t, uint8_t> > patterns);

    INLINE std::string lstrip(std::string s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !::isspace(ch);
        }));

        return s;
    }

    INLINE std::string join(const std::vector<std::string>& elements, const char* const separator)
    {
        switch (elements.size())
        {
            case 0:
                return "";
            case 1:
                return elements[0];
            default:
                std::ostringstream os;
                std::copy(elements.begin(), elements.end() - 1, std::ostream_iterator<std::string>(os, separator));
                os << *elements.rbegin();
                return os.str();
        }
    }

  protected:
    std::map<int16_t, std::pair<int, std::string> > m_name_map;
};

class ToontownClientManager; // forward declaration

class ToontownLoginOperation : virtual public LoginOperation
{
  public:
    ToontownLoginOperation(ToontownClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~ToontownLoginOperation();

    void login_response(uint8_t return_code, std::string return_str,
                        std::string play_token, uint32_t do_id);
};

class ToontownCreateAvatarOperation : virtual public CreateAvatarOperation
{
  public:
    ToontownCreateAvatarOperation(ToontownClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~ToontownCreateAvatarOperation();

    // Method for packing an int into a field.
    INLINE void pack_int(DCPacker &packer, DCField *field, int value)
    {
        packer.raw_pack_uint16(field->get_number());
        packer.begin_pack(field);
        packer.push();
        packer.pack_int(value);
        packer.pop();
        packer.end_pack();
    }

    // Method for packing a uint into a field.
    INLINE void pack_uint(DCPacker &packer, DCField *field, unsigned int value)
    {
        packer.raw_pack_uint16(field->get_number());
        packer.begin_pack(field);
        packer.push();
        packer.pack_uint(value);
        packer.pop();
        packer.end_pack();
    }

    // Method for packing a string into a field.
    INLINE void pack_string(DCPacker &packer, DCField *field, const string value)
    {
        packer.raw_pack_uint16(field->get_number());
        packer.begin_pack(field);
        packer.push();
        packer.pack_string(value);
        packer.pop();
        packer.end_pack();
    }

    // Creates the DistributedToon object with the default values.
    void create_avatar();
};

class SetNameTypedOperation : virtual public AvatarOperation
{
  public:
    SetNameTypedOperation(ToontownClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~SetNameTypedOperation();

    // Stores the avatar id and name. Either jumps to judging the name,
    // or retrieves the account first.
    void start(uint32_t av_id, std::string name);

    // Retrieves the avatar from the database.
    void post_account_func();

    // Verifies the toon's dclass & fields, then judges the name.
    void handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id);

    // Judges the name, updates the toon name fields, and sends a response.
    void judge_name();

  protected:
    uint32_t m_av_id;
    std::string m_av_name;
};

class SetNamePatternOperation : virtual public AvatarOperation
{
  public:
    SetNamePatternOperation(ToontownClientManager *manager, DisneyClient& client, channel_t target);
    virtual ~SetNamePatternOperation();

    // Stores the avatar id and pattern then retrieves the account.
    void start(uint32_t av_id, int16_t p1, uint8_t f1, int16_t p2, uint8_t f2,
               int16_t p3, uint8_t f3, int16_t p4, uint8_t f4);

    // Retrieves the avatar from the database.
    void post_account_func();

    // Verifies the toon's dclass & fields, then sets the name.
    void handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id);

    // Puts together the pattern into a name, updates the toon name fields, and sends a response.
    void set_name();

  protected:
    uint32_t m_av_id;
    int16_t m_p1;
    uint8_t m_f1;
    int16_t m_p2;
    uint8_t m_f2;
    int16_t m_p3;
    uint8_t m_f3;
    int16_t m_p4;
    uint8_t m_f4;
};

class GetAvatarInfoOperation : virtual public GameOperation
{
  public:
    GetAvatarInfoOperation(ToontownClientManager *manager, DisneyClient& client, Operator *op,
                           uint32_t target, uint32_t sender_av_id, uint32_t av_id);
    virtual ~GetAvatarInfoOperation();

    // Starts the operation.
    void start();

    // Queries the avatar (or pet) fields.
    void get_avatar_info();

    // Collects & verifies the fields.
    void handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id);

    // Finishes the operation.
    void finished();

    // Finishes the operation with a failure.
    void failure(std::string reason);

  protected:
    ToontownClientManager *m_ttmgr;
    Operator *m_op;
    uint32_t m_sender_av_id;
    uint32_t m_av_id;
    bool m_is_pet;
    nlohmann::json m_fields;
};

class GetFriendsListOperation : virtual public GameOperation
{
  public:
    GetFriendsListOperation(ToontownClientManager *manager, DisneyClient& client,
                            Operator *op, uint32_t target, uint32_t av_id);
    virtual ~GetFriendsListOperation();

    // Starts the operation.
    void start();

    // Queries the avatar for the friends list.
    void get_friends_list();

    // Collects the friends list from the fields.
    void handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id);

    // Iterates over the friends list & retrieves the info for each friend.
    void get_friend_details();

    // Collects the details of each individual friend.
    void friend_callback(bool success = 0, uint32_t av_id = 0,
                         nlohmann::json &fields = nlohmann::json({}), bool is_pet = 0,
                         std::vector<AvatarBasicInfo> friend_details = std::vector<AvatarBasicInfo>{},
                         std::vector<uint32_t> online_friends = std::vector<uint32_t>{},
                         bool online = 0);

    // Checks if we're ready to see if the friends are online.
    void test_finished();

    // Checks if each friend is online.
    void check_friends_online();

    // Receives the activation query response for each friend.
    void get_activated_resp(uint32_t do_id, uint32_t ctx, bool activated);

    // Completes the operation.
    void finished();

    // Fails the operation.
    void failure(std::string reason);

  protected:
    ToontownClientManager *m_ttmgr;
    Operator *m_op;
    uint32_t m_av_id;
    std::vector<AvatarBasicInfo> m_friend_details;
    size_t m_iterated;
    std::map<uint32_t, GetAvatarInfoOperation*> m_operations;
    std::vector<uint32_t> m_online_friends;
    std::vector<uint32_t> m_friends_list;
};

class UpdateAvatarFieldOperation : virtual public GameOperation
{
  public:
    UpdateAvatarFieldOperation(ToontownClientManager *manager, DisneyClient& client, Operator *op,
                               uint32_t target, uint32_t sender_av_id, uint32_t av_id);
    virtual ~UpdateAvatarFieldOperation();

    // Starts the operation with the field & value.
    void start(std::string field, std::vector<uint32_t> value);

    // Checks if the avatar is online.
    void get_avatar_online();

    // Receives the activation query response for the avatar.
    void get_activated_resp(uint32_t do_id, uint32_t ctx, bool activated);

    // Updates the avatar field.
    void update_avatar_field();

    // Completes the operation.
    void finished();

    // Fails the operation.
    void failure(std::string reason);

  protected:
    Operator *m_op;
    uint32_t m_sender_av_id;
    uint32_t m_av_id;
    std::string m_field;
    std::vector<uint32_t> m_value;
    bool m_online;
};

class ToontownFriendOperator : virtual public Operator
{
  public:
    ToontownFriendOperator(ToontownClientManager *manager, DisneyClient& client, std::string op_name);
    virtual ~ToontownFriendOperator();

    // Calls a callback function depending on the operation name.
    void friend_callback(bool success = 0, uint32_t av_id = 0,
                         nlohmann::json &fields = nlohmann::json({}), bool is_pet = 0,
                         std::vector<AvatarBasicInfo> friend_details = std::vector<AvatarBasicInfo>{},
                         std::vector<uint32_t> online_friends = std::vector<uint32_t>{},
                         bool online = 0);

  private:
    // Sends the friends list response to the avatar.
    void got_friends_list(bool success, uint32_t av_id,
                          std::vector<AvatarBasicInfo> friend_details,
                          std::vector<uint32_t> online_friends);

    std::string m_op_name;
};

class ToontownClientManager : virtual public OTPClientManager
{
  public:
    NameGenerator *m_name_generator;

    std::map<uint32_t, AvatarBasicInfoCache> m_av_basic_info_cache;

    ToontownClientManager(DCClass *player_class, uint32_t database_id, std::string database_type,
                          std::string database_file, std::string name_file);

    // Gathers together PotentialAvatar structs through packed fields & an avatar set.
    std::vector<PotentialAvatar> get_potential_avatars(std::map<uint32_t, nlohmann::json> packed_fields, std::vector<uint32_t> av_set);

    // Forms name patterns into a full name string.
    std::string create_name(std::vector<std::pair<int16_t, uint8_t> > patterns);

    // Runs a ToontownLoginOperation.
    void login(DisneyClient& client, std::string play_token, channel_t sender, std::string version,
               uint32_t dc_hash, int32_t token_type, std::string want_magic_words);

    // Runs a ToontownCreateAvatarOperation.
    void create_avatar(DisneyClient& client, uint32_t sender,
                       std::string dna_string, uint8_t index);

    // Runs a SetNameTypedOperation.
    void set_name_typed(DisneyClient& client, uint32_t sender,
                        uint32_t av_id, std::string name);

    // Runs a SetNamePatternOperation.
    void set_name_pattern(DisneyClient& client, uint32_t sender, uint32_t av_id,
                          int16_t p1, uint8_t f1, int16_t p2, uint8_t f2,
                          int16_t p3, uint8_t f3, int16_t p4, uint8_t f4);

    // Runs a GetFriendsListOperation.
    void get_friends_list_request(DisneyClient& client, uint32_t sender, uint32_t av_id);
};