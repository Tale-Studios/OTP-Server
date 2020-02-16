#include "ToontownClientManager.h"
#include "DisneyClient.cpp"
#include "DisneyClientMessages.h"
#include "core/global.h"
#include "core/msgtypes.h"
#include "json/json.hpp"

using namespace std;
using json = nlohmann::json;

NameGenerator::NameGenerator(string name_file)
{
    // Open & read the name file.
    ifstream f(name_file);
    if(!f) {
        // Not a valid file.
        return;
    }

    string current_line;
    while(getline(f, current_line)) {
        if(lstrip(current_line).substr(0, 1) != "#") {
            size_t a1 = current_line.find('*');
            size_t a2 = current_line.find('*', a1 + 1);
            m_name_map[stoi(current_line.substr(0, a1))] = make_pair(stoi(current_line.substr(a1 + 1, a2)), current_line.substr(a2 + 1, current_line.size() - 1));
        }
    }

    f.close();
}

NameGenerator::~NameGenerator()
{
}

string NameGenerator::make_name(vector<pair<int16_t, uint8_t> > patterns)
{
    if(m_name_map.empty()) {
        return "";
    }

    vector<string> parts;
    for(auto pattern : patterns) {
        int16_t p = pattern.first;
        uint8_t f = pattern.second;

        string part = "";

        if(m_name_map.find(p) != m_name_map.end()) {
            pair<int, string> part_pair = m_name_map[p];

            part = part_pair.second;
        }

        if(f) {
            part[0] = toupper(part[0]);
        } else {
            for(auto& c : part) {
                c = tolower(c);
            }
        }

        parts.push_back(part);
    }

    parts[2] += parts[3];
    parts.erase(parts.begin() + 3);

    for(size_t i=0;i<parts.size();++i) {
        string s = parts[i];

        if(s == "") {
            parts.erase(remove(parts.begin(), parts.end(), s), parts.end());
        }
    }

    return join(parts, " ");
}

ToontownLoginOperation::ToontownLoginOperation(ToontownClientManager *manager, DisneyClient& client, channel_t target) :
    LoginOperation(manager, client, target), GameOperation(manager, client, target),
    Operator(manager, client)
{
}

ToontownLoginOperation::~ToontownLoginOperation()
{
}

void ToontownLoginOperation::login_response(uint8_t return_code, string return_str,
                                            string play_token, uint32_t do_id)
{
    // Send the login response to the client.
    DatagramPtr resp = Datagram::create();
    resp->add_uint16(CLIENT_LOGIN_TOONTOWN_RESP);
    resp->add_uint8(return_code);
    resp->add_string(return_str);
    resp->add_uint32(do_id);
    resp->add_string(play_token);
    resp->add_uint8(1);
    resp->add_string("YES");
    resp->add_string("YES");
    resp->add_string("NULL");
    resp->add_uint32((uint32_t)time(0));
    resp->add_uint32(clock());
    resp->add_string("FULL");
    resp->add_string("YES");
    resp->add_string("-1");
    resp->add_int32(0);
    resp->add_string("NO_PARENT_ACCOUNT");
    resp->add_string(play_token);
    m_client.forward_datagram(resp);

    // Shut down this operation.
    off();
}

ToontownCreateAvatarOperation::ToontownCreateAvatarOperation(ToontownClientManager *manager, DisneyClient& client, channel_t target) :
    CreateAvatarOperation(manager, client, target), GameOperation(manager, client, target),
    Operator(manager, client)
{
}

ToontownCreateAvatarOperation::~ToontownCreateAvatarOperation()
{
}

