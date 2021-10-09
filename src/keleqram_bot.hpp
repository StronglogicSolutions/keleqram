#include <stdio.h>
#include <tgbot/tgbot.h>
#include <vector>

namespace keleqram {
using  TimePoint         = std::chrono::time_point<std::chrono::system_clock>;
using  Duration          = std::chrono::seconds;
using  MessagePtr        = TgBot::Message::Ptr;
using  TelegramException = TgBot::TgException;

const extern int64_t DEFAULT_CHAT_ID;

template<typename... Args>
static void log(Args... args)
{
  for (const auto& s : {args...})
    std::cout << s;
  std::cout << std::endl;
}

class KeleqramBot
{
public:
using LongPoll    = TgBot::TgLongPoll;
using TelegramBot = TgBot::Bot;
using TelegramAPI = TgBot::Api;

KeleqramBot(const std::string& token = "");

void SetListeners();
void Poll();
void HandleMessage(MessagePtr message);
void SendMessage(const std::string& text, const int64_t& id = DEFAULT_CHAT_ID);
void SendMedia  (const std::string& url,  const int64_t& id = DEFAULT_CHAT_ID);

private:
bool IsReply(const int32_t& id) const;

TelegramBot          m_bot;
TelegramAPI          m_api;
LongPoll             m_poll;
uint32_t             tx;
uint32_t             rx;
uint32_t             tx_err;
uint32_t             rx_err;
std::vector<int32_t> tx_msg_ids;
};

class kint8_t
{
public:
kint8_t(uint8_t max_value = 2)
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
