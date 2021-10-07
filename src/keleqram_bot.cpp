#include "keleqram_bot.hpp"
#include "request.hpp"
#include <chrono>
#include <sstream>

namespace keleqram {
static TimePoint         initial_time = std::chrono::system_clock::now();
static const char*       START_COMMAND  {"start"};
static const char*       TOKEN          {""};
static const char*       DEFAULT_REPLY  {"Defeat Global Fascism"};
static const uint32_t    THIRTY_MINS    {1800};
static const uint32_t    KANYE_URL_INDEX   {0};
static const uint32_t    ZENQUOTE_URL_INDEX{1};
static const uint32_t    BTC_URL_INDEX     {2};
static const uint32_t    INSULT_URL_INDEX  {3};
static const uint32_t    WIKI_URL_INDEX    {4};
static const uint32_t    LINK_URL_INDEX    {5};
static const uint32_t    ETH_URL_INDEX     {6};
static const char*       URLS[] {
  "https://api.kanye.rest/",
  "https://zenquotes.io/api/random",
  "https://blockchain.info/ticker",
  "https://evilinsult.com/generate_insult.php?lang=en&type=json",
  "https://en.wikipedia.org/w/api.php?action=query&utf8=&format=json&list=search&srsearch=",
  "https://min-api.cryptocompare.com/data/pricemultifull?fsyms=LINK&tsyms=USD",
  "https://min-api.cryptocompare.com/data/pricemultifull?fsyms=ETH&tsyms=USD"
};
static const int64_t     CHAT_IDs[] {

};

/**
 * Global var
 */
static kint8_t chat_idx{};

/**
  ┌──────────────────────────────────────────────────────────┐
  │░░░░░░░░░░░░░░░░░░░░░░░░░░ Helpers ░░░░░░░░░░░░░░░░░░░░░░░│
  └──────────────────────────────────────────────────────────┘
*/
template<typename... Args>
static void log(Args... args) {
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

static bool ActionTimer(uint32_t duration = THIRTY_MINS)
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
 *
 */
static std::string ExtractWikiText(const nlohmann::json& json)
{
  const auto  item = json["query"]["search"][0];
  std::string s    = item["snippet"].get<std::string>();

  for (auto it = s.find("</span>"); it != std::string::npos;)
  {
    s   = s.substr(it + 7);
    it  = s.find("</span>");
  }

  return item["title"].get<std::string>() + ":\n" + s;
}

/**
 *
 */
static std::string GetWiki(std::string message)
{
  const auto IsValid = [](const nlohmann::json& json) -> bool
  {
    return (!json.is_null() && json.is_object() && !json["query"]["search"].empty());
  };

    std::string text{};
    const std::string query = StringTools::urlEncode(message.substr(8));
    RequestResponse   response{cpr::Get(cpr::Url{URLS[WIKI_URL_INDEX]} + query, cpr::VerifySsl{false})};
    if (!response.error)
    {
      auto json = response.json();
      if (IsValid(json))
        text +=  ExtractWikiText(json);
      else
        text += query + " was not found.";
    }
    return text;
}

/**
 * HandleRequest
 * @static
 *
 * @param [in] {std::string}
 */
static std::string HandleRequest(std::string message)
{
  if (message.find("@KIQ_TelegramBot")         != std::string::npos)
    return DEFAULT_REPLY;
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
  if (ToLower(message).find("/person") != std::string::npos)
    return GetWiki(message);
  return "";
}

/**
  ┌────────────────────────────────────────────────────────┐
  │░░░░░░░░░░░░░░░░░░░░░░ KeleqramBot ░░░░░░░░░░░░░░░░░░░░░│
  └────────────────────────────────────────────────────────┘
*/

/**
 * KeleqramBot
 *
 * @constructor
 */
KeleqramBot::KeleqramBot()
: m_bot(TOKEN),
  m_api(m_bot.getApi()),
  m_poll(m_bot)
{
  Hello(m_bot);
  SetListeners();
}

/**
 * Poll
 */
void KeleqramBot::Poll()
{
  m_poll.start();
  if (ActionTimer())
    SendMessage(CHAT_IDs[chat_idx++], GetRequest(ZENQUOTE_URL_INDEX));
}

/**
 * SetListeners
 */
void KeleqramBot::SetListeners()
{
  m_bot.getEvents().onCommand   (START_COMMAND, [this](MessagePtr message)
  {
    SendMessage(message->chat->id, "Hi!");
  });
  m_bot.getEvents().onAnyMessage([this](MessagePtr message)
  {
    HandleMessage(message);
  });
}

/**
 * IsReply
 *
 * @param [in] {int32_t}
 */
bool KeleqramBot::IsReply(const int32_t& id) const
{
  for (const auto& message_id : tx_msg_ids)
    if (message_id == id) return true;
  return false;
}

/**
 * SendMessage
 *
 * @param [in] {int64_t}
 * @param [in] {std::string}
 */
void KeleqramBot:: SendMessage(const int64_t& id, const std::string& text)
{
  if (!text.empty())
    tx_msg_ids.emplace_back(m_api.sendMessage(id, text)->messageId);
}

/**
 * HandleMessage
 *
 * @param [in] {MessagePtr}
 */
void KeleqramBot::HandleMessage(MessagePtr message)
{
  const MessagePtr reply_message = message->replyToMessage;
  const int64_t&   id            = message->chat->id;
  LogMessage(message);

  if (StringTools::startsWith(message->text, "/start"))
    (void)(0);
  else
  if (reply_message && IsReply(reply_message->messageId))
    SendMessage(id, "I hear you, bitch");
  else
    SendMessage(id, HandleRequest(message->text));
}


/**
 * RunMain
 *
 * Program entry
 */
int RunMain()
{
  try
  {
    KeleqramBot k_bot{};

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
