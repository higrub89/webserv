#include "HttpResponse.hpp"

#include <sstream>

namespace {
// Case-insensitive comparison for header keys (RFC 7230).
bool equalsIgnoreCase(const std::string& a, const std::string& b) {
  if (a.size() != b.size())
    return false;
  for (std::string::size_type i = 0; i < a.size(); ++i) {
    char ca = a[i];
    char cb = b[i];
    if (ca >= 'A' && ca <= 'Z')
      ca = static_cast<char>(ca - 'A' + 'a');
    if (cb >= 'A' && cb <= 'Z')
      cb = static_cast<char>(cb - 'A' + 'a');
    if (ca != cb)
      return false;
  }
  return true;
}
}  // namespace

HttpResponse::HttpResponse() : statusCode_(200), statusPhrase_("OK") {
}
HttpResponse::~HttpResponse() {
}

void HttpResponse::reset() {
  statusCode_ = 200;
  statusPhrase_ = "OK";
  headers_.clear();
  body_.clear();
  setCookies_.clear();
}

void HttpResponse::setStatusCode(int code, const std::string& phrase) {
  statusCode_ = code;
  statusPhrase_ = phrase;
}

void HttpResponse::setStatusCode(int code) {
  statusCode_ = code;
  statusPhrase_ = reasonPhrase(code);
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
  headers_[key] = value;
}

void HttpResponse::setBody(const std::vector<char>& body) {
  body_ = body;
}

void HttpResponse::setBody(const std::string& body) {
  body_.assign(body.begin(), body.end());
}

void HttpResponse::setCookie(const std::string& key, const std::string& value, const std::string& path, int max_age, bool http_only) {
  std::ostringstream cookie;
  cookie << key << "=" << value << "; Path=" << path;
  if (max_age >= 0)
    cookie << "; Max-Age=" << max_age;
  if (http_only)
    cookie << "; HttpOnly";
  setCookies_.push_back(cookie.str());
}

std::vector<char> HttpResponse::serialize() const {
  std::ostringstream oss;
  oss << "HTTP/1.1 " << statusCode_ << " " << statusPhrase_ << "\r\n";
  for (std::map<std::string, std::string>::const_iterator it = headers_.begin(); it != headers_.end(); ++it) {
    // Content-Length is always computed from the body below.
    if (equalsIgnoreCase(it->first, "Content-Length"))
      continue;
    oss << it->first << ": " << it->second << "\r\n";
  }
  for (std::vector<std::string>::const_iterator it = setCookies_.begin(); it != setCookies_.end(); ++it)
    oss << "Set-Cookie: " << *it << "\r\n";
  oss << "Content-Length: " << body_.size() << "\r\n\r\n";
  std::string header = oss.str();
  std::vector<char> out(header.begin(), header.end());
  out.insert(out.end(), body_.begin(), body_.end());
  return out;
}

const std::string& HttpResponse::reasonPhrase(int code) {
  static std::map<int, std::string> phrases;
  if (phrases.empty()) {
    phrases[200] = "OK";
    phrases[201] = "Created";
    phrases[204] = "No Content";
    phrases[301] = "Moved Permanently";
    phrases[302] = "Found";
    phrases[400] = "Bad Request";
    phrases[403] = "Forbidden";
    phrases[404] = "Not Found";
    phrases[405] = "Method Not Allowed";
    phrases[413] = "Payload Too Large";
    phrases[414] = "URI Too Long";
    phrases[431] = "Request Header Fields Too Large";
    phrases[500] = "Internal Server Error";
    phrases[501] = "Not Implemented";
    phrases[502] = "Bad Gateway";
    phrases[504] = "Gateway Timeout";
    phrases[505] = "HTTP Version Not Supported";
  }
  std::map<int, std::string>::const_iterator it = phrases.find(code);
  if (it != phrases.end())
    return it->second;
  static const std::string unknown = "Unknown";
  return unknown;
}
