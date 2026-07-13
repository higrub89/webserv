#include "Logger.hpp"

#include <ctime>
#include <iostream>
#include <sstream>

std::string Logger::timestamp() {
  char buf[64];
  std::time_t now = std::time(NULL);
  struct tm tm_info;

  localtime_r(&now, &tm_info);
  std::strftime(buf, sizeof(buf), "[%d/%b/%Y:%H:%M:%S]", &tm_info);
  return std::string(buf);
}

void Logger::info(const std::string& msg) {
  std::cout << timestamp() << " " << ANSI_INFO << "[INFO]" << ANSI_RESET << "  "
            << msg << std::endl;
}

void Logger::error(const std::string& msg) {
  std::cerr << timestamp() << " " << ANSI_ERROR << "[ERROR]" << ANSI_RESET
            << " " << msg << std::endl;
}

void Logger::debug(const std::string& msg) {
  if (DEBUG)
    std::cout << timestamp() << " " << ANSI_DEBUG << "[DEBUG]" << ANSI_RESET
              << " " << msg << std::endl;
}

std::string Logger::addrToString(const struct sockaddr_in& addr) {
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));

  std::ostringstream oss;
  oss << ip << ":" << ntohs(addr.sin_port);
  return oss.str();
}

in_addr_t Logger::stringToAddr(const std::string& ip) {
  struct in_addr addr;
  if (inet_pton(AF_INET, ip.c_str(), &addr) != 1)
    return INADDR_ANY;
  return addr.s_addr;
}
