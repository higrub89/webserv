#ifndef HTTPREQUEST_HPP_
#define HTTPREQUEST_HPP_

#include <map>
#include <string>
#include <vector>

/**
 * @class HttpRequest
 * @brief Data Transfer Object (DTO) encapsulating a parsed HTTP/1.1 request.
 *
 * Stores the HTTP method, original URI, sanitized path, query parameters, protocol version,
 * header map, payload body bytes, parsed cookies, and connection persistency flags.
 */
class HttpRequest {
private:
  std::string method_;
  std::string uri_;          // Original full URI as received (e.g. for logging/redirects)
  std::string path_;         // Percent-decoded path without query string
  std::string queryString_;  // Raw query string after '?'
  std::string version_;      // HTTP version string (e.g. "HTTP/1.1")
  std::map<std::string, std::string> headers_;
  std::vector<char> body_;
  bool isChunked_;
  size_t contentLength_;
  bool keepAlive_;
  std::map<std::string, std::string> cookies_;

public:
  HttpRequest();
  ~HttpRequest();

  void reset();

  const std::string& getMethod() const;
  const std::string& getUri() const;
  const std::string& getPath() const;
  const std::string& getQueryString() const;
  const std::string& getVersion() const;
  const std::map<std::string, std::string>& getHeaders() const;
  const std::vector<char>& getBody() const;
  bool isChunked() const;
  size_t contentLength() const;
  bool isKeepAlive() const;
  const std::map<std::string, std::string>& getCookies() const;

  void setMethod(const std::string& method);
  void setUri(const std::string& uri);
  void setPath(const std::string& path);
  void setQueryString(const std::string& query);
  void setVersion(const std::string& version);
  void addHeader(const std::string& key, const std::string& value);
  void appendBody(const std::vector<char>& chunk);
  void appendBody(const char* data, size_t size);
  void setChunked(bool chunked);
  void setContentLength(size_t length);
  void setKeepAlive(bool keep_alive);

  void parseCookies();
};

#endif  // HTTPREQUEST_HPP_
