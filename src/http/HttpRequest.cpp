// STUB — Responsabilidad de Alex. Implementación mínima para enlazar.
#include "HttpRequest.hpp"

HttpRequest::HttpRequest()
    : isChunked_(false), contentLength_(0), keepAlive_(false) {}

HttpRequest::~HttpRequest() {}

void HttpRequest::reset() {
  method_.clear(); uri_.clear(); version_.clear();
  headers_.clear(); body_.clear(); cookies_.clear();
  isChunked_ = false; contentLength_ = 0; keepAlive_ = false;
}

const std::string& HttpRequest::getMethod() const { return method_; }
const std::string& HttpRequest::getUri() const { return uri_; }
const std::string& HttpRequest::getVersion() const { return version_; }
const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return headers_; }
const std::vector<char>& HttpRequest::getBody() const { return body_; }
bool HttpRequest::isChunked() const { return isChunked_; }
size_t HttpRequest::contentLength() const { return contentLength_; }
bool HttpRequest::isKeepAlive() const { return keepAlive_; }
const std::map<std::string, std::string>& HttpRequest::getCookies() const { return cookies_; }

void HttpRequest::setMethod(const std::string& m) { method_ = m; }
void HttpRequest::setUri(const std::string& u) { uri_ = u; }
void HttpRequest::setVersion(const std::string& v) { version_ = v; }
void HttpRequest::addHeader(const std::string& k, const std::string& v) { headers_[k] = v; }
void HttpRequest::appendBody(const std::vector<char>& c) {
  body_.insert(body_.end(), c.begin(), c.end());
}
void HttpRequest::appendBody(const char* d, size_t s) {
  body_.insert(body_.end(), d, d + s);
}
void HttpRequest::setChunked(bool c) { isChunked_ = c; }
void HttpRequest::setContentLength(size_t l) { contentLength_ = l; }
void HttpRequest::setKeepAlive(bool k) { keepAlive_ = k; }
void HttpRequest::parseCookies() {}
