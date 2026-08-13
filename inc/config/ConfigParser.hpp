#ifndef CONFIGPARSER_HPP_
#define CONFIGPARSER_HPP_

#include <iosfwd>
#include <string>
#include <vector>

#include "types/ConfigStructures.hpp"

/**
 * @class ConfigParser
 * @brief Parses an NGINX-inspired configuration file into a ConfigMap.
 *
 * Zero-tolerance policy (decisions #2b/#4/#5): any syntax error, unknown
 * directive, duplicated directive or invalid value throws std::runtime_error
 * with a line-numbered message, and the server must refuse to start.
 *
 * ConfigMap keys are normalized "ip:port" strings (decision #17):
 * "listen 8080;" becomes "0.0.0.0:8080".
 */
class ConfigParser {
public:
  ConfigParser();
  ~ConfigParser();

  /**
   * @brief Parses the given file and builds the ConfigMap.
   * @throw std::runtime_error on any syntax or validation error.
   */
  ConfigMap parse(const std::string& filepath);

private:
  struct Token {
    std::string text;
    int line;
  };

  std::vector<Token> tokens_;
  size_t pos_;

  // Tokenizer
  void tokenize(std::istream& in);

  // Token stream helpers
  bool atEnd() const;
  const Token& peek() const;
  Token next();
  void expect(const std::string& text);

  // Grammar
  void parseServer(ConfigMap& out);
  void parseServerDirective(ServerConfig& server, std::string& ip, int& port,
                            std::vector<bool>& seen);
  LocationConfig parseLocation(std::string& path);

  // Directive value parsing / validation
  void parseListen(const std::string& value, std::string& ip, int& port,
                   int line);
  size_t parseBodySize(const std::string& value, int line);
  int parseErrorCode(const std::string& value, int line);
  std::string normalizeIp(const std::string& ip, int line);
  void validateMethods(const std::vector<std::string>& methods, int line);

  // Reads all arguments up to ';' (at least min_args) for the directive
  // named 'name' found at line 'line'.
  std::vector<std::string> readArgs(const std::string& name, size_t min_args,
                                    int line);
};

#endif  // CONFIGPARSER_HPP_
