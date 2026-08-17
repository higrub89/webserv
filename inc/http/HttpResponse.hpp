#ifndef HTTPRESPONSE_HPP_
#define HTTPRESPONSE_HPP_

#include <map>
#include <string>
#include <vector>

/**
 * @class HttpResponse
 * @brief Data Transfer Object (DTO) and serializer for HTTP/1.1 responses.
 *
 * Encapsulates the HTTP status code, reason phrase, response headers, Set-Cookie directives,
 * and payload body bytes, providing serialization to wire format for transmission.
 */
class HttpResponse {
private:
  int statusCode_;
  std::string statusPhrase_;
  std::map<std::string, std::string> headers_;
  std::vector<char> body_;

  // Multiple Set-Cookie headers are kept in a vector since HTTP permits multiple Set-Cookie fields
  std::vector<std::string> setCookies_;

public:
  HttpResponse();
  ~HttpResponse();

  void reset();

  void setStatusCode(int code, const std::string& phrase);
  void setStatusCode(int code);
  void setHeader(const std::string& key, const std::string& value);
  void setBody(const std::vector<char>& body);
  void setBody(const std::string& body);

  void setCookie(const std::string& key, const std::string& value, const std::string& path = "/", int max_age = -1, bool http_only = true);
  std::vector<char> serialize() const;
  static const std::string& reasonPhrase(int code);
};

#endif  // HTTPRESPONSE_HPP_
