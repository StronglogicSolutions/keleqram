#include "keleqram_bot.hpp"
#include <chrono>

namespace keleqram {
static const char*       START_COMMAND  {"start"};
static const char*       HELP_COMMAND   {"help"};
static const char*       DELETE_COMMAND {"delete"};
static const char*       TOKEN          {""};
static const char*       DEFAULT_REPLY  {"Defeat Global Fascism"};
static const char*       DEFAULT_RETORT {"I hear you, bitch"};
static const char*       MARKDOWN_MODE  {"Markdown"};
static const uint32_t    KANYE_URL_INDEX   {0};
static const uint32_t    ZENQUOTE_URL_INDEX{1};
static const uint32_t    BTC_URL_INDEX     {2};
static const uint32_t    INSULT_URL_INDEX  {3};
static const uint32_t    WIKI_URL_INDEX    {4};
static const uint32_t    LINK_URL_INDEX    {5};
static const uint32_t    ETH_URL_INDEX     {6};
static const uint32_t    GTRENDS_URL_INDEX {7}; // TODO: Add
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

const int64_t DEFAULT_CHAT_ID = *(CHAT_IDs);
/**
 * Global var
 */
static kint8_t chat_idx{};

/**
  ┌──────────────────────────────────────────────────────────┐
  │░░░░░░░░░░░░░░░░░░░░░░░░░░ Helpers ░░░░░░░░░░░░░░░░░░░░░░░│
  └──────────────────────────────────────────────────────────┘**/
static void LogMessage(const MessagePtr& message)
{
  log(std::string{"User "                  }, message->from->firstName,
      std::string{" from chat "            }, message->chat->title,
      std::string{" with ID "              }, std::to_string(message->chat->id),
      std::string{" said the following: \n"}, message->text);
}
/**
 * Greeting
 *
 * Bot greeting and list of commands
 */
static std::string Greeting(MessagePtr& message)
{
  static const char* BotInfo{
    "**Available commands:**\n```\n"
    "/kanye        - Timeless advice and inspiration\n"
    "/quote        - Quotes from persons that are not Kanye West\n"
    "/btc          - Latest BTC price\n"
    "/link         - Latest LINK price\n"
    "/eth          - Latest Ethereum price\n"
    "/insult       - You deserve what you get\n"
    "/wiki <query> - Search Wikipedia```"};
  const auto& name = (message->newChatMember) ?
                      (message->newChatMember->firstName.empty()) ?
                        message->from->firstName : message->newChatMember->firstName :
                     (message->from) ? message->from->firstName : "the room";
  const auto& room = message->chat->title;
  return "Welcome to " + room + ", " + name + "\n\n" + BotInfo;
};

/**
 * GetRequest
 */
static std::string GetRequest(uint32_t url_index)
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
 * GetWiki
 */
static std::string GetWiki(std::string message)
{
  const auto IsValid = [](const nlohmann::json& json) -> bool
  {
    return (!json.is_null() && json.is_object() && !json["query"]["search"].empty());
  };

  std::string text{};
  const std::string query = StringTools::urlEncode(message.substr(6));
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
  if (ToLower(message).find("/wiki") != std::string::npos)
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
KeleqramBot::KeleqramBot(const std::string& token)
: m_bot((token.empty()) ? TOKEN : token),
  m_api(m_bot.getApi()),
  m_poll(m_bot),
  tx(0),
  rx(0),
  tx_err(0),
  rx_err(0),
  tx_msgs(TXMessages{})
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
    SendMessage(GetRequest(ZENQUOTE_URL_INDEX), CHAT_IDs[chat_idx++]);
}

/**
 * SetListeners
 */
void KeleqramBot::SetListeners()
{
  m_bot.getEvents().onCommand   (START_COMMAND,  [this](MessagePtr message)
  {
    SendMessage("Hi!", message->chat->id);
  });
  m_bot.getEvents().onCommand   (HELP_COMMAND,   [this](MessagePtr message)
  {
    SendMessage(Greeting(message), message->chat->id, MARKDOWN_MODE);
  });
  m_bot.getEvents().onCommand   (DELETE_COMMAND, [this](MessagePtr message)
  {
    DeleteMessages(message);
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
bool KeleqramBot::IsReply(const int64_t& chat_id, const int32_t& id)
{
  for (const auto& message_id : tx_msgs[chat_id])
    if (message_id == id) return true;
  return false;
}

template <typename T = int64_t>
static const int64_t ValidateID(const T& id)
{
  static const size_t MIN_SIZE{5};
  if constexpr (std::is_integral<T>::value)
    return id;
  else
  if constexpr (std::is_same_v<T, std::string>)
    return (id.size() < MIN_SIZE) ? DEFAULT_CHAT_ID : std::stoll(id);
  else
    return DEFAULT_CHAT_ID;
}

/**
 * SendMessage
 *
 * @param [in] {int64_t}
 * @param [in] {std::string}
 */
template<typename T>
void KeleqramBot::SendMessage(const std::string& message, const T& id, const std::string& parse_mode)
{
  using ReplyPtr = TgBot::GenericReply::Ptr;

  if (message.empty()) return;

  static const bool     PreviewsActive{false};
  static const int32_t  NoReplyID     {0};
  static const ReplyPtr NoInterface   {nullptr};
         const int64_t  dest    =     ValidateID(id);

  for (const auto& text : ChunkMessage(message))
  {
    tx_msgs[dest].emplace_back(m_api.sendMessage(dest, text, PreviewsActive, NoReplyID, NoInterface, parse_mode)->messageId);
    log("Sent ", text.c_str(), " to " , std::to_string(dest).c_str());
  }
}

/**
 * SendMedia
 *
 * @param [in] {unsigned char*}
 * @param [in] {size_t}
 * @param [in] {int64_t}
 */
template<typename T>
void KeleqramBot::SendMedia(const std::string& url,  const T& id)
{
  if (url.empty()) return;
  const int64_t dest = ValidateID(id);
  const auto    path = FetchTemporaryFile(url);
  const auto    mime = GetMimeType(path);
  if (mime.name.empty())
    return (void)(log("Couldn't detect mime type"));

  if (mime.IsPhoto())
    m_api.sendPhoto(dest, TgBot::InputFile::fromFile(path, mime.name));
  else
    m_api.sendVideo(dest, TgBot::InputFile::fromFile(path, mime.name));
  log("Uploaded ", url.c_str(), " to " , std::to_string(dest).c_str());
}

/**
 * HandleMessage
 *
 * @param [in] {MessagePtr}
 */
void KeleqramBot::HandleMessage(MessagePtr message)
{
  const auto IsEvent = [](const MessagePtr& message) -> bool { return message->text.empty(); };

  const MessagePtr reply_message = message->replyToMessage;
  const int64_t&   id            = message->chat->id;
  LogMessage(message);

  if (reply_message && IsReply(id, reply_message->messageId))
    SendMessage(DEFAULT_RETORT, id);
  else
  if (IsEvent(message))
    HandleEvent(message);
  else
    SendMessage(HandleRequest(message->text), id);
}

/**
 * HandleEvent
 *
 * @param [in] {MessagePtr}
 */
void KeleqramBot::HandleEvent(MessagePtr message)
{
  if (message->newChatMember)
    SendMessage(Greeting(message), message->chat->id, MARKDOWN_MODE);
  else
  if (message->leftChatMember)
    SendMessage("Good riddance", message->chat->id);
}

void KeleqramBot::DeleteMessages(MessagePtr message)
{
  using ChatMsgs = std::vector<int32_t>;

  const auto GetNum = [](const std::string& s) -> DeleteAction
  {
    return DeleteAction(s);
  };

  const int64_t     chat_id  = message->chat->id;
  const auto        action   = GetNum(message->text);
        ChatMsgs&   messages = tx_msgs[chat_id];
  if (action.valid && messages.size())
  {
    auto it = (messages.end() - action.n);

    m_api.deleteMessage(chat_id, *(it));
    it = messages.erase(it);

    if (action.delete_last)
    {
      while (it != messages.end())
      {
        m_api.deleteMessage(chat_id, *(it));
        it = messages.erase(it);
      }
    }
  }
}

/**
  ┌──────────────────────────────────────────────────────────┐
  │░░░░░░░░░░░░░░░░░░░░░░ Specializations ░░░░░░░░░░░░░░░░░░░░│
  └──────────────────────────────────────────────────────────┘
*/

template void KeleqramBot::SendMessage(const std::string& text,
                                       const std::string& id,
                                       const std::string& parse_mode = "");
template void KeleqramBot::SendMessage(const std::string& text,
                                       const int64_t&     id,
                                       const std::string& parse_mode = "");
template void KeleqramBot::SendMedia  (const std::string& url,  const std::string& id);
template void KeleqramBot::SendMedia  (const std::string& url,  const int64_t&     id);


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
