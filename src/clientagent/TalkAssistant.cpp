#include "TalkAssistant.h"
#include "OTPClientManager.h"
#include "DisneyClient.cpp"
#include "json/json.hpp"

using namespace std;
using json = nlohmann::json;

TalkPath::TalkPath(TalkAssistant *talk_assistant, OTPClientManager *manager,
                   DisneyClient& client, uint32_t av_id) :
    Operator(manager, client), m_talk_assistant(talk_assistant), m_av_id(av_id)
{
}

TalkPath::~TalkPath()
{
}

void TalkPath::handle_talk(DatagramIterator& dgi)
{
    // First, we need to unpack the setTalk field into a JSON object.
    json data = unpack_json_objects(dgi, m_manager->m_player_class, 1, 103);
    dgi.read_remainder();
    string message = data["setTalk"].at(3);
    vector<TalkModification> mod_vector = m_talk_assistant->filter_whitelist(message);
    json field;
    if(mod_vector.size()) {
        json mods = json::array();
        for(auto mod : mod_vector) {
            mods.push_back(json::array({mod.offset, mod.size}));
        }
        field = {{"setTalk", {m_av_id, 0, "", message, mods, 0}}};
    } else {
        field = {{"setTalk", {m_av_id, 0, "", message, json::array(), 0}}};
    }

    // Pack a new field.
    DCPacker packer;
    pack_json_objects(packer, m_manager->m_player_class, field, 0);

    // Update the field for the local avatar.
    DatagramPtr resp = Datagram::create();
    resp->add_uint16(CLIENT_OBJECT_UPDATE_FIELD);
    resp->add_doid(m_av_id);
    resp->add_uint16(103);
    resp->add_data(packer.get_string());
    m_client.forward_datagram(resp);

    // Update the field for any visible avatars.
    vector<doid_t> avs = m_client.get_visible_avatars();
    for(doid_t av_id : avs) {
        DatagramPtr dg = Datagram::create(get_puppet_connection_channel(av_id),
                                          m_av_id,
                                          STATESERVER_OBJECT_SET_FIELD);
        dg->add_doid(m_av_id);
        dg->add_uint16(103);
        dg->add_data(packer.get_string());
        m_client.dispatch_datagram(dg);
    }
}

void TalkPath::handle_talk_whisper(DatagramIterator& dgi, uint32_t do_id)
{
    // First, we need to unpack the setTalkWhisper field into a JSON object.
    json data = unpack_json_objects(dgi, m_manager->m_player_class, 1, 104);
    dgi.read_remainder();
    string message = data["setTalkWhisper"].at(3);
    vector<TalkModification> mod_vector = m_talk_assistant->filter_whitelist(message);
    json field;
    if(mod_vector.size()) {
        json mods = json::array();
        for(auto mod : mod_vector) {
            mods.push_back(json::array({mod.offset, mod.size}));
        }
        field = {{"setTalkWhisper", {m_av_id, 0, "", message, mods, 0}}};
    } else {
        field = {{"setTalkWhisper", {m_av_id, 0, "", message, json::array(), 0}}};
    }

    // Pack a new field.
    DCPacker packer;
    pack_json_objects(packer, m_manager->m_player_class, field, 0);

    // Update the field for the recipient.
    DatagramPtr resp = Datagram::create(get_puppet_connection_channel(do_id),
                                        m_av_id,
                                        STATESERVER_OBJECT_SET_FIELD);
    resp->add_doid(do_id);
    resp->add_uint16(104);
    resp->add_data(packer.get_string());
    m_client.dispatch_datagram(resp);
}

TalkAssistant::TalkAssistant(OTPClientManager *manager) : m_manager(manager)
{
    m_log = std::unique_ptr<LogCategory>(new LogCategory("talkassistant", "TalkAssistant"));
}

TalkAssistant::~TalkAssistant()
{
}

void TalkAssistant::load_whitelist(string whitelist_path)
{
    if(m_whitelist.size() > 0) {
        // The whitelist is already loaded.
        return;
    }

    // Open the whitelist file.
    ifstream whitelist_file(whitelist_path);
    if(whitelist_file.good()) {
        m_log->info() << "Loading whitelist file: " << whitelist_path << endl;

        // Populate the whitelist vector.
        string tok;
        while(std::getline(whitelist_file, tok, '\n')) {
            // Clean it:
            tok.erase(tok.find_last_not_of("\n\r") + 1);
            tok.erase(0, tok.find_first_not_of("\n\r"));
            transform(tok.begin(), tok.end(), tok.begin(),
                      [](unsigned char c){ return tolower(c); });

            // Put it back:
            m_whitelist.push_back(tok);
        }
    } else {
        m_log->error() << "Failed to read whitelist file!" << endl;
    }
}

bool TalkAssistant::in_whitelist(string text)
{
    // Clean the text first:
    text.erase(text.find_last_not_of(".,?!") + 1);
    text.erase(0, text.find_first_not_of(".,?!"));
    transform(text.begin(), text.end(), text.begin(),
              [](unsigned char c){ return tolower(c); });

    // Find it in the whitelist.
    if(find(m_whitelist.begin(), m_whitelist.end(), text) != m_whitelist.end()) {
        return true;
    }

    // It's not in the whitelist.
    return false;
}

vector<TalkModification> TalkAssistant::filter_whitelist(string message)
{
    // Make a stream to split the message.
    vector<string> words;
    stringstream ss(message);
    string tok;

    // Keep splitting the message by the space delimiter.
    while(std::getline(ss, tok, ' ')) {
        words.push_back(tok);
    }

    size_t offset = 0;
    vector<TalkModification> mods;
    for(auto word : words) {
        if(!in_whitelist(word)) {
            TalkModification mod = TalkModification();
            mod.offset = offset;
            mod.size = offset + word.size() - 1;
            mods.push_back(mod);
        }

        offset += word.size() + 1;
    }

    return mods;
}

void TalkAssistant::set_talk(DisneyClient& client, uint32_t av_id, DatagramIterator& dgi)
{
    // Create a new TalkPath.
    TalkPath* talk_path = new TalkPath(this, m_manager, client, av_id);

    // Handle the talk request.
    talk_path->handle_talk(dgi);
}

void TalkAssistant::set_talk_whisper(DisneyClient& client, uint32_t do_id,
                                     uint32_t av_id, DatagramIterator& dgi)
{
    // Create a new TalkPath.
    TalkPath* talk_path = new TalkPath(this, m_manager, client, av_id);

    // Handle the talk request.
    talk_path->handle_talk_whisper(dgi, do_id);
}