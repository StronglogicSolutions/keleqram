#include <sstream>
#include <cctype>
#include <random>

#include <tgbot/tgbot.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include "INIReader.h"
#include <kutils.hpp>

namespace keleqram {
using  TimePoint  = std::chrono::time_point<std::chrono::system_clock>;
using  Duration   = std::chrono::seconds;
using  MessagePtr = TgBot::Message::Ptr;
using  TXMessages = std::map<int64_t, std::vector<int32_t>>;
//-------------------------------------------------------------
static INIReader      config             {""};
static const char*    BOT_SECTION        {"bot"};
static const char*    GREETING_SECTION   {"greeting"};
static const char*    ROOMS_SECTION      {"rooms"};
static const char*    CUSTOM_URL_SECTION {"custom_url"};
static const int32_t  TELEGRAM_CHAR_LIMIT{4096};
static const uint32_t QUARTER_DAY        {21600};
static TimePoint      initial_time = std::chrono::system_clock::now();
//-------------------------------------------------------------
static std::string GetConfigValue(const std::string& section, const std::string& key, const std::string& fallback = "")
{
  return config.GetString(section, key, fallback);
}
//-------------------------------------------------------------
struct DeleteAction
{
  explicit DeleteAction(const std::string& s);

  bool   delete_last;
  bool   delete_back;
  bool   valid;
  size_t n;
};
//-------------------------------------------------------------
struct Room
{
  int64_t     id;
  std::string name;
  long        url_index{ -1 };

  std::string serialize() const { return std::to_string(id) + ',' + name; }

  static Room deserialize(const std::string& room_s)
  {
    Room room;
    if (auto idx = room_s.find(',') != std::string::npos)
    {
      room.id   = std::stoll(room_s.substr(0, idx - 1));
      room.name = room_s.substr(idx + 1);
    }
    return room;
  }

