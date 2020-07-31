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

    // Extract the message and filter it through the blacklist.
    string pure_message = data["setTalk"].at(3);
    if(!pure_message.size()) {
        return;
    }

    tuple<string, bool> filter = m_talk_assistant->filter_blacklist(pure_message);
    string message = get<0>(filter);
    bool blacklisted = get<1>(filter);

    if(blacklisted) {
        // There have been blacklisted words. In this case, we overwrite the message,
        // and only the local avatar will be able to see what they said.
        message = "";
        for(size_t i = 0; i < pure_message.size(); ++i) {
            message += " ";
        }
    }

    // Filter the new message through the whitelist as well.
    vector<TalkModification> mod_vector = m_talk_assistant->filter_whitelist(message);
    json mods = json::array();
    for(auto mod : mod_vector) {
        mods.push_back(json::array({mod.offset, mod.size}));
    }

    // For the setTalk field, we want three talk paths: the local path, the normal path,
    // and the secret friend path. The local path is meant only for the local avatar's
    // eyes: if the local avatar has true friends, it is expected that they see
    // words that aren't in the whitelist without scrubbing. As for the normal path,
    // they will be seen by any non-true friends that can see the message and
    // contain normal scrubbing where necessary. The secret friend path would
    // be the same as the local path as in both cases any non-whitelisted words
    // would appear in italics, but it would be directed towards any secret
    // friends that can see the message as opposed to just the local avatar.
    // We'll start off by preparing the local path.
    json local_path;
    if(m_client.count_true_friends() || blacklisted) {
        // We have true friends or used a blacklisted word; just pass an empty array
        // in place of any necessary modifications.
        local_path = {{"setTalk", {m_av_id, 0, m_client.get_avatar_name(), pure_message, json::array(), 0}}};
    } else {
        // No true friends here; pass any existing modifications.
        local_path = {{"setTalk", {m_av_id, 0, m_client.get_avatar_name(), message, mods, 0}}};
    }

    // Now for the normal path, which contains any necessary modifications irregardless.
    json normal_path = {{"setTalk", {m_av_id, 0, m_client.get_avatar_name(), message, mods, 0}}};

    // And finally, the true friend path, which is the same as the local path.
    // If the true friend path were to be used, it would mean that the local avatar
    // does have true friends, and therefore the local path wouldn't be scrubbed
    // anyways. The exception for this is if the message contains a blacklisted
    // word; in this case, we'll just apply whitelist modifications.
    json tf_path = local_path;
    if(blacklisted) {
        tf_path = normal_path;
    }

    // Now we need to calculate targets! We skip this for the local path as we
    // can just directly dispatch that to our client. For the normal path,
    // we have to get a set containing visible avatars.
    // We can use a handy argument to filter out secret friends.
    unordered_set<channel_t> normal_targets = m_client.get_visible_targets(true);

    // For the secret friend path, we take an unfiltered visible avatars vector
    // and filter the non-true friend avatars out of there.
    vector<doid_t> sf_targets_av = m_client.get_visible_avatars();
    for(auto it = sf_targets_av.begin(); it != sf_targets_av.end(); it++) {
        if(!m_client.is_true_friend(*it)) {
            sf_targets_av.erase(it--);
        }
    }

    // We also need to convert that vector into an unordered set of channels.
    unordered_set<channel_t> sf_targets;
    for(auto do_id : sf_targets_av) {
        sf_targets.insert(do_id + ((int64_t)1001L << 32));
    }

    // In comes the field updates. Starting with the local path, we just
    // pack the field, and dispatch it.
    DCPacker local_packer;
    pack_json_objects(local_packer, m_manager->m_player_class, local_path, 0);

    // And dispatch:
    DatagramPtr local_dg = Datagram::create();
    local_dg->add_uint16(CLIENT_OBJECT_UPDATE_FIELD);
    local_dg->add_doid(m_av_id);
    local_dg->add_uint16(103);
    local_dg->add_data(local_packer.get_string());
    m_client.forward_datagram(local_dg);

    // Now, we pack the normal path, and send that out to the normal targets.
    DCPacker normal_packer;
    pack_json_objects(normal_packer, m_manager->m_player_class, normal_path, 0);

    // Send it out:
    DatagramPtr normal_dg = Datagram::create(normal_targets,
                                             m_av_id,
                                             STATESERVER_OBJECT_SET_FIELD);
    normal_dg->add_doid(m_av_id);
    normal_dg->add_uint16(103);
    normal_dg->add_data(normal_packer.get_string());
    m_client.dispatch_datagram(normal_dg);

    // Finally, we pack the secret friend path, and send that out to visible true friends.
    DCPacker sf_packer;
    pack_json_objects(sf_packer, m_manager->m_player_class, tf_path, 0);

    // Send it out:
    DatagramPtr sf_dg = Datagram::create(sf_targets,
                                         m_av_id,
                                         STATESERVER_OBJECT_SET_FIELD);
    sf_dg->add_doid(m_av_id);
    sf_dg->add_uint16(103);
    sf_dg->add_data(sf_packer.get_string());
    m_client.dispatch_datagram(sf_dg);
}

