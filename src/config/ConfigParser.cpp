#include "ConfigParser.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "Utils.hpp"

namespace {

// Throws a line-numbered configuration error (decisions #2b/#5: zero
// tolerance, clear message, server refuses to start).
void fail(int line, const std::string& msg) {
  std::ostringstream oss;
  oss << "config: line " << line << ": " << msg;
  throw std::runtime_error(oss.str());
}

bool parseOnOff(const std::string& value, const std::string& name, int line) {
  if (value == "on")
    return true;
  if (value == "off")
    return false;
  fail(line, "directive '" + name + "' expects 'on' or 'off', got '" + value +
               "'");
  return false;  // unreachable
}

void checkDuplicate(std::vector<bool>& seen, int idx, const std::string& name,
                    int line) {
  if (seen[idx])
    fail(line, "duplicated directive '" + name + "'");
  seen[idx] = true;
}

}  // namespace

ConfigParser::ConfigParser() : pos_(0) {
}
ConfigParser::~ConfigParser() {
}

ConfigMap ConfigParser::parse(const std::string& filepath) {
  std::ifstream in(filepath.c_str());
  if (!in)
    throw std::runtime_error("config: cannot open file: " + filepath);
  tokenize(in);
  ConfigMap out;
  while (!atEnd()) {
    if (peek().text == "server")
      parseServer(out);
    else
      fail(peek().line,
           "expected 'server' block, got '" + peek().text + "'");
  }
  if (out.empty())
    throw std::runtime_error("config: no 'server' block defined");
  return out;
}

void ConfigParser::tokenize(std::istream& in) {
  tokens_.clear();
  pos_ = 0;
  std::string line;
  int lineno = 0;
  while (std::getline(in, line)) {
    ++lineno;
    std::string current;
    for (std::string::size_type i = 0; i <= line.size(); ++i) {
      char c = (i < line.size()) ? line[i] : ' ';
      if (c == '#')
        break;
      if (c == ' ' || c == '\t' || c == '\r' || c == '{' || c == '}' ||
          c == ';') {
        if (!current.empty()) {
          Token t;
          t.text = current;
          t.line = lineno;
          tokens_.push_back(t);
          current.clear();
        }
        if (c == '{' || c == '}' || c == ';') {
          Token t;
          t.text = std::string(1, c);
          t.line = lineno;
          tokens_.push_back(t);
        }
      } else {
        current += c;
      }
    }
    if (!current.empty()) {
      Token t;
      t.text = current;
      t.line = lineno;
      tokens_.push_back(t);
    }
  }
}

bool ConfigParser::atEnd() const {
  return pos_ >= tokens_.size();
}

const ConfigParser::Token& ConfigParser::peek() const {
  return tokens_[pos_];
}

ConfigParser::Token ConfigParser::next() {
  return tokens_[pos_++];
}

void ConfigParser::expect(const std::string& text) {
  if (atEnd())
    throw std::runtime_error("config: unexpected end of file, expected '" +
                             text + "'");
  if (peek().text != text)
    fail(peek().line,
         "expected '" + text + "', got '" + peek().text + "'");
  ++pos_;
}

