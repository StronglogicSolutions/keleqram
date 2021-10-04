#include "keleqram_bot.hpp"

static std::vector<int32_t> sent_message_ids{};

bool IsReply(int32_t id)
{
  for (const auto& message_id : sent_message_ids)
    if (message_id == id)
      return true;
  return false;
}

int RunMain()
{
    TgBot::Bot bot("TOKEN");
    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message)
    {
        bot.getApi().sendMessage(message->chat->id, "Hi!");
    });
    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message)
    {
        printf("User wrote %s\n", message->text.c_str());
        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }
        auto reply = message->replyToMessage;
        if (reply && IsReply(reply->messageId))
        {
           auto outbound_message = bot.getApi().sendMessage(message->chat->id, "I hear you, bitch");
           sent_message_ids.emplace_back(outbound_message->messageId);
        }
        else
        {
          auto it = message->text.find("@KIQ_TelegramBot");
          if (it != std::string::npos)
          {
            auto outbound_message = bot.getApi().sendMessage(message->chat->id, "Defeat Global Fascism");
            sent_message_ids.emplace_back(outbound_message->messageId);
          }
        }
    });
    try
    {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgBot::TgLongPoll longPoll(bot);
        while (true)
        {
            printf("Long poll started\n");
            longPoll.start();
        }
    }
    catch (TgBot::TgException& e)
    {
        printf("error: %s\n", e.what());
    }
    return 0;
}
