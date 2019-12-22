#include "ToontownClientManager.h"
#include "DisneyClient.cpp"
#include "ClientMessages.h"
#include "core/global.h"
#include "core/msgtypes.h"

using namespace std;

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
            string part_sub = part.substr(0, 1);
            for (auto & c: part_sub) c = toupper(c);
            part = part_sub + part.substr(1, 0);
        } else {
            for (auto & c: part) c = tolower(c);
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
    uint8_t head_index = 0;
    uint8_t head_color = 0;
    try {
        DatagramPtr dna_dg = Datagram::create(m_dna_string);
        DatagramIterator dna_dgi = DatagramIterator(dna_dg);
        dna_dgi.get_fixed_string(1);
        uint8_t head_index = dna_dgi.read_uint8();
        dna_dgi.skip(sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) +
                     sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) +
                     sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) +
                     sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t));
        uint8_t head_color = dna_dgi.read_uint8();
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
    switch(head_color)
    {
        case 1:
            color = "Peach";
        case 2:
            color = "Bright Red";
        case 3:
            color = "Red";
        case 4:
            color = "Maroon";
        case 5:
            color = "Sienna";
        case 6:
            color = "Brown";
        case 7:
            color = "Tan";
        case 8:
            color = "Coral";
        case 9:
            color = "Orange";
        case 10:
            color = "Yellow";
        case 11:
            color = "Cream";
        case 12:
            color = "Citrine";
        case 13:
            color = "Lime";
        case 14:
            color = "Sea Green";
        case 15:
            color = "Green";
        case 16:
            color = "Light Blue";
        case 17:
            color = "Aqua";
        case 18:
            color = "Blue";
        case 19:
            color = "Periwinkle";
        case 20:
            color = "Royal Blue";
        case 21:
            color = "Slate Blue";
        case 22:
            color = "Purple";
        case 23:
            color = "Lavender";
        case 24:
            color = "Pink";
        case 25:
            color = "Plum";
        case 26:
            color = "Black";
        default:
            color = "White";
    }

    // Add the color and animal together to create the temporary name.
    string av_name = color + animal;

    // We will now construct a new Toon with the given values.
    DCPacker av_packer;

    // Get the avatar dclass.
    DCClass *avatar = m_manager->m_player_class;

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
    pack_string(av_packer, dna_field, m_dna_string);

    // Pack the account ID.
    DCField *disl_field = avatar->get_field_by_name("setDISLid");
    pack_uint(av_packer, disl_field, (uint32_t)m_target);

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
    pack_string(av_packer, avatar->get_field_by_name("setExperience"), "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
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
    pack_string(av_packer, avatar->get_field_by_name("setInventory"), "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
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

    // Create this new Toon object in the database.
    create_object(m_manager->m_database_id, avatar, av_packer, 112);
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

void SetNameTypedOperation::handle_query(uint32_t ctx, uint16_t dclass_id, DCPacker &unpacker)
{
    AvatarOperation::handle_query(ctx, dclass_id, unpacker);

    if(m_manager->m_player_class->get_number() != dclass_id) {
        // This dclass is not a valid avatar! Kill the connection.
        kill("One of the account's avatars is invalid!");
        return;
    }

    DCField* wish_name_state_field = m_manager->m_player_class->get_field_by_name("WishNameState");
    string wish_name_state = unpack_string_field(unpacker, wish_name_state_field, 1).front();
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
        DCField* wish_name_state_field = m_manager->m_player_class->get_field_by_name("WishNameState");
        DCField* wish_name_field = m_manager->m_player_class->get_field_by_name("WishName");
        vector<DCField*> fields {wish_name_state_field, wish_name_field};
        vector<DCPacker> new_fields;

        DCPacker wish_name_state_packer;
        DCPacker wish_name_packer;

        vector<string> wns {"OPEN"};
        vector<string> wn {m_av_name};

        pack_string_field(wish_name_state_packer, wish_name_state_field, wns, false, false);
        pack_string_field(wish_name_packer, wish_name_field, wn, false, false);

        new_fields.push_back(wish_name_state_packer);
        new_fields.push_back(wish_name_packer);

        // Update the object.
        update_object(m_manager->m_database_id, m_av_id, fields, new_fields, vector<DCPacker>{});
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

void SetNamePatternOperation::handle_query(uint32_t ctx, uint16_t dclass_id, DCPacker &unpacker)
{
    AvatarOperation::handle_query(ctx, dclass_id, unpacker);

    if(m_manager->m_player_class->get_number() != dclass_id) {
        // This dclass is not a valid avatar! Kill the connection.
        kill("One of the account's avatars is invalid!");
        return;
    }

    DCField* wish_name_state_field = m_manager->m_player_class->get_field_by_name("WishNameState");
    string wish_name_state = unpack_string_field(unpacker, wish_name_state_field, 1).front();
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

    // We can now update the avatar object with the name.
    DCField* wish_name_state_field = m_manager->m_player_class->get_field_by_name("WishNameState");
    DCField* wish_name_field = m_manager->m_player_class->get_field_by_name("WishName");
    DCField* set_name_field = m_manager->m_player_class->get_field_by_name("setName");
    vector<DCField*> fields {wish_name_state_field, wish_name_field, set_name_field};
    vector<DCPacker> new_fields;

    DCPacker wish_name_state_packer;
    DCPacker wish_name_packer;
    DCPacker name_packer;

    vector<string> wns {"LOCKED"};
    vector<string> wn {""};
    vector<string> n {name};

    pack_string_field(wish_name_state_packer, wish_name_state_field, wns, false, false);
    pack_string_field(wish_name_packer, wish_name_field, wn, false, false);
    pack_string_field(name_packer, set_name_field, n, false, false);

    new_fields.push_back(wish_name_state_packer);
    new_fields.push_back(wish_name_packer);
    new_fields.push_back(name_packer);

    // Update the object.
    update_object(m_manager->m_database_id, m_av_id, fields, new_fields, vector<DCPacker>{});

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

vector<PotentialAvatar> ToontownClientManager::get_potential_avatars(map<uint32_t, DCPacker> packed_fields, vector<uint32_t> av_set)
{
    // Here is where we'll construct a list of potential avatars,
    // given the data from the packed fields, and send that to the client.
    vector<PotentialAvatar> potential_avatars;

    // Loop through the avatar set vector:
    for(size_t i = 0; i < av_set.size(); ++i) {
        uint32_t av_id = av_set[i];

        DCPacker unpacker = packed_fields[av_id];
        DCClass *toon = g_dcf->get_class_by_name("DistributedToon");
        DCField *wish_name_state_field = toon->get_field_by_name("WishNameState");
        DCField *set_name_field = toon->get_field_by_name("setName");
        DCField *wish_name_field = toon->get_field_by_name("WishName");
        DCField *dna_string_field = toon->get_field_by_name("setDNAString");

        // Get the appropriate values.
        string wish_name = "";
        uint8_t name_state = 0;

        unpacker.begin_unpack(wish_name_state_field);
        unpacker.seek("WishNameState");
        string wish_name_state = unpacker.unpack_string();
        unpacker.end_unpack();

        unpacker.begin_unpack(set_name_field);
        unpacker.seek("setName");
        string name = unpacker.unpack_string();
        unpacker.end_unpack();

        unpacker.begin_unpack(wish_name_field);
        unpacker.seek("WishName");
        string field_wish_name = unpacker.unpack_string();
        unpacker.end_unpack();

        unpacker.begin_unpack(dna_string_field);
        unpacker.seek("setDNAString");
        string dna_string = unpacker.unpack_string();
        unpacker.end_unpack();

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