  bool has_url() const { return url_index > -1; }
};
using Rooms = std::vector<Room>;
/**
  ┌──────────────────────────────────────────────────────────┐
  │░░░░░░░░░░░░░░░░░░░░░░░░░░ Helpers ░░░░░░░░░░░░░░░░░░░░░░░│
  └──────────────────────────────────────────────────────────┘
*/
[[ maybe_unused ]]
static bool ActionTimer(uint32_t duration = QUARTER_DAY)
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
//-------------------------------------------------------------
[[ maybe_unused ]]
static std::string GetExecutableCWD()
{
  std::string full_path{realpath("/proc/self/exe", NULL)};
  return full_path.substr(0, full_path.size() - (13));
}
//-------------------------------------------------------------
[[ maybe_unused ]]
static bool IsAllNum(const std::string& s)
{
  for (const char& c : s)
    if (!std::isdigit(c)) return false;
  return true;
}
//-------------------------------------------------------------
[[ maybe_unused ]]
static void Hello(TgBot::Bot& bot)
{
  printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
}
//-------------------------------------------------------------
[[ maybe_unused ]]
static std::string ExtractTempFilename(const std::string& full_url)
{
  static const char* TEMP_FILE{"temp_file"};

        auto ext_end  = full_url.find_first_of('?');
             ext_end  = ext_end == std::string::npos ? full_url.size() : ext_end;
  const auto url      = full_url.substr(0, ext_end);
  const auto ext_beg  = url.find_last_of('.');
  const auto ext_len  = (ext_beg != url.npos) ? (url.size() - ext_beg) : 0;
  const auto filename = (ext_len > 0) ? TEMP_FILE + url.substr(ext_beg, ext_len) : TEMP_FILE;
  return filename;
}
//-------------------------------------------------------------
[[ maybe_unused ]]
static std::string FetchTemporaryFile(const std::string& full_url, const bool verify_ssl = true)
{
  (void)(verify_ssl);
  const auto filename   = ExtractTempFilename(full_url);
  const cpr::Response r = cpr::Get(cpr::Url{full_url});
  std::ofstream o{filename};
  o << r.text;
  return filename;
}
//-------------------------------------------------------------
[[ maybe_unused ]]
static std::vector<std::string> const ChunkMessage(const std::string& message) {
  static const uint32_t MAX_CHUNK_SIZE = TELEGRAM_CHAR_LIMIT - 6;

  std::vector<std::string>     chunks{};
  const std::string::size_type message_size {message.size()};
  const std::string::size_type num_of_chunks{message.size() / MAX_CHUNK_SIZE + 1};
  uint32_t                     chunk_index  {1};
  std::string::size_type       bytes_chunked{ };

  if (num_of_chunks > 1)
  {
    chunks.reserve(num_of_chunks);
    while (bytes_chunked < message_size)
    {
      const std::string::size_type size_to_chunk =
        (bytes_chunked + MAX_CHUNK_SIZE > message_size) ?
          (message_size - bytes_chunked) :
          MAX_CHUNK_SIZE;

      std::string oversized_chunk = message.substr(bytes_chunked, size_to_chunk);

      const std::string::size_type ws_idx = oversized_chunk.find_last_of(" ") + 1;
      const std::string::size_type pd_idx = oversized_chunk.find_last_of(".") + 1;
      const std::string::size_type index  = (size_to_chunk > MAX_CHUNK_SIZE) ?
         (ws_idx > pd_idx) ?  ws_idx :
                              pd_idx
                           :
         size_to_chunk;

      chunks.emplace_back((!index) ?
        oversized_chunk :
        oversized_chunk.substr(0, index) + '\n' +
        std::to_string(chunk_index++)    + '/'  + std::to_string(num_of_chunks));

      bytes_chunked += index;
    }
  }
  else
    chunks.emplace_back(message);

  return chunks;
}
//-------------------------------------------------------------
[[ maybe_unused ]]
static std::string ExtractWikiText(const nlohmann::json& json)
{
  std::string text{};
  if (!json.is_null() && json.is_object() && json.contains("query"))
  {
    for (const auto& item : json["query"]["search"])
    {
      std::string s = item["snippet"].get<std::string>();
      for (auto it = s.find("</span>"); it != std::string::npos;)
      {
        s   = s.substr(it + 7);
        it  = s.find("</span>");
      }
      text += s;
    }
  }

  auto decoded_text = kutils::DecodeHTML(text);
  return decoded_text;
}
//-------------------------------------------------------------
[[ maybe_unused ]]
static int32_t GetRandom(const int32_t& min, const int32_t& max)
{
  static std::random_device                     rd;
  static std::default_random_engine             e{rd()};
  static std::uniform_int_distribution<int32_t> dis(min, max);
  return dis(e);
};
//-------------------------------------------------------------
[[ maybe_unused ]]
static std::vector<Room> GetConfigRooms()
{
  std::vector<Room> rooms;
  int  i   = 0;
  auto v   = config.GetString(ROOMS_SECTION, std::to_string(i), "");
  auto idx = v.find(',');
  while (idx != std::string::npos)
  {
    const auto key = v.substr(0, idx);
    const auto room_id = v.substr(idx + 1);
    rooms.push_back(Room{ std::stoll(key), room_id, config.GetInteger(CUSTOM_URL_SECTION, key, -1) });
    v = config.GetString(ROOMS_SECTION, std::to_string(i++), "");
    idx = v.find(',');
  }

  return rooms;
}
//-------------------------------------------------------------
[[ maybe_unused ]]
static void SetReplies(std::vector<std::string>& replies)
{
  std::string replies_s = GetConfigValue(BOT_SECTION, "replies");
  for (const auto& reply : StringTools::split(replies_s, '|'))
    replies.emplace_back(reply);
}
//-------------------------------------------------------------
[[ maybe_unused ]]
static nlohmann::json get_json(const cpr::Response& r)
{
  return nlohmann::json::parse(r.text, nullptr, false);
}
//-------------------------------------------------------------
static void SaveMessages(const TXMessages& msgs)
{
  nlohmann::json json;
  json = msgs;
  kutils::SaveToFile(json.dump(), "messages.json");
}
//-------------------------------------------------------------
static TXMessages GetSavedMessages()
{
  nlohmann::json json;
  TXMessages     msgs;
  if (const auto db = kutils::ReadFile("messages.json"); !db.empty())
  {
    json = db;
    int64_t key;
    if (!json.is_null() && json.is_string())
      json = nlohmann::json::parse(json.get<std::string>());
    kutils::log(json.dump());
    for (const auto& room_data : json)
      for (const auto& value : room_data)
      {
        if (!value.is_array())
          key = value.get<int64_t>();
        else
          for (const auto& id : value)
            msgs[key].push_back(id.get<int32_t>());
      }
  }
  return msgs;
}
} // ns keleqram
