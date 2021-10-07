#pragma once

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>


namespace keleqram {
class request_error : public std::logic_error {
public:
explicit request_error(const std::string message)
: std::logic_error(message),
  error_message(message) {}
virtual ~request_error() override {}

virtual const char* what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW {
  return error_message.c_str();
}

private:
std::string error_message;
};

inline std::string GetRequestError(cpr::ErrorCode code) {
  if (code == cpr::ErrorCode::OK)
                      return "OK";
  if (code == cpr::ErrorCode::CONNECTION_FAILURE)
                      return "CONNECTION_FAILURE";
  if (code == cpr::ErrorCode::EMPTY_RESPONSE)
                      return "EMPTY_RESPONSE";
  if (code == cpr::ErrorCode::HOST_RESOLUTION_FAILURE)
                      return "HOST_RESOLUTION_FAILURE";
  if (code == cpr::ErrorCode::INTERNAL_ERROR)
                      return "INTERNAL_ERROR";
  if (code == cpr::ErrorCode::INVALID_URL_FORMAT)
    return "INVALID_URL_FORMAT";
  if (code == cpr::ErrorCode::NETWORK_RECEIVE_ERROR)
                      return "NETWORK_RECEIVE_ERROR";
  if (code == cpr::ErrorCode::NETWORK_SEND_FAILURE)
                      return "NETWORK_SEND_FAILURE";
  if (code == cpr::ErrorCode::OPERATION_TIMEDOUT)
                      return "OPERATION_TIMEDOUT";
  if (code == cpr::ErrorCode::PROXY_RESOLUTION_FAILURE)
                      return "PROXY_RESOLUTION_FAILURE";
  if (code == cpr::ErrorCode::SSL_CONNECT_ERROR)
                      return "SSL_CONNECT_ERROR";
  if (code == cpr::ErrorCode::SSL_LOCAL_CERTIFICATE_ERROR)
                      return "SSL_LOCAL_CERTIFICATE_ERROR";
  if (code == cpr::ErrorCode::SSL_REMOTE_CERTIFICATE_ERROR)
                      return "SSL_REMOTE_CERTIFICATE_ERROR";
  if (code == cpr::ErrorCode::SSL_CACERT_ERROR)
                      return "SSL_CACERT_ERROR";
  if (code == cpr::ErrorCode::GENERIC_SSL_ERROR)
                      return "GENERIC_SSL_ERROR";
  if (code == cpr::ErrorCode::UNSUPPORTED_PROTOCOL)
                      return "UNSUPPORTED_PROTOCOL";
  // if (code == cpr::ErrorCode::REQUEST_CANCELLED)
  //                     return "REQUEST_CANCELLED";
  return "UNKNOWN_ERROR";
}

struct RequestResponse {
public:
cpr::Response response;
bool          error;

RequestResponse(cpr::Response r)
: response(r),
  error(r.status_code >= 400)
{}

nlohmann::json json() const {
  return nlohmann::json::parse(response.text, nullptr, false);
}

std::string text() const {
  return response.text;
}

const std::string GetError() const {
  if (error) {
    std::string error_message{response.error.message};
    std::string error_code_message = GetRequestError(response.error.code);
    std::string error_code = std::to_string(static_cast<uint32_t>(response.error.code));

    return std::string{
      "Status: "         + std::to_string(response.status_code) +
      "\nText: "         + response.text +
      "\nError Message:" + error_message +
      "\nError Code: "   + error_code +
      "\nError: "        + error_code_message
    };
  }
  return "";
}
};

static void SaveToFile(const std::string& data, const std::string& path)
{
  std::ofstream o{path};
  o << data;
}

} // namespace kwitter