void ConfigParser::parseServer(ConfigMap& out) {
  Token keyword = next();  // "server"
  expect("{");
  ServerConfig server;
  server.client_max_body_size = 1048576;  // 1m, NGINX default
  std::string ip;
  int port = -1;
  std::vector<bool> seen(SERVER_SEEN_COUNT, false);

  while (!atEnd() && peek().text != "}") {
    if (peek().text == "location") {
      int locLine = peek().line;
      std::string path;
      LocationConfig location = parseLocation(path);
      location.path = path;
      if (server.locations.find(path) != server.locations.end())
        fail(locLine, "duplicated location '" + path + "'");
      server.locations[path] = location;
    } else {
      parseServerDirective(server, ip, port, seen);
    }
  }
  if (atEnd())
    fail(keyword.line, "unclosed 'server' block");
  expect("}");

  if (port == -1)
    fail(keyword.line, "server block missing mandatory 'listen' directive");
  if (server.root_dir.empty())
    fail(keyword.line, "server block missing mandatory 'root' directive");

  // Locations without their own root inherit the server root.
  for (std::map<std::string, LocationConfig>::iterator it =
         server.locations.begin();
       it != server.locations.end(); ++it)
    if (it->second.root_dir.empty())
      it->second.root_dir = server.root_dir;

  // Group by normalized "ip:port" key (decision #17). The first server
  // declared for a key is its default server (decision #6).
  std::ostringstream key;
  key << ip << ":" << port;
  ServerGroup& group = out[key.str()];
  if (group.servers.empty()) {
    group.ip = ip;
    group.port = port;
  }
  for (std::vector<ServerConfig>::const_iterator sv = group.servers.begin();
       sv != group.servers.end(); ++sv)
    for (std::vector<std::string>::const_iterator existing =
           sv->server_names.begin();
         existing != sv->server_names.end(); ++existing)
      for (std::vector<std::string>::const_iterator name =
             server.server_names.begin();
           name != server.server_names.end(); ++name)
        if (*existing == *name)
          fail(keyword.line, "duplicated server_name '" + *name +
                               "' for " + key.str());
  group.servers.push_back(server);
}

void ConfigParser::parseServerDirective(ServerConfig& server, std::string& ip,
                                        int& port, std::vector<bool>& seen) {
  Token name = next();
  if (name.text == "listen") {
    checkDuplicate(seen, SEEN_LISTEN, name.text, name.line);
    std::vector<std::string> args = readArgs(name.text, 1, name.line);
    if (args.size() != 1)
      fail(name.line, "'listen' takes exactly one argument");
    parseListen(args[0], ip, port, name.line);
  } else if (name.text == "server_name") {
    checkDuplicate(seen, SEEN_SERVER_NAME, name.text, name.line);
    server.server_names = readArgs(name.text, 1, name.line);
  } else if (name.text == "root") {
    checkDuplicate(seen, SEEN_ROOT, name.text, name.line);
    std::vector<std::string> args = readArgs(name.text, 1, name.line);
    if (args.size() != 1)
      fail(name.line, "'root' takes exactly one argument");
    server.root_dir = args[0];
  } else if (name.text == "client_max_body_size") {
    checkDuplicate(seen, SEEN_BODY_SIZE, name.text, name.line);
    std::vector<std::string> args = readArgs(name.text, 1, name.line);
    if (args.size() != 1)
      fail(name.line, "'client_max_body_size' takes exactly one argument");
    server.client_max_body_size = parseBodySize(args[0], name.line);
  } else if (name.text == "error_page") {
    // Repeatable directive: each line maps one or more codes to a page;
    // repeating a code is the duplication error (decision #4).
    std::vector<std::string> args = readArgs(name.text, 2, name.line);
    const std::string& page = args[args.size() - 1];
    for (std::string::size_type i = 0; i + 1 < args.size(); ++i) {
      int code = parseErrorCode(args[i], name.line);
      if (server.error_pages.find(code) != server.error_pages.end())
        fail(name.line, "duplicated error_page for code " + args[i]);
      server.error_pages[code] = page;
    }
  } else {
    fail(name.line,
         "unknown directive '" + name.text + "' in server block");
  }
}

