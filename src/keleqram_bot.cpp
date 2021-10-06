#include "keleqram_bot.hpp"
#include "request.hpp"
#include <chrono>
#include <sstream>

namespace keleqram {
static TimePoint         initial_time = std::chrono::system_clock::now();
static const char*       START_COMMAND  {"start"};
static const char*       TOKEN          {""};
static const uint32_t    THIRTY_MINS    {1800};
static const uint32_t    KANYE_URL_INDEX   {0};
static const uint32_t    ZENQUOTE_URL_INDEX{1};
static const uint32_t    BTC_URL_INDEX     {2};
static const uint32_t    INSULT_URL_INDEX  {3};
static const uint32_t    WIKI_API_INDEX    {4};
static const uint32_t    LINK_URL_INDEX    {5};
static const uint32_t    ETH_URL_INDEX     {6};
static const char*       URLS[] {
  "https://api.kanye.rest/",
  "https://zenquotes.io/api/random",
  "https://blockchain.info/ticker",
  "https://evilinsult.com/generate_insult.php?lang=en&type=json",
  "https://en.wikipedia.org/w/api.php?action=query&list=search&srsearch=Roy%20Nelson&utf8=&format=json",
  "https://min-api.cryptocompare.com/data/pricemultifull?fsyms=LINK&tsyms=USD",
  "https://min-api.cryptocompare.com/data/pricemultifull?fsyms=ETH&tsyms=USD"
};
static const char*       CHAT_IDs[] {

};

/**
 * Global var
 */
static std::vector<int32_t> sent_message_ids{};
static kint8_t              chat_room_idx   {};

/**
 * Utils
 */
template<typename... Args>
inline void log(Args... args) {
  for (const auto& s : {args...})
    std::cout << s;
  std::cout << std::endl;
}

static void LogMessage(const MessagePtr& message)
{
  log(std::string{"User "                  }, message->from->firstName,
      std::string{" from chat "            }, std::to_string(message->chat->id),
      std::string{" said the following: \n"}, message->text);
}

bool ActionTimer(uint32_t duration = THIRTY_MINS)
{
  const TimePoint now = std::chrono::system_clock::now();
  const int64_t   elapsed = std::chrono::duration_cast<Duration>(now - initial_time).count();

  if (elapsed > duration)
  {
    initial_time = now;
    return true;
  }

  return false;
}

static std::string FloatToDecimalString(float n)
{
  std::stringstream ss;
  ss << std::fixed << std::setprecision(2) << n;
  return ss.str();
}

static std::string ToLower(std::string& s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](char c) { return tolower(c); });
  return s;
}

bool IsReply(int32_t id)
{
  for (const auto& message_id : sent_message_ids)
    if (message_id == id)
      return true;
  return false;
}

static void Hello(TgBot::Bot& bot)
{
  printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
}

/**
 * GetRequest
 */
std::string GetRequest(uint32_t url_index)
{
  using namespace keleqram;
  std::string text{};
  RequestResponse response{cpr::Get(cpr::Url{URLS[url_index]}, cpr::VerifySsl{false})};
  if (!response.error)
  {
    switch (url_index)
    {
      case (KANYE_URL_INDEX):
      {
        auto json = response.json();
        if (!json.is_null() && json.is_object())
          text += "Kanye says: \"" + json["quote"].get<std::string>() + '\"';
      }
      break;
      case (ZENQUOTE_URL_INDEX):
      {
        auto json = response.json();
        if (!json.is_null() && json.is_array())
          text += json[0]["q"].get<std::string>();
      }
      break;
      case (INSULT_URL_INDEX):
      {
        auto json = response.json();
        if (!json.is_null() && json.is_object())
          text += json["insult"].get<std::string>();
      }
      break;
      case (BTC_URL_INDEX):
      {
        auto json = response.json();
        if (!json.is_null() && json.is_object())
          text += "BTC: $" + std::to_string(json["USD"]["last"].get<int32_t>()) + " USD";
      }
      break;
      case (LINK_URL_INDEX):
      {
        auto json = response.json();
        if (!json.is_null() && json.is_object())
          text += "LINK: $" + FloatToDecimalString(json["RAW"]["LINK"]["USD"]["PRICE"].get<float>()) + " USD";
      }
      break;
      case (ETH_URL_INDEX):
      {
        auto json = response.json();
        if (!json.is_null() && json.is_object())
          text += "ETH: $" + FloatToDecimalString(json["RAW"]["ETH"]["USD"]["PRICE"].get<float>()) + " USD";
      }
      break;
    }
  }

  return text;
}

/**
 * HandleRequest
 */
static std::string HandleRequest(std::string message)
{
  if (message.find("@KIQ_TelegramBot")         != std::string::npos)
    return "Defeat Global Fascism";
  else
  if (ToLower(message).find("/kanye") != std::string::npos)
    return GetRequest(KANYE_URL_INDEX);
  if (ToLower(message).find("/quote") != std::string::npos)
    return GetRequest(ZENQUOTE_URL_INDEX);
  if (ToLower(message) == "/btc")
    return GetRequest(BTC_URL_INDEX);
  if (ToLower(message) == "/link")
    return GetRequest(LINK_URL_INDEX);
  if (ToLower(message) == "/eth")
    return GetRequest(ETH_URL_INDEX);
  if (ToLower(message) == "/insult")
    return GetRequest(INSULT_URL_INDEX);
  return "";
}

KeleqramBot::KeleqramBot()
: m_bot(TOKEN),
  m_api(m_bot.getApi()),
  m_poll(m_bot)
{
  Hello(m_bot);
}

void KeleqramBot::Poll()
{
  log("Polling");
  m_poll.start();
  if (ActionTimer())
    m_api.sendMessage(CHAT_IDs[chat_room_idx++], GetRequest(ZENQUOTE_URL_INDEX));
}

void KeleqramBot::Init()
{
  SetListeners();
}

void KeleqramBot::SetListeners()
{
  m_bot.getEvents().onCommand   (START_COMMAND, [this](MessagePtr message)
  {
       m_api.sendMessage(message->chat->id, "Hi!"); }
  );
  m_bot.getEvents().onAnyMessage([this](MessagePtr message)
  {
    HandleMessage(message);
  });
}

void KeleqramBot::HandleMessage(MessagePtr message)
{
  const auto reply_message = message->replyToMessage;

  if (StringTools::startsWith(message->text, "/start")) return;

  LogMessage(message);

  if (reply_message && IsReply(reply_message->messageId))
    sent_message_ids.emplace_back(m_api.sendMessage(message->chat->id, "I hear you, bitch")->messageId);
  else
  {
    const std::string text = HandleRequest(message->text);
      if (!text.empty())
        sent_message_ids.emplace_back(m_api.sendMessage(message->chat->id, text)->messageId);
  }
}


int RunMain()
{
  try
  {
    KeleqramBot k_bot{};
    k_bot.Init();

    for (;;)
      k_bot.Poll();
  }
  catch (TelegramException& e)
  {
    log("Exception: ", e.what());
  }
  return 0;
}

} // ns keleqram