void ToontownCreateAvatarOperation::create_avatar()
{
    // Get the Toon head color and animal type from the DNA string.
    uint8_t head_index;
    uint8_t head_color;
    try {
        DatagramPtr dna_dg = Datagram::create(m_dna_string);
        DatagramIterator dna_dgi = DatagramIterator(dna_dg);
        dna_dgi.get_fixed_string(1);
        head_index = dna_dgi.read_uint8();
        dna_dgi.skip(sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) +
                     sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) +
                     sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) +
                     sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t));
        head_color = dna_dgi.read_uint8();
    } catch(const DatagramIteratorEOF&) {
        // This DNA string is invalid! Kill the connection.
        kill("Invalid DNA specified!");
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

    // Put together a massive JSON object with all of the default toon fields.
    json toon_fields = {{"setName", {av_name}},
                        {"WishNameState", {"OPEN"}},
                        {"WishName", {""}},
                        {"setDNAString", {m_dna_string}},
                        {"setDISLid", {(uint32_t)m_target}},
                        {"setAccountName", {""}},
                        {"setFriendsList", {{}}},
                        {"setPreviousAccess", {0}},
                        {"setGM", {0}},
                        {"setMaxBankMoney", {12000}},
                        {"setBankMoney", {0}},
                        {"setMaxMoney", {40}},
                        {"setMoney", {0}},
                        {"setMaxHp", {15}},
                        {"setHp", {15}},
                        {"setExperience", {R"(\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00)"}},
                        {"setMaxCarry", {20}},
                        {"setTrackAccess", {{0, 0, 0, 0, 1, 1, 0}}},
                        {"setTrackProgress", {-1, 0}},
                        {"setTrackBonusLevel", {{-1, -1, -1, -1, -1, -1, -1}}},
                        {"setInventory", {R"(\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00)"}},
                        {"setMaxNPCFriends", {16}},
                        {"setNPCFriendsDict", {{}}},
                        {"setDefaultShard", {0}},
                        {"setDefaultZone", {0}},
                        {"setShtickerBook", {""}},
                        {"setZonesVisited", {{2000}}},
                        {"setHoodsVisited", {{2000}}},
                        {"setInterface", {""}},
                        {"setLastHood", {0}},
                        {"setTutorialAck", {1}},
                        {"setMaxClothes", {10}},
                        {"setClothesTopsList", {{}}},
                        {"setClothesBottomsList", {{}}},
                        {"setMaxAccessories", {0}},
                        {"setHatList", {{}}},
                        {"setGlassesList", {{}}},
                        {"setBackpackList", {{}}},
                        {"setShoesList", {{}}},
                        {"setHat", {0, 0, 0}},
                        {"setGlasses", {0, 0, 0}},
                        {"setBackpack", {0, 0, 0}},
                        {"setShoes", {0, 0, 0}},
                        {"setGardenSpecials", {{}}},
                        {"setEmoteAccess", {{1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
                        {"setCustomMessages", {{}}},
                        {"setResistanceMessages", {{}}},
                        {"setPetTrickPhrases", {{0}}},
                        {"setCatalogSchedule", {0, 0}},
                        {"setCatalog", {"", "", ""}},
                        {"setMailboxContents", {""}},
                        {"setDeliverySchedule", {""}},
                        {"setGiftSchedule", {""}},
                        {"setAwardMailboxContents", {""}},
                        {"setAwardSchedule", {""}},
                        {"setAwardNotify", {0}},
                        {"setCatalogNotify", {0, 0}},
                        {"setSpeedChatStyleIndex", {1}},
                        {"setTeleportAccess", {{}}},
                        {"setCogStatus", {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}}},
                        {"setCogCount", {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
                        {"setCogRadar", {{0, 0, 0, 0}}},
                        {"setBuildingRadar", {{0, 0, 0, 0}}},
                        {"setCogLevels", {{0, 0, 0, 0}}},
                        {"setCogTypes", {{0, 0, 0, 0}}},
                        {"setCogParts", {{0, 0, 0, 0}}},
                        {"setCogMerits", {{0, 0, 0, 0}}},
                        {"setHouseId", {0}},
                        {"setQuests", {{}}},
                        {"setQuestHistory", {{}}},
                        {"setRewardHistory", {0, {}}},
                        {"setQuestCarryLimit", {1}},
                        {"setCheesyEffect", {0, 0, 0}},
                        {"setPosIndex", {0}},
                        {"setFishCollection", {{}, {}, {}}},
                        {"setMaxFishTank", {20}},
                        {"setFishTank", {{}, {}, {}}},
                        {"setFishingRod", {0}},
                        {"setFishingTrophies", {{}}},
                        {"setFlowerCollection", {{}, {}}},
                        {"setFlowerBasket", {{}, {}}},
                        {"setMaxFlowerBasket", {20}},
                        {"setGardenTrophies", {{}}},
                        {"setShovel", {0}},
                        {"setShovelSkill", {0}},
                        {"setWateringCan", {0}},
                        {"setWateringCanSkill", {0}},
                        {"setPetId", {0}},
                        {"setPetTutorialDone", {0}},
                        {"setFishBingoTutorialDone", {0}},
                        {"setFishBingoMarkTutorialDone", {0}},
                        {"setKartBodyType", {-1}},
                        {"setKartBodyColor", {-1}},
                        {"setKartAccessoryColor", {-1}},
                        {"setKartEngineBlockType", {-1}},
                        {"setKartSpoilerType", {-1}},
                        {"setKartFrontWheelWellType", {-1}},
                        {"setKartBackWheelWellType", {-1}},
                        {"setKartRimType", {-1}},
                        {"setKartDecalType", {-1}},
                        {"setTickets", {200}},
                        {"setKartingHistory", {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
                        {"setKartingTrophies", {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
                        {"setKartingPersonalBest", {{0, 0, 0, 0, 0, 0}}},
                        {"setKartingPersonalBest2", {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
                        {"setKartAccessoriesOwned", {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}},
                        {"setCogSummonsEarned", {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
                        {"setGardenStarted", {0}},
                        {"setGolfHistory", {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
                        {"setPackedGolfHoleBest", {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
                        {"setGolfCourseBest", {{0, 0, 0}}},
                        {"setPinkSlips", {0}},
                        {"setNametagStyle", {0}}};

    // Create this new Toon object in the database.
    create_object(m_manager->m_database_id, m_manager->m_player_class, toon_fields);
}

SetNameTypedOperation::SetNameTypedOperation(ToontownClientManager *manager, DisneyClient& client, channel_t target) :
    AvatarOperation(manager, client, target), GameOperation(manager, client, target),
    Operator(manager, client), m_av_id(0)
{
}

SetNameTypedOperation::~SetNameTypedOperation()
{
}

void SetNameTypedOperation::start(uint32_t av_id, string name)
{
    m_av_id = av_id;
    m_av_name = name;

    if(av_id != 0) {
        // If av_id is not 0, the user is submitting this name,
        // so we need to retrieve the account.
        retrieve_account();
        return;
    }

    // Otherwise, av_id is 0, meaning that a check request was sent.
    judge_name();
}

void SetNameTypedOperation::post_account_func()
{
    if(m_av_id > 0 && find(m_av_set.begin(), m_av_set.end(), m_av_id) == m_av_set.end()) {
        // The avatar exists, but it's not an avatar that is
        // associated with this account. Kill the connection.
        kill("Tried to name an avatar not in the account!");
        return;
    }

    // Query the database for the avatar.
    query_object(m_manager->m_database_id, m_av_id);
}

void SetNameTypedOperation::handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id)
{
    if(!m_past_acc_query) {
        // Not our turn yet.
        AvatarOperation::handle_query(dgi, ctx, dclass_id);
        return;
    }

    AvatarOperation::handle_query(dgi, ctx, dclass_id);

    if(m_manager->m_player_class->get_number() != dclass_id) {
        // This dclass is not a valid avatar! Kill the connection.
        kill(string("One of the account's avatars is invalid! dclass = ") + to_string(dclass_id) + ", expected = " + to_string(m_manager->m_player_class->get_number()));
        return;
    }

    // Extract the avatar fields.
    uint16_t field_count = dgi.read_uint16();
    json avatar = unpack_json_objects(dgi, m_manager->m_player_class, field_count);

    // Get the wish name state.
    string wish_name_state = avatar["WishNameState"].get<vector<string> >()[0];
    if(wish_name_state != "OPEN") {
        // This avatar's wish name state is not set
        // to a nameable state. Kill the connection.
        kill("Avatar is not in a nameable state!");
        return;
    }

    // Now we can move on to the judging!
    judge_name();
}

void SetNameTypedOperation::judge_name()
{
    // Let's see if the name is valid.
    bool status = m_manager->judge_name(m_av_name);

    if(m_av_id > 0 && status) {
        // Cool, this is a valid name, and we have an av_id.
        // Let's update their avatar with the new wish name & status.
        update_object(m_manager->m_database_id, m_av_id, m_manager->m_player_class, json({
                      {"WishNameState", {"OPEN"}}, {"WishName", {m_av_name}}}), json({}));
    }

    if(m_av_id != 0) {
        // If the av_id is not 0, log this server event, as the avatar's
        // wish name & state have been modified.
        LoggedEvent event("avatar-wish-name");
        event.add("av_id", to_string(m_av_id));
        event.add("name", m_av_name);
        m_client.write_server_event(event);
    }

    // Otherwise, we're done! We can now send the response update and shut down.
    DatagramPtr resp = Datagram::create();
    resp->add_uint16(CLIENT_SET_WISHNAME_RESP);
    resp->add_uint32(m_av_id);
    resp->add_uint16(0);
    resp->add_string("YES");
    resp->add_string("");
    resp->add_string("");
    m_client.forward_datagram(resp);
    off();
}

SetNamePatternOperation::SetNamePatternOperation(ToontownClientManager *manager, DisneyClient& client, channel_t target) :
    AvatarOperation(manager, client, target), GameOperation(manager, client, target),
    Operator(manager, client), m_av_id(0)
{
}

SetNamePatternOperation::~SetNamePatternOperation()
{
}

void SetNamePatternOperation::start(uint32_t av_id, int16_t p1, uint8_t f1, int16_t p2, uint8_t f2,
                                    int16_t p3, uint8_t f3, int16_t p4, uint8_t f4)
{
    // Store these values.
    m_av_id = av_id;
    m_p1 = p1;
    m_f1 = f1;
    m_p2 = p2;
    m_f2 = f2;
    m_p3 = p3;
    m_f3 = f3;
    m_p4 = p4;
    m_f4 = f4;

    // Retrieve the account.
    retrieve_account();
}

void SetNamePatternOperation::post_account_func()
{
    if(m_av_id > 0 && find(m_av_set.begin(), m_av_set.end(), m_av_id) == m_av_set.end()) {
        // The avatar exists, but it's not an avatar that is
        // associated with this account. Kill the connection.
        kill("Tried to name an avatar not in the account!");
        return;
    }

    // Query the database for the avatar.
    query_object(m_manager->m_database_id, m_av_id);
}

void SetNamePatternOperation::handle_query(DatagramIterator &dgi, uint32_t ctx, uint16_t dclass_id)
{
    if(!m_past_acc_query) {
        // Not our turn yet.
        AvatarOperation::handle_query(dgi, ctx, dclass_id);
        return;
    }

    AvatarOperation::handle_query(dgi, ctx, dclass_id);

    if(m_manager->m_player_class->get_number() != dclass_id) {
        // This dclass is not a valid avatar! Kill the connection.
        kill(string("One of the account's avatars is invalid! dclass = ") + to_string(dclass_id) + ", expected = " + to_string(m_manager->m_player_class->get_number()));
        return;
    }

    // Extract the avatar fields.
    uint16_t field_count = dgi.read_uint16();
    json avatar = unpack_json_objects(dgi, m_manager->m_player_class, field_count);

    // Get the wish name state.
    string wish_name_state = avatar["WishNameState"].get<vector<string> >()[0];
    if(wish_name_state != "OPEN") {
        // This avatar's wish name state is not set
        // to a nameable state. Kill the connection.
        kill("Avatar is not in a nameable state!");
        return;
    }

    // Otherwise, we can set the name.
    set_name();
}

void SetNamePatternOperation::set_name()
{
    vector<pair<int16_t, uint8_t> > patterns;
    patterns.push_back(make_pair(m_p1, m_f1));
    patterns.push_back(make_pair(m_p2, m_f2));
    patterns.push_back(make_pair(m_p3, m_f3));
    patterns.push_back(make_pair(m_p4, m_f4));
    string name = m_manager->create_name(patterns);
    if(!name.size()) {
        kill("Failed to compute avatar pattern name!");
        return;
    }

    // We can now update the avatar object with the name.
    update_object(m_manager->m_database_id, m_av_id, m_manager->m_player_class, json({
                  {"WishNameState", {"LOCKED"}},
                  {"WishName", {""}},
                  {"setName", {name}}}), json({}));

    // We're done. We can now send the response update and shut down.
    LoggedEvent event("avatar-named");
    event.add("av_id", to_string(m_av_id));
    event.add("name", name);
    m_client.write_server_event(event);

    DatagramPtr resp = Datagram::create();
    resp->add_uint16(CLIENT_SET_NAME_PATTERN_ANSWER);
    resp->add_uint32(m_av_id);
    resp->add_uint8(0);
    m_client.forward_datagram(resp);
    off();
}

ToontownClientManager::ToontownClientManager(DCClass* player_class, uint32_t database_id, string database_type,
                                             string database_file, string name_file) :
    OTPClientManager(player_class, database_id, 6, database_type, database_file)
{
    m_name_generator = new NameGenerator(name_file);
}

vector<PotentialAvatar> ToontownClientManager::get_potential_avatars(map<uint32_t, json> packed_fields, vector<uint32_t> av_set)
{
    // Here is where we'll construct a list of potential avatars,
    // given the data from the packed fields, and send that to the client.
    vector<PotentialAvatar> potential_avatars;

    // Loop through the avatar set vector:
    for(size_t i = 0; i < av_set.size(); ++i) {
        uint32_t av_id = av_set[i];
        if(av_id <= 0) {
            continue;
        }

        // Get the fields for this avatar.
        json fields = packed_fields[av_id];

        // Get the appropriate values.
        string wish_name = "";
        uint8_t name_state = 0;
        string wish_name_state = fields["WishNameState"].get<vector<string> >()[0];
        string name = fields["setName"].get<vector<string> >()[0];
        string field_wish_name = fields["WishName"].get<vector<string> >()[0];
        string dna_string = fields["setDNAString"].get<vector<string> >()[0];

        if(wish_name_state == "OPEN") {
            name_state = 1;
        } else if(wish_name_state == "PENDING") {
            name_state = 2;
        } else if(wish_name_state == "APPROVED") {
            name_state = 3;
            name = field_wish_name;
        } else if(wish_name_state == "REJECTED") {
            name_state = 4;
        } else if(wish_name_state == "LOCKED") {
            name_state = 0;
        } else {
            name_state = 0;
        }

        PotentialAvatar data = PotentialAvatar();
        data.av_id = av_id;
        data.name = name;
        data.dna_string = dna_string;
        data.index = i;

        // Add the PotentialAvatar to potential_avatars.
        potential_avatars.push_back(data);
    }

    return potential_avatars;
}

string ToontownClientManager::create_name(vector<pair<int16_t, uint8_t> > patterns)
{
    return m_name_generator->make_name(patterns);
}

void ToontownClientManager::login(DisneyClient& client, string play_token, channel_t sender, string version,
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
    ToontownLoginOperation* op = new ToontownLoginOperation(this, client, sender);
    m_connection2operation[sender] = op;
    op->start(play_token, version, dc_hash, token_type, want_magic_words);
}

void ToontownClientManager::create_avatar(DisneyClient& client, uint32_t sender,
                                          string dna_string, uint8_t index)
{
    ToontownCreateAvatarOperation* operation = new ToontownCreateAvatarOperation(this, client, sender);
    bool success = run_operation(operation, sender);
    if(success) {
        operation->start(dna_string, index);
    }
}

void ToontownClientManager::set_name_typed(DisneyClient& client, uint32_t sender,
                                           uint32_t av_id, string name)
{
    SetNameTypedOperation* operation = new SetNameTypedOperation(this, client, sender);
    bool success = run_operation(operation, sender);
    if(success) {
        operation->start(av_id, name);
    }
}

void ToontownClientManager::set_name_pattern(DisneyClient& client, uint32_t sender, uint32_t av_id, int16_t p1, uint8_t f1, int16_t p2,
                                             uint8_t f2, int16_t p3, uint8_t f3, int16_t p4, uint8_t f4)
{
    SetNamePatternOperation* operation = new SetNamePatternOperation(this, client, sender);
    bool success = run_operation(operation, sender);
    if(success) {
        operation->start(av_id, p1, f1, p2, f2, p3, f3, p4, f4);
    }
}