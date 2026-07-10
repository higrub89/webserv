#ifndef HTTPREQUEST_HPP_
#define HTTPREQUEST_HPP_

#include <map>
#include <string>
#include <vector>

/**
 * @class HttpRequest
 * @brief DTO representing a fully parsed HTTP client request.
 *
 * Replaces: HTTP request representation in the old codebase.
 */
class HttpRequest {
private:
  std::string method_;
  std::string uri_;
  std::string version_;
  std::map<std::string, std::string> headers_;
  std::vector<char> body_;
  bool isChunked_;
  size_t contentLength_;
  bool keepAlive_;

  // Parsed cookies map (Key -> Value).
  // Resolves: Cookie/Session bonus requirement.
  std::map<std::string, std::string> cookies_;

public:
  HttpRequest();
  ~HttpRequest();

  /**
   * @brief Clears internal fields logically so the object can be recycled under
   * high load.
   */
  void reset();

  // Getters
  const std::string& getMethod() const;
  const std::string& getUri() const;
  const std::string& getVersion() const;
  const std::map<std::string, std::string>& getHeaders() const;
  const std::vector<char>& getBody() const;
  bool isChunked() const;
  size_t contentLength() const;
  bool isKeepAlive() const;
  const std::map<std::string, std::string>& getCookies() const;

  // Setters and modifiers (primarily used by HttpParser)
  void setMethod(const std::string& method);
  void setUri(const std::string& uri);
  void setVersion(const std::string& version);
  void addHeader(const std::string& key, const std::string& value);
  void appendBody(const std::vector<char>& chunk);
  void appendBody(const char* data, size_t size);
  void setChunked(bool chunked);
  void setContentLength(size_t length);
  void setKeepAlive(bool keep_alive);

  /**
   * @brief Parses the raw "Cookie" headers inside headers_ map to populate
   * cookies_.
   */
  void parseCookies();
};

#endif  // HTTPREQUEST_HPP_
