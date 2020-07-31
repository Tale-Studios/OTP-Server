#pragma once
#include "core/global.h"
#include "core/Logger.h"
#include "json/json.hpp"
#include "OTPClientManager.h"
#include "Client.h"

class DisneyClient;
class TalkAssistant;

struct TalkModification
{
    uint16_t offset;
    uint16_t size;
};

// A TalkPath represents the channels in which a talk message
// would be transmitted to. A whisper path would be directly transmitted
// to a certain player, whereas a public message path would be sent
// to every visible object.
class TalkPath : public Operator
{
  public:
    TalkPath(TalkAssistant *talk_assistant, OTPClientManager *manager,
             DisneyClient& client, uint32_t av_id);
    ~TalkPath();

    void handle_talk(DatagramIterator& dgi);
    void handle_talk_whisper(DatagramIterator& dgi, uint32_t do_id);

  private:
    TalkAssistant* m_talk_assistant;
    uint32_t m_av_id;
};

// A TalkAssistant takes in a public talk message or a whisper and
// makes a TalkPath based on the player's location.
class TalkAssistant
{
  public:
    TalkAssistant(OTPClientManager *manager);
    ~TalkAssistant();

    bool in_list(std::string text, std::vector<std::string> list);

    void load_whitelist(std::string whitelist_path);
    std::vector<TalkModification> filter_whitelist(std::string message);

    void load_blacklist(std::string blacklist_path);
    std::tuple<std::string, std::string> filter_blacklist(std::string message);

    void set_talk(DisneyClient& client, uint32_t av_id, DatagramIterator& dgi);
    void set_talk_whisper(DisneyClient& client, uint32_t do_id,
                          uint32_t av_id, DatagramIterator& dgi);

  private:
    std::unique_ptr<LogCategory> m_log;

    OTPClientManager* m_manager;

    std::vector<std::string> m_whitelist;
    std::vector<std::string> m_blacklist;
};