void TalkPath::handle_talk_whisper(DatagramIterator& dgi, uint32_t do_id)
{
    // First, we need to unpack the setTalkWhisper field into a JSON object.
    json data = unpack_json_objects(dgi, m_manager->m_player_class, 1, 104);
    dgi.read_remainder();

    // Extract the message and filter it through the blacklist.
    string message = data["setTalkWhisper"].at(3);
    if(!message.size()) {
        return;
    }

    tuple<string, bool> filter = m_talk_assistant->filter_blacklist(message);
    message = get<0>(filter);

    if(get<1>(filter)) {
        // There have been blacklisted words.
        return;
    }

    // Filter the new message through the whitelist as well.
    vector<TalkModification> mod_vector = m_talk_assistant->filter_whitelist(message);

    // Set up the new field with the arguments filled in.
    json field;
    if(mod_vector.size() && !m_client.is_true_friend(do_id)) {
        json mods = json::array();
        for(auto mod : mod_vector) {
            mods.push_back(json::array({mod.offset, mod.size}));
        }
        field = {{"setTalkWhisper", {m_av_id, 0, m_client.get_avatar_name(), message, mods, 0}}};
    } else {
        field = {{"setTalkWhisper", {m_av_id, 0, m_client.get_avatar_name(), message, json::array(), 0}}};
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

bool TalkAssistant::in_list(string text, vector<string> list)
{
    // Clean the text first:
    text.erase(text.find_last_not_of(".,?!") + 1);
    text.erase(0, text.find_first_not_of(".,?!"));
    transform(text.begin(), text.end(), text.begin(),
              [](unsigned char c){ return tolower(c); });

    // Nothing always works.
    if(text.size() == 0) {
        if(list == m_blacklist) {
            return false;
        }

        return true;
    }

    // Find it in the list.
    if(find(list.begin(), list.end(), text) != list.end()) {
        return true;
    }

    // It's not in the list.
    return false;
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
        if(!in_list(word, m_whitelist)) {
            TalkModification mod = TalkModification();
            mod.offset = offset;
            mod.size = offset + word.size() - 1;
            mods.push_back(mod);
        }

        offset += word.size() + 1;
    }

    return mods;
}

void TalkAssistant::load_blacklist(string blacklist_path)
{
    if(m_blacklist.size() > 0) {
        // The blacklist is already loaded.
        return;
    }

    // Open the blacklist file.
    ifstream blacklist_file(blacklist_path);
    if(blacklist_file.good()) {
        m_log->info() << "Loading blacklist file: " << blacklist_path << endl;

        // Populate the blacklist vector.
        string tok;
        while(std::getline(blacklist_file, tok, '\n')) {
            // Clean it:
            tok.erase(tok.find_last_not_of("\n\r") + 1);
            tok.erase(0, tok.find_first_not_of("\n\r"));
            transform(tok.begin(), tok.end(), tok.begin(),
                      [](unsigned char c){ return tolower(c); });

            // Put it back:
            m_blacklist.push_back(tok);
        }
    } else {
        m_log->error() << "Failed to read blacklist file!" << endl;
    }
}

tuple<string, bool> TalkAssistant::filter_blacklist(string message)
{
    // Make a stream to split the message.
    vector<string> words;
    stringstream ss(message);
    string tok;

    // Keep splitting the message by the space delimiter.
    while(std::getline(ss, tok, ' ')) {
        words.push_back(tok);
    }

    vector<string> new_words;
    bool flag = false;
    for(auto word : words) {
        if(in_list(word, m_blacklist)) {
            flag = true;
            continue;
        }

        new_words.push_back(word);
    }

    stringstream res;
    copy(new_words.begin(), new_words.end(), ostream_iterator<string>(res, " "));

    return make_tuple(res.str(), flag);
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