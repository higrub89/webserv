#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <sstream>
#include <string>
#include <vector>

class Utils {
private:
  Utils();  // Non-instantiable

public:
  /**
   * @brief Extracts the file extension (e.g., ".py") from a URI, ignoring query
   * parameters.
   */
  static std::string getExtension(const std::string& uri);

  /**
   * @brief Finds the end of headers (delimited by \r\n\r\n or \n\n) in a
   * character buffer.
   * @param buffer The buffer to search.
   * @param delimiter_len Output parameter receiving the length of the delimiter
   * found (2 or 4).
   * @return The starting index of the body (after the delimiter), or
   * std::string::npos if not found.
   */
  static size_t findHeadersEnd(const std::vector<char>& buffer,
                               size_t& delimiter_len);

  /**
   * @brief Case-insensitively checks if a line starts with "status:".
   */
  static bool startsWithStatus(const std::string& line);

  /**
   * @brief Converts an HTTP header key (e.g., "Accept-Language") to the CGI
   * environment variable format (e.g., "ACCEPT_LANGUAGE").
   */
  static std::string toHeaderEnvKey(const std::string& key);

  /**
   * @brief Converts any streamable value to std::string.
   */
  template <typename T>
  static std::string toString(const T& val) {
    std::ostringstream oss;
    oss << val;
    return oss.str();
  }
};

#endif  // UTILS_HPP_
