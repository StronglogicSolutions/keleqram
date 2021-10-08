#include "keleqram_bot.hpp"
#include "request.hpp"
#include <chrono>
#include <sstream>

namespace keleqram {
static TimePoint         initial_time = std::chrono::system_clock::now();
static const char*       START_COMMAND  {"start"};
static const char*       TOKEN          {"2022095039:AAGVKF0fpYkCby7KHJ2dLhEcP_lQziTx7_M"};
static const char*       DEFAULT_REPLY  {"Defeat Global Fascism"};
static const char*       DEFAULT_RETORT {"I hear you, bitch"};
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
  -229652245,
  -261325234,
  -1001499149725
};

const int64_t DEFAULT_CHAT_ID = *(CHAT_IDs);
/**
 * Global var
 */
static kint8_t chat_idx{};

/**
  ┌──────────────────────────────────────────────────────────┐
  │░░░░░░░░░░░░░░░░░░░░░░░░░░ Helpers ░░░░░░░░░░░░░░░░░░░░░░░│
  └──────────────────────────────────────────────────────────┘
*/

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

static std::string ToLower(const std::string& s)
{
  std::string t{s};
  std::transform(t.begin(), t.end(), t.begin(), [](char c) { return tolower(c); });
  return t;
}

static void Hello(TgBot::Bot& bot)
{
  printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
}

static std::string GetMimeType(const std::string& path)
{
  const auto it = path.find_last_of('.');
  if (it != std::string::npos)
  {
    const auto extension = ToLower(path.substr(it + 1));

    if (extension == "jpg" || extension == "jpeg")
      return "image/jpeg";
    else
    if (extension == "png")
      return "image/png";
    else
    if (extension == "gif")
      return "image/gif";
  }

  return "";
}

static std::string ExtractTempFilename(const std::string& full_url)
{
  static const char* TEMP_FILE{"temp_file"};
  auto ext_end = full_url.find_first_of('?');
  ext_end      = ext_end == std::string::npos ? full_url.size() : ext_end;
  auto url     = full_url.substr(0, ext_end);
  auto ext_beg = url.find_last_of('.');
  auto ext_len = (ext_beg != url.npos) ? (url.size() - ext_beg) : 0;

  auto filename = (ext_len > 0) ?
                    TEMP_FILE + url.substr(ext_beg, ext_len) :
                    TEMP_FILE;
}

static std::string FetchTemporaryFile(const std::string& full_url, const bool verify_ssl = true)
{
  auto ext_end = full_url.find_first_of('?');
  ext_end      = ext_end == std::string::npos ? full_url.size() : ext_end;
  auto url     = full_url.substr(0, ext_end);
  auto ext_beg = url.find_last_of('.');
  auto ext_len = (ext_beg != url.npos) ? (url.size() - ext_beg) : 0;

  auto filename = ExtractTempFilename(full_url);


  cpr::Response r = cpr::Get(cpr::Url{full_url}, cpr::VerifySsl(verify_ssl));
  SaveToFile(r.text, filename);

  return filename;
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
KeleqramBot::KeleqramBot(const std::string& token)
: m_bot((token.empty()) ? TOKEN : token),
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
    SendMessage(GetRequest(ZENQUOTE_URL_INDEX), CHAT_IDs[chat_idx++]);
}

/**
 * SetListeners
 */
void KeleqramBot::SetListeners()
{
  m_bot.getEvents().onCommand   (START_COMMAND, [this](MessagePtr message)
  {
    SendMessage("Hi!", message->chat->id);
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
void KeleqramBot::SendMessage(const std::string& text, const int64_t& id)
{
  if (text.empty()) return;

  tx_msg_ids.emplace_back(m_api.sendMessage(id, text)->messageId);
  log("Sent ", text.c_str(), " to " , std::to_string(id).c_str());
}

/**
 * SendPhoto
 *
 * @param [in] {unsigned char*}
 * @param [in] {size_t}
 * @param [in] {int64_t}
 */
void KeleqramBot::SendPhoto(const std::string& url, const int64_t& id)
{
  if (url.empty()) return;

  const auto path = FetchTemporaryFile(url);
  const auto mime = GetMimeType(path);
  if (mime.empty())
    return (void)(log("Couldn't detect mime type"));

  m_api.sendPhoto(id, TgBot::InputFile::fromFile(path, mime));
  log("Uploaded ", url.c_str(), " to " , std::to_string(id).c_str());
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

  if (reply_message && IsReply(reply_message->messageId))
    SendMessage(DEFAULT_RETORT, id);
  else
    SendMessage(HandleRequest(message->text), id);
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
