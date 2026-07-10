#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <arpa/inet.h>

#include <string>

#ifndef DEBUG
#  define DEBUG 0
#endif

#define ANSI_RESET "\033[0m"
#define ANSI_ERROR "\033[31m"
#define ANSI_INFO "\033[36m"
#define ANSI_DEBUG "\033[33m"

/**
 * @class Logger
 * @brief Provides logging utilities for the server.
 *
 * Replaces: SocketUtils.hpp
 */
class Logger {
public:
  static std::string timestamp();

  static void info(const std::string& msg);

  static void error(const std::string& msg);

  static void debug(const std::string& msg);

  static std::string addrToString(const struct sockaddr_in& addr);
  static in_addr_t stringToAddr(const std::string& ip);
};

#endif  // LOGGER_HPP_
