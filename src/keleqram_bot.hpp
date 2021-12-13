#include <vector>
#include <unordered_map>
#include "util.hpp"

namespace keleqram {
using  TelegramException = TgBot::TgException;
using  TXMessages        = std::unordered_map<int64_t, std::vector<int32_t>>;
const extern int64_t DEFAULT_CHAT_ID;

template<typename... Args>
static void log(Args... args)
{
  for (const auto& s : {args...})
    std::cout << s;
  std::cout << std::endl;
}

struct RequestInfo
{
std::string id;
std::string message;
};

class KeleqramBot
{
public:
using LongPoll    = TgBot::TgLongPoll;
using TelegramBot = TgBot::Bot;
using TelegramAPI = TgBot::Api;
using Replies     = std::vector<std::string>;

KeleqramBot(const std::string& token = "");

void SetListeners();
void Poll();
void HandleMessage(MessagePtr message);
void HandlePrivateRequest(MessagePtr message);
void HandleEvent(MessagePtr message);
template<typename T>
void SendMessage(const std::string& text, const T& id, const std::string& parse_mode = "");
template<typename T>
void SendMedia  (const std::string& url,  const T& id);
void DeleteMessages(MessagePtr message);

private:
bool IsReply(const int64_t& chat_id, const int32_t& id);

TelegramBot          m_bot;
TelegramAPI          m_api;
LongPoll             m_poll;
uint32_t             tx;
uint32_t             rx;
uint32_t             tx_err;
uint32_t             rx_err;
TXMessages           tx_msgs;
Replies              m_replies;
};

class kint8_t
{
public:
kint8_t(uint8_t max_value = 5)
: m_value(0),
  m_max(max_value)
{}


uint8_t operator++(int)
{
  const uint8_t ret = m_value;
  m_value           = (m_value == m_max) ? 0 : m_value + 1;
  return ret;
}

private:
uint8_t m_value;
uint8_t m_max;
};

int RunMain();

} // ns keleqram
