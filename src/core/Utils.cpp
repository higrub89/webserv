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
    prefix[i] = std::tolower(static_cast<unsigned char>(prefix[i]));
  }
  return prefix == "status:";
}

std::string toHeaderEnvKey(const std::string& key) {
  std::string formatted = key;
  for (size_t i = 0; i < formatted.length(); ++i) {
    formatted[i] = std::toupper(static_cast<unsigned char>(formatted[i]));
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

std::string normalizeUriPath(const std::string& path) {
  if (path.empty()) {
    return "/";
  }

  bool hasTrailingSlash = (path.size() > 1 && path[path.size() - 1] == '/');
  std::vector<std::string> segments;
  std::string::size_type start = 0;

  while (start < path.size()) {
    std::string::size_type end = path.find('/', start);
    if (end == std::string::npos) {
      end = path.size();
    }
    if (end > start) {
      std::string segment = path.substr(start, end - start);
      if (segment == ".") {
        // Skip current directory dot segment
      } else if (segment == "..") {
        if (!segments.empty()) {
          segments.pop_back();
        }
      } else {
        segments.push_back(segment);
      }
    }
    start = end + 1;
  }

  if (segments.empty()) {
    return "/";
  }

  std::string result;
  for (size_t i = 0; i < segments.size(); ++i) {
    result += "/" + segments[i];
  }
  if (hasTrailingSlash) {
    result += "/";
  }
  return result;
}

std::string resolvePath(const std::string& uri, const std::string& route_path, const std::string& root_dir) {
  std::string root = root_dir;
  if (root.size() > 1 && root[root.size() - 1] == '/') {
    root.erase(root.size() - 1);
  }

  std::string relPath = uri;
  if (!route_path.empty()) {
    if (relPath.compare(0, route_path.size(), route_path) == 0) {
      relPath = relPath.substr(route_path.size());
    } else if (route_path[route_path.size() - 1] == '/' && (relPath + "/").compare(0, route_path.size(), route_path) == 0) {
      relPath = "";
    }
  }

  if (!relPath.empty() && relPath[0] != '/') {
    relPath = "/" + relPath;
  }
  return root + relPath;
}

std::string htmlEscape(const std::string& str) {
  std::string out;
  out.reserve(str.size());
  for (size_t i = 0; i < str.size(); ++i) {
    char c = str[i];
    if (c == '&') {
      out += "&amp;";
    } else if (c == '<') {
      out += "&lt;";
    } else if (c == '>') {
      out += "&gt;";
    } else if (c == '"') {
      out += "&quot;";
    } else if (c == '\'') {
      out += "&#39;";
    } else {
      out += c;
    }
  }
  return out;
}

}  // namespace Utils
