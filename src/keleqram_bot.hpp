#include <vector>
#include <unordered_map>
#include "util.hpp"

namespace keleqram {
using  TelegramException = TgBot::TgException;
extern int64_t DEFAULT_CHAT_ID;

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

using ObserverFn = void(*)(const std::vector<std::string>&);
using Observers  = std::vector<ObserverFn>;

class KeleqramBot
{
public:
using LongPoll    = TgBot::TgLongPoll;
using TelegramBot = TgBot::Bot;
using TelegramAPI = TgBot::Api;
using Replies     = std::vector<std::string>;
using Payload     = std::vector<std::string>;

KeleqramBot(const std::string& token = "");

void        SetListeners();
void        Poll();
void        HandleMessage(MessagePtr message);
void        HandlePrivateRequest(MessagePtr message);
void        HandleEvent(MessagePtr message);
template<typename T>
void        SendMessage(const std::string& text, const T& id, const std::string& parse_mode = "");
template<typename T>
void        SendMedia  (const std::string& url,  const T& id);
template<typename T>
std::string SendPoll   (const std::string& text, const T& id, const std::vector<std::string>& options);
std::string StopPoll   (const std::string& id, const std::string& poll_id);
void        DeleteMessages(MessagePtr message);
template <typename T>
void        DeleteMessages(const T& id, const DeleteAction& action);
void        AddObserver(ObserverFn fn_ptr);
std::string GetRooms() const;

private:
bool        IsReply(const int64_t& chat_id, const int32_t& id);


TelegramBot m_bot;
TelegramAPI m_api;
LongPoll    m_poll;
uint32_t    tx;
uint32_t    rx;
uint32_t    tx_err;
uint32_t    rx_err;
TXMessages  tx_msgs;
Replies     m_replies;
Observers   m_observers;
Rooms       m_rooms;
};

template <typename T = size_t>
class kint_t
{
public:
kint_t(T max_value = 4)
: m_value(0),
  m_max(max_value)
{}


T operator++(int)
{
  const T ret = m_value;
  m_value     = (m_value == m_max) ? 0 : m_value + 1;
  return ret;
}

private:
T m_value;
T m_max;
};

int RunMain();

} // ns keleqram
