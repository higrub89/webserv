// STUB — Responsabilidad de Alex. Implementación mínima para enlazar.
#include "HttpResponse.hpp"

#include <sstream>

HttpResponse::HttpResponse() : statusCode_(200), statusPhrase_("OK") {}
HttpResponse::~HttpResponse() {}

void HttpResponse::reset() {
  statusCode_ = 200; statusPhrase_ = "OK";
  headers_.clear(); body_.clear();
}

void HttpResponse::setStatusCode(int code, const std::string& phrase) {
  statusCode_ = code; statusPhrase_ = phrase;
}

void HttpResponse::setHeader(const std::string& k, const std::string& v) {
  headers_[k] = v;
}

void HttpResponse::setBody(const std::vector<char>& b) { body_ = b; }

void HttpResponse::setBody(const std::string& b) {
  body_.assign(b.begin(), b.end());
}

void HttpResponse::setCookie(const std::string& key, const std::string& val,
                             const std::string& path, int max_age,
                             bool http_only) {
  (void)key; (void)val; (void)path; (void)max_age; (void)http_only;
}

std::vector<char> HttpResponse::serialize() const {
  std::ostringstream oss;
  oss << "HTTP/1.1 " << statusCode_ << " " << statusPhrase_ << "\r\n";
  for (std::map<std::string, std::string>::const_iterator it = headers_.begin();
       it != headers_.end(); ++it)
    oss << it->first << ": " << it->second << "\r\n";
  oss << "Content-Length: " << body_.size() << "\r\n\r\n";
  std::string hdr = oss.str();
  std::vector<char> out(hdr.begin(), hdr.end());
  out.insert(out.end(), body_.begin(), body_.end());
  return out;
}
