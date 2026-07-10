#include "Logger.hpp"

#include <ctime>
#include <iostream>

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
