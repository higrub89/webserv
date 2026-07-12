#include "Utils.hpp"

#include <cctype>

std::string Utils::getExtension(const std::string& uri) {
  size_t question_mark = uri.find('?');
  std::string path =
    (question_mark == std::string::npos) ? uri : uri.substr(0, question_mark);
  size_t dot = path.find_last_of('.');
  if (dot != std::string::npos && dot < path.length() - 1) {
    return path.substr(dot);
  }
  return "";
}

size_t Utils::findHeadersEnd(const std::vector<char>& buffer,
                             size_t& delimiter_len) {
  for (size_t i = 0; i < buffer.size(); ++i) {
    if (i + 3 < buffer.size() && buffer[i] == '\r' && buffer[i + 1] == '\n' &&
        buffer[i + 2] == '\r' && buffer[i + 3] == '\n') {
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

bool Utils::startsWithStatus(const std::string& line) {
  if (line.size() < 7)
    return false;
  std::string prefix = line.substr(0, 7);
  for (size_t i = 0; i < 7; ++i) {
    prefix[i] = std::tolower(prefix[i]);
  }
  return prefix == "status:";
}

std::string Utils::toHeaderEnvKey(const std::string& key) {
  std::string formatted = key;
  for (size_t i = 0; i < formatted.length(); ++i) {
    formatted[i] = std::toupper(formatted[i]);
    if (formatted[i] == '-') {
      formatted[i] = '_';
    }
  }
  return formatted;
}
