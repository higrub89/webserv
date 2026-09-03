#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <sstream>
#include <string>
#include <vector>

/**
 * @class Utils
 * @brief Static utility functions for URI manipulation, string formatting, header parsing, and CGI conversions.
 */
namespace Utils {
/**
 * @brief Extracts the file extension (e.g., ".py") from a URI, ignoring query parameters.
 * @param uri The URI or path string to inspect.
 * @return Extracted extension including the dot, or empty string if none found.
 */
std::string getExtension(const std::string& uri);

/**
 * @brief Finds the end of HTTP headers (delimited by \r\n\r\n or \n\n) in a character buffer.
 * @param buffer The buffer to search.
 * @param delimiter_len Output parameter receiving the length of the delimiter found (2 or 4).
 * @return The starting index of the delimiter in buffer, or std::string::npos if not found.
 */
size_t findHeadersEnd(const std::vector<char>& buffer, size_t& delimiter_len);

/**
 * @brief Case-insensitively checks if a line starts with "status:".
 * @param line The header line to check.
 * @return true if line begins with "status:", false otherwise.
 */
bool startsWithStatus(const std::string& line);

/**
 * @brief Converts an HTTP header key (e.g., "Accept-Language") to the CGI environment variable format (e.g., "ACCEPT_LANGUAGE").
 * @param key The HTTP header field name.
 * @return Formatted environment variable name in uppercase with hyphens converted to underscores.
 */
std::string toHeaderEnvKey(const std::string& key);

/**
 * @brief Trims leading and trailing whitespace from a string.
 * @param str The string to trim.
 * @param whitespace Characters considered whitespace.
 * @return Trimmed substring.
 */
std::string trim(const std::string& str, const std::string& whitespace = " \t\r\n\f\v");

/**
 * @brief Converts a string to all lowercase characters.
 * @param str Input string.
 * @return Lowercase string.
 */
std::string toLowerCase(const std::string& str);

/**
 * @brief Case-insensitively compares two strings.
 * @param a First string.
 * @param b Second string.
 * @return true if equal ignoring case, false otherwise.
 */
bool equalsIgnoreCase(const std::string& a, const std::string& b);

/**
 * @brief Checks if a string starts with a given prefix without allocating memory.
 * @param str The string to inspect.
 * @param prefix The prefix to check for.
 * @return true if str starts with prefix, false otherwise.
 */
bool startsWith(const std::string& str, const std::string& prefix);

/**
 * @brief Checks if a string ends with a given suffix without allocating memory.
 * @param str The string to inspect.
 * @param suffix The suffix to check for.
 * @return true if str ends with suffix, false otherwise.
 */
bool endsWith(const std::string& str, const std::string& suffix);

/**
 * @brief Checks if a string consists exclusively of decimal digit characters ('0'-'9').
 * @param str Input string.
 * @return true if non-empty and all characters are digits, false otherwise.
 */
bool isDigits(const std::string& str);

/**
 * @brief Normalizes a URI path (removes '.' and '..' dot-segments).
 * Prevents path traversal out of root and guarantees a valid leading '/' path.
 * @param path The raw URI path.
 * @return Normalized canonical path string.
 */
std::string normalizeUriPath(const std::string& path);

/**
 * @brief Converts any streamable value to std::string (C++98 alternative to std::to_string).
 * @param val The value to convert.
 * @return The string representation of val.
 */
template <typename T>
std::string toString(const T& val) {
  std::ostringstream oss;
  oss << val;
  return oss.str();
}

/**
 * @brief Joins a vector of elements into a single string with a specified delimiter.
 * @param elements Vector of elements to join.
 * @param delimiter Separator string between elements.
 * @return Joined string.
 */
template <typename T>
std::string join(const std::vector<T>& elements, const std::string& delimiter) {
  std::ostringstream oss;
  for (size_t i = 0; i < elements.size(); ++i) {
    if (i != 0) {
      oss << delimiter;
    }
    oss << elements[i];
  }
  return oss.str();
}
}  // namespace Utils

#endif  // UTILS_HPP_
