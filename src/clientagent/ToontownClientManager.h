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
    std::map<int, std::pair<int, std::string> > m_name_map;
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
    void handle_query(uint32_t ctx, uint16_t dclass_id, DCPacker &unpacker);

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
    void handle_query(uint32_t ctx, uint16_t dclass_id, DCPacker &unpacker);

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

class ToontownClientManager : virtual public OTPClientManager
{
  public:
    NameGenerator *m_name_generator;

    ToontownClientManager(DCClass *player_class, uint32_t database_id, std::string database_type,
                          std::string database_file, std::string name_file);

    // Gathers together PotentialAvatar structs through packed fields & an avatar set.
    std::vector<PotentialAvatar> get_potential_avatars(std::map<uint32_t, DCPacker> packed_fields, std::vector<uint32_t> av_set);

    // Forms name patterns into a full name string.
    std::string create_name(std::vector<std::pair<int16_t, uint8_t> > patterns);

    // Runs a ToontownLoginOperation.
    void login(DisneyClient& client, std::string play_token, channel_t sender, std::string version,
               uint32_t dc_hash, int32_t token_type, std::string want_magic_words);

    // Runs a ToontownCreateAvatarOperation.
    void create_toon(DisneyClient& client, uint32_t sender,
                     std::string dna_string, uint8_t index);

    // Runs a SetNameTypedOperation.
    void set_name_typed(DisneyClient& client, uint32_t sender,
                        uint32_t av_id, std::string name);

    // Runs a SetNamePatternOperation.
    void set_name_pattern(DisneyClient& client, uint32_t sender, uint32_t av_id,
                          int16_t p1, uint8_t f1, int16_t p2, uint8_t f2,
                          int16_t p3, uint8_t f3, int16_t p4, uint8_t f4);
};