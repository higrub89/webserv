#ifndef HTTPRESPONSE_HPP_
#define HTTPRESPONSE_HPP_

#include <map>
#include <string>
#include <vector>

/**
 * @class HttpResponse
 * @brief DTO representing a response to be serialized and sent to a client.
 *
 * Replaces: HTTP response builder in the old codebase.
 */
class HttpResponse {
private:
  int statusCode_;
  std::string statusPhrase_;
  std::map<std::string, std::string> headers_;
  std::vector<char> body_;

  // Response cookies kept out of headers_: a std::map overwrites duplicate
  // keys and HTTP allows multiple Set-Cookie lines (decision #16).
  std::vector<std::string> setCookies_;

public:
  HttpResponse();
  ~HttpResponse();

  /**
   * @brief Resets the fields logically so the response object can be recycled.
   */
  void reset();

  // Setters
  void setStatusCode(int code, const std::string& phrase);

  /**
   * @brief Sets the status code resolving the standard reason phrase
   * automatically (e.g. 404 -> "Not Found").
   */
  void setStatusCode(int code);
  void setHeader(const std::string& key, const std::string& value);
  void setBody(const std::vector<char>& body);
  void setBody(const std::string& body);

  /**
   * @brief Appends a "Set-Cookie" header to the response with custom options.
   * @param key Cookie name.
   * @param value Cookie value.
   * @param path URI path limit (defaults to "/").
   * @param max_age Expiry time in seconds (-1 to ignore).
   * @param http_only Restrict Javascript access (defaults to true).
   */
  void setCookie(const std::string& key, const std::string& value,
                 const std::string& path = "/", int max_age = -1,
                 bool http_only = true);

  /**
   * @brief Serializes the HTTP status line, headers, and body into a raw byte
   * buffer.
   * @return std::vector<char> The formatted byte buffer.
   */
  std::vector<char> serialize() const;

  /**
   * @brief Returns the standard reason phrase for an HTTP status code
   * ("codigos precisos"). Unknown codes map to "Unknown".
   */
  static const std::string& reasonPhrase(int code);
};

#endif  // HTTPRESPONSE_HPP_
