#include "HttpRequest.hpp"

#include "Utils.hpp"

HttpRequest::HttpRequest() : isChunked_(false), contentLength_(0), keepAlive_(false) {
}

HttpRequest::~HttpRequest() {
}

void HttpRequest::reset() {
  method_.clear();
  uri_.clear();
  path_.clear();
  queryString_.clear();
  version_.clear();
  headers_.clear();
  body_.clear();
  cookies_.clear();
  isChunked_ = false;
  contentLength_ = 0;
  keepAlive_ = false;
}

const std::string& HttpRequest::getMethod() const {
  return method_;
}
const std::string& HttpRequest::getUri() const {
  return uri_;
}
const std::string& HttpRequest::getPath() const {
  return path_;
}
const std::string& HttpRequest::getQueryString() const {
  return queryString_;
}
const std::string& HttpRequest::getVersion() const {
  return version_;
}
const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
  return headers_;
}
const std::vector<char>& HttpRequest::getBody() const {
  return body_;
}
bool HttpRequest::isChunked() const {
  return isChunked_;
}
size_t HttpRequest::contentLength() const {
  return contentLength_;
}
bool HttpRequest::isKeepAlive() const {
  return keepAlive_;
}
const std::map<std::string, std::string>& HttpRequest::getCookies() const {
  return cookies_;
}

void HttpRequest::setMethod(const std::string& method) {
  method_ = method;
}
void HttpRequest::setUri(const std::string& uri) {
  uri_ = uri;
}
void HttpRequest::setPath(const std::string& path) {
  path_ = path;
}
void HttpRequest::setQueryString(const std::string& query) {
  queryString_ = query;
}
void HttpRequest::setVersion(const std::string& version) {
  version_ = version;
}
void HttpRequest::addHeader(const std::string& key, const std::string& value) {
  headers_[key] = value;
}
void HttpRequest::appendBody(const std::vector<char>& chunk) {
  body_.insert(body_.end(), chunk.begin(), chunk.end());
}
void HttpRequest::appendBody(const char* data, size_t size) {
  body_.insert(body_.end(), data, data + size);
}
void HttpRequest::setChunked(bool chunked) {
  isChunked_ = chunked;
}
void HttpRequest::setContentLength(size_t length) {
  contentLength_ = length;
}
void HttpRequest::setKeepAlive(bool keep_alive) {
  keepAlive_ = keep_alive;
}

void HttpRequest::parseCookies() {
  cookies_.clear();
  // Header keys are normalized to lowercase by the parser (decision #14).
  std::map<std::string, std::string>::const_iterator it = headers_.find("cookie");
  if (it == headers_.end())
    return;
  const std::string& raw = it->second;
  std::string::size_type start = 0;
  while (start < raw.size()) {
    std::string::size_type end = raw.find(';', start);
    if (end == std::string::npos)
      end = raw.size();
    std::string pair = Utils::trim(raw.substr(start, end - start));
    if (!pair.empty()) {
      // Split at the first '='; values may legally contain '=' themselves.
      std::string::size_type eq = pair.find('=');
      if (eq != std::string::npos) {
        std::string key = Utils::trim(pair.substr(0, eq));
        if (!key.empty())
          cookies_[key] = Utils::trim(pair.substr(eq + 1));
      }
    }
    start = end + 1;
  }
}
