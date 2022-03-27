#include "request.hpp"
#include "util.hpp"
#include <sstream>

namespace keleqram {
DeleteAction::DeleteAction(const std::string& s)
{
  static const size_t  npos = std::string::npos;
  static const char*   prefix_last{"/delete last "};
  static const char*   prefix_back{"/delete back "};
  static const size_t  prefix_length{13};
  const size_t idx   = s.find(prefix_last);
  delete_last = (idx != npos);
  delete_back = (!delete_last) ? (s.find(prefix_back) != npos) : false;

  if (delete_last || delete_back)
  {
    valid           = true;
    const auto& rem = s.substr(prefix_length);
    if (isdigit(rem.front())) n = std::stoul(rem);
  }
}
} // ns keleqram
