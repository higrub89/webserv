#include "Utils.hpp"

#include <cctype>

namespace Utils {

std::string getExtension(const std::string& uri) {
  size_t question_mark = uri.find('?');
  std::string path = (question_mark == std::string::npos) ? uri : uri.substr(0, question_mark);
  size_t dot = path.find_last_of('.');
  if (dot != std::string::npos && dot < path.length() - 1) {
    return path.substr(dot);
  }
  return "";
}

size_t findHeadersEnd(const std::vector<char>& buffer, size_t& delimiter_len) {
  for (size_t i = 0; i < buffer.size(); ++i) {
    if (i + 3 < buffer.size() && buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n') {
      delimiter_len = 4;
      return i;
    }
    if (i + 1 < buffer.size() && buffer[i] == '\n' && buffer[i + 1] == '\n') {
      delimiter_len = 2;
      return i;
    }
  }
  return std::string::npos;
}

bool startsWithStatus(const std::string& line) {
  if (line.size() < 7)
    return false;
  std::string prefix = line.substr(0, 7);
  for (size_t i = 0; i < 7; ++i) {
    prefix[i] = std::tolower(prefix[i]);
  }
  return prefix == "status:";
}

std::string toHeaderEnvKey(const std::string& key) {
  std::string formatted = key;
  for (size_t i = 0; i < formatted.length(); ++i) {
    formatted[i] = std::toupper(formatted[i]);
    if (formatted[i] == '-') {
      formatted[i] = '_';
    }
  }
  return formatted;
}

std::string trim(const std::string& str, const std::string& whitespace) {
  size_t start = str.find_first_not_of(whitespace);
  if (start == std::string::npos) {
    return "";
  }
  size_t end = str.find_last_not_of(whitespace);
  return str.substr(start, end - start + 1);
}

std::string toLowerCase(const std::string& str) {
  if (str.empty()) {
    return str;
  }
  std::string result = str;
  for (size_t i = 0; i < result.length(); ++i) {
    result[i] = std::tolower(static_cast<unsigned char>(result[i]));
  }
  return result;
}

bool equalsIgnoreCase(const std::string& a, const std::string& b) {
  if (a.size() != b.size())
    return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
      return false;
  }
  return true;
}

bool startsWith(const std::string& str, const std::string& prefix) {
  if (str.size() < prefix.size())
    return false;
  return str.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string& str, const std::string& suffix) {
  if (str.size() < suffix.size())
    return false;
  return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool isDigits(const std::string& str) {
  if (str.empty())
    return false;
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] < '0' || str[i] > '9')
      return false;
  }
  return true;
}

}  // namespace Utils