LocationConfig ConfigParser::parseLocation(std::string& path) {
  Token keyword = next();  // "location"
  if (atEnd() || peek().text == "{" || peek().text == "}" ||
      peek().text == ";")
    fail(keyword.line, "'location' expects a path");
  path = next().text;
  if (!Utils::startsWith(path, "/"))
    fail(keyword.line, "location path must start with '/'");
  expect("{");

  LocationConfig location;
  location.autoindex = false;
  location.upload_enable = false;
  location.client_max_body_size = 0;  // 0 = hereda del server (contrato del Router)
  std::vector<bool> seen(LOCATION_SEEN_COUNT, false);

  while (!atEnd() && peek().text != "}") {
    Token name = next();
    if (name.text == "methods") {
      checkDuplicate(seen, LSEEN_METHODS, name.text, name.line);
      std::vector<std::string> args = readArgs(name.text, 1, name.line);
      validateMethods(args, name.line);
      location.allowed_methods = args;
    } else if (name.text == "root") {
      checkDuplicate(seen, LSEEN_ROOT, name.text, name.line);
      std::vector<std::string> args = readArgs(name.text, 1, name.line);
      if (args.size() != 1)
        fail(name.line, "'root' takes exactly one argument");
      location.root_dir = args[0];
    } else if (name.text == "client_max_body_size") {
      checkDuplicate(seen, LSEEN_BODY_SIZE, name.text, name.line);
      std::vector<std::string> args = readArgs(name.text, 1, name.line);
      if (args.size() != 1)
        fail(name.line, "'client_max_body_size' takes exactly one argument");
      location.client_max_body_size = parseBodySize(args[0], name.line);
    } else if (name.text == "autoindex") {
      checkDuplicate(seen, LSEEN_AUTOINDEX, name.text, name.line);
      std::vector<std::string> args = readArgs(name.text, 1, name.line);
      if (args.size() != 1)
        fail(name.line, "'autoindex' takes exactly one argument");
      location.autoindex = parseOnOff(args[0], name.text, name.line);
    } else if (name.text == "index") {
      checkDuplicate(seen, LSEEN_INDEX, name.text, name.line);
      std::vector<std::string> args = readArgs(name.text, 1, name.line);
      if (args.size() != 1)
        fail(name.line, "'index' takes exactly one argument");
      location.index_file = args[0];
    } else if (name.text == "cgi") {
      // Repeatable: one mapping per extension (multi-CGI bonus);
      // repeating an extension is a duplication error (decision #4).
      std::vector<std::string> args = readArgs(name.text, 2, name.line);
      if (args.size() != 2)
        fail(name.line, "'cgi' expects: cgi <.ext> <binary>");
      if (!Utils::startsWith(args[0], "."))
        fail(name.line, "cgi extension must start with '.', got '" +
                          args[0] + "'");
      if (location.cgi_handlers.find(args[0]) !=
          location.cgi_handlers.end())
        fail(name.line, "duplicated cgi mapping for '" + args[0] + "'");
      location.cgi_handlers[args[0]] = args[1];
    } else if (name.text == "redirect") {
      checkDuplicate(seen, LSEEN_REDIRECT, name.text, name.line);
      std::vector<std::string> args = readArgs(name.text, 1, name.line);
      if (args.size() != 1)
        fail(name.line, "'redirect' takes exactly one argument");
      location.return_redirect = args[0];
    } else if (name.text == "upload_enable") {
      checkDuplicate(seen, LSEEN_UPLOAD_ENABLE, name.text, name.line);
      std::vector<std::string> args = readArgs(name.text, 1, name.line);
      if (args.size() != 1)
        fail(name.line, "'upload_enable' takes exactly one argument");
      location.upload_enable = parseOnOff(args[0], name.text, name.line);
    } else if (name.text == "upload_store") {
      checkDuplicate(seen, LSEEN_UPLOAD_STORE, name.text, name.line);
      std::vector<std::string> args = readArgs(name.text, 1, name.line);
      if (args.size() != 1)
        fail(name.line, "'upload_store' takes exactly one argument");
      location.upload_store = args[0];
    } else {
      fail(name.line,
           "unknown directive '" + name.text + "' in location block");
    }
  }
  if (atEnd())
    fail(keyword.line, "unclosed 'location' block");
  expect("}");

  if (location.upload_enable && location.upload_store.empty())
    fail(keyword.line,
         "'upload_enable on' requires an 'upload_store' path");
  if (location.allowed_methods.empty())
    location.allowed_methods.push_back("GET");  // least-privilege default
  return location;
}

void ConfigParser::parseListen(const std::string& value, std::string& ip,
                               int& port, int line) {
  std::string host;
  std::string portStr;
  std::string::size_type colon = value.rfind(':');
  if (colon == std::string::npos) {
    host = "0.0.0.0";  // no interface given: bind all (decision #17)
    portStr = value;
  } else {
    host = normalizeIp(value.substr(0, colon), line);
    portStr = value.substr(colon + 1);
  }
  if (!Utils::isDigits(portStr) || portStr.size() > 5)
    fail(line, "invalid port in 'listen " + value + "'");
  long p = 0;
  for (std::string::size_type i = 0; i < portStr.size(); ++i)
    p = p * 10 + (portStr[i] - '0');
  if (p < 1 || p > 65535)
    fail(line, "port out of range (1-65535) in 'listen " + value + "'");
  ip = host;
  port = static_cast<int>(p);
}

