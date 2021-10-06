#include <stdio.h>
#include <tgbot/tgbot.h>
#include <vector>

namespace keleqram {
using  TimePoint         = std::chrono::time_point<std::chrono::system_clock>;
using  Duration          = std::chrono::seconds;
using  MessagePtr        = TgBot::Message::Ptr;
using  TelegramException = TgBot::TgException;

bool IsReply(int32_t id);

class KeleqramBot
{
public:
using LongPoll    = TgBot::TgLongPoll;
using TelegramBot = TgBot::Bot;
using TelegramAPI = TgBot::Api;

KeleqramBot();
void Init();
void SetListeners();
void Poll();
void HandleMessage(MessagePtr message);

private:
TelegramBot m_bot;
TelegramAPI m_api;
LongPoll    m_poll;
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
