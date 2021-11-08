#include <sstream>
#include <tgbot/tgbot.h>
#include "request.hpp"

namespace keleqram {
using  TimePoint  = std::chrono::time_point<std::chrono::system_clock>;
using  Duration   = std::chrono::seconds;
using  MessagePtr = TgBot::Message::Ptr;

static const int32_t  TELEGRAM_CHAR_LIMIT{4096};
static const uint32_t FIFTH_OF_DAY   {17280};
static TimePoint      initial_time = std::chrono::system_clock::now();

struct DeleteAction
{
  explicit DeleteAction(const std::string& s);

  bool    delete_last;
  bool    delete_back;
  bool    valid;
  int32_t n;
};

/**
  ┌──────────────────────────────────────────────────────────┐
  │░░░░░░░░░░░░░░░░░░░░░░░░░░ Helpers ░░░░░░░░░░░░░░░░░░░░░░░│
  └──────────────────────────────────────────────────────────┘
*/
[[ maybe_unused ]]
static bool ActionTimer(uint32_t duration = FIFTH_OF_DAY)
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

[[ maybe_unused ]]
static std::string FloatToDecimalString(float n)
{
  std::stringstream ss;
  ss << std::fixed << std::setprecision(2) << n;
  return ss.str();
}

[[ maybe_unused ]]
static std::string ToLower(const std::string& s)
{
  std::string t{s};
  std::transform(t.begin(), t.end(), t.begin(), [](char c) { return tolower(c); });
  return t;
}

[[ maybe_unused ]]
static void Hello(TgBot::Bot& bot)
{
  printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
}

[[ maybe_unused ]]
static std::string DecodeHTML(const std::string& text)
{
  std::string                                  decoded{};
  std::unordered_map<std::string, std::string> convert({
    {"&quot;",  "\""},
    {"&apos;",  "'"},
    {"&amp;",   "&"},
    {"&gt;",    ">"},
    {"&lt;",    "<"},
    {"&frasl;", "/"}});

  for (size_t i = 0; i < text.size(); ++i)
  {
    bool flag = false;
    for (const auto& [key, value] : convert)
    {
      if (i + key.size() - 1 < text.size())
      {
        if (text.substr(i, key.size()) == key)
        {
          decoded += value;
          i += key.size() - 1;
          flag = true;
          break;
        }
      }
    }

  if (!flag)
    decoded += text[i];

  }

  return decoded;
}

struct MimeType
{
  std::string name;
  bool        video;

  bool IsVideo() const { return video;    }
  bool IsPhoto() const { return !(video); }
};

[[ maybe_unused ]]
static MimeType GetMimeType(const std::string& path)
{
  const auto it = path.find_last_of('.');
  if (it != std::string::npos)
  {
    const auto extension = ToLower(path.substr(it + 1));

    if (extension == "jpg" || extension == "jpeg")
      return MimeType{"image/jpeg", false};
    else
    if (extension == "png")
      return MimeType{"image/png", false};
    else
    if (extension == "gif")
      return MimeType{"image/gif", false};
    else
    if (extension == "mp4")
      return MimeType{"video/mp4", true};
    else
    if (extension == "mkv")
      return MimeType{"video/mkv", true};
    else
    if (extension == "webm")
      return MimeType{"video/webm", true};
    else
    if (extension == "mpeg")
      return MimeType{"video/mpeg", true};
    else
    if (extension == "mov")
      return MimeType{"video/quicktime", true};
  }
  return MimeType{"unknown", false};
}

[[ maybe_unused ]]
static std::string ExtractTempFilename(const std::string& full_url)
{
  static const char* TEMP_FILE{"temp_file"};

        auto ext_end = full_url.find_first_of('?');
        ext_end      = ext_end == std::string::npos ? full_url.size() : ext_end;
  const auto url     = full_url.substr(0, ext_end);
  const auto ext_beg = url.find_last_of('.');
  const auto ext_len = (ext_beg != url.npos) ? (url.size() - ext_beg) : 0;
  const auto filename = (ext_len > 0) ? TEMP_FILE + url.substr(ext_beg, ext_len) : TEMP_FILE;
  return filename;
}

[[ maybe_unused ]]
static std::string FetchTemporaryFile(const std::string& full_url, const bool verify_ssl = true)
{
  const auto filename   = ExtractTempFilename(full_url);
  const cpr::Response r = cpr::Get(cpr::Url{full_url}, cpr::VerifySsl(verify_ssl));
  SaveToFile(r.text, filename);

  return filename;
}

/**
 * @brief Chunk Message
 *
 * @param   [in]  {std::string} message
 * @returns [out] {std::vector<std::string>}
 */
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

/**
 *
 */
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

  auto decoded_text = DecodeHTML(text);
  return decoded_text;
}

} // ns keleqram