std::string ConfigParser::normalizeIp(const std::string& ip, int line) {
  if (ip == "*" || ip == "0.0.0.0")
    return "0.0.0.0";
  if (ip == "localhost")
    return "127.0.0.1";
  // Validate dotted-quad form.
  int octets = 0;
  std::string::size_type start = 0;
  while (start <= ip.size()) {
    std::string::size_type dot = ip.find('.', start);
    if (dot == std::string::npos)
      dot = ip.size();
    std::string part = ip.substr(start, dot - start);
    if (!Utils::isDigits(part) || part.size() > 3)
      fail(line, "invalid IP address '" + ip + "'");
    int value = 0;
    for (std::string::size_type i = 0; i < part.size(); ++i)
      value = value * 10 + (part[i] - '0');
    if (value > 255)
      fail(line, "invalid IP address '" + ip + "'");
    ++octets;
    if (dot == ip.size())
      break;
    start = dot + 1;
  }
  if (octets != 4)
    fail(line, "invalid IP address '" + ip + "'");
  return ip;
}

size_t ConfigParser::parseBodySize(const std::string& value, int line) {
  if (value.empty())
    fail(line, "'client_max_body_size' expects a size");
  size_t multiplier = 1;
  std::string digits = value;
  char last = value[value.size() - 1];
  if (last == 'k' || last == 'K') {
    multiplier = 1024;
    digits = value.substr(0, value.size() - 1);
  } else if (last == 'm' || last == 'M') {
    multiplier = 1048576;
    digits = value.substr(0, value.size() - 1);
  }
  if (!Utils::isDigits(digits))
    fail(line, "invalid size '" + value +
                 "' (expected digits with optional k/K/m/M suffix)");
  size_t result = 0;
  for (std::string::size_type i = 0; i < digits.size(); ++i) {
    if (result > 4294967295UL / 10)
      fail(line, "'client_max_body_size' value too large: " + value);
    result = result * 10 + (digits[i] - '0');
  }
  if (result > 4294967295UL / multiplier)
    fail(line, "'client_max_body_size' value too large: " + value);
  return result * multiplier;
}

int ConfigParser::parseErrorCode(const std::string& value, int line) {
  if (!Utils::isDigits(value) || value.size() != 3)
    fail(line, "invalid error_page code '" + value + "'");
  int code = 0;
  for (std::string::size_type i = 0; i < value.size(); ++i)
    code = code * 10 + (value[i] - '0');
  if (code < 300 || code > 599)
    fail(line, "error_page code out of range (300-599): " + value);
  return code;
}

void ConfigParser::validateMethods(const std::vector<std::string>& methods,
                                   int line) {
  for (std::vector<std::string>::const_iterator it = methods.begin();
       it != methods.end(); ++it) {
    if (*it != "GET" && *it != "POST" && *it != "DELETE")
      fail(line, "unsupported method '" + *it +
                   "' (allowed: GET, POST, DELETE)");
    for (std::vector<std::string>::const_iterator other = methods.begin();
         other != it; ++other)
      if (*other == *it)
        fail(line, "duplicated method '" + *it + "'");
  }
}

std::vector<std::string> ConfigParser::readArgs(const std::string& name,
                                                size_t min_args, int line) {
  std::vector<std::string> args;
  while (!atEnd() && peek().text != ";") {
    if (peek().text == "{" || peek().text == "}")
      fail(peek().line, "expected ';' after directive '" + name + "'");
    args.push_back(next().text);
  }
  if (atEnd())
    fail(line, "missing ';' after directive '" + name + "'");
  next();  // consume ';'
  if (args.size() < min_args) {
    std::ostringstream oss;
    oss << "directive '" << name << "' needs at least " << min_args
        << " argument(s)";
    fail(line, oss.str());
  }
  return args;
}
