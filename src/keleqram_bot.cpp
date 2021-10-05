#include "keleqram_bot.hpp"
#include "request.hpp"
#include <chrono>

using  TimePoint  = std::chrono::time_point<std::chrono::system_clock>;
using  Duration   = std::chrono::seconds;
using  MessagePtr = TgBot::Message::Ptr;

static TimePoint         initial_time = std::chrono::system_clock::now();
static const std::string KSTYLEYOCHAT   {"KSTYLEYO"};
static const char*       START_COMMAND  {"start"};
static const char*       TOKEN          {};
static const uint32_t    THIRTY_MINS    {1800};
static const uint32_t    KANYE_URL_INDEX   {0};
static const uint32_t    ZENQUOTE_URL_INDEX{1};
const char* URLS[] {
  "https://api.kanye.rest/",
  "https://zenquotes.io/api/random"
};

/**
 * Global var
 */
static std::vector<int32_t> sent_message_ids{};

/**
 * Utils
 */
template<typename... Args>
inline void log(Args... args) {
  for (const auto& s : {args...})
    std::cout << s;
  std::cout << std::endl;
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
  if (ToLower(message).find("kanye")           != std::string::npos)
    return GetRequest(KANYE_URL_INDEX);
  if (ToLower(message).find("inspiring quote") != std::string::npos)
    return GetRequest(ZENQUOTE_URL_INDEX);
  return "";
}

/**
 * HandleMessage
 */
static void HandleMessage(TgBot::Api& api, MessagePtr message)
{
  const auto reply_message = message->replyToMessage;

  if (StringTools::startsWith(message->text, "/start")) return;

  printf("User %s from chat %s said the following:\n%s\n",
    message->from->username.c_str(), message->chat->title.c_str(), message->text.c_str());

  if (reply_message && IsReply(reply_message->messageId))
    sent_message_ids.emplace_back(api.sendMessage(message->chat->id, "I hear you, bitch")->messageId);
  else
  {
    std::string text = HandleRequest(message->text);
      if (!text.empty())
        sent_message_ids.emplace_back(api.sendMessage(message->chat->id, text)->messageId);
  }
}

int RunMain()
{
  TgBot::Bot                     bot{TOKEN};
  static TgBot::EventBroadcaster events = bot.getEvents();
  TgBot::Api                     api    = bot.getApi();

  events.onCommand   (START_COMMAND, [&api](MessagePtr message) { api.sendMessage(message->chat->id, "Hi!"); });
  events.onAnyMessage([&api](MessagePtr message)                { HandleMessage(api, message);               });

  try
  {
    Hello(bot);
    TgBot::TgLongPoll longPoll{bot};

    for (;;)
    {
      log("Polling");
      longPoll.start();
      if (ActionTimer())
        bot.getApi().sendMessage(KSTYLEYOCHAT, GetRequest(ZENQUOTE_URL_INDEX));
    }
  }
  catch (TgBot::TgException& e)
  {
    log("Exception: ", e.what());
  }
  return 0;
}
