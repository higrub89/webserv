#include "AEventHandler.hpp"

AEventHandler::AEventHandler(int fd) : fd_(fd) {
}

AEventHandler::~AEventHandler() {
}

int AEventHandler::getFd() const {
  return fd_;
}

AEventHandler::AEventHandler(const AEventHandler& other) : fd_(other.fd_) {
}

AEventHandler& AEventHandler::operator=(const AEventHandler& other) {
  if (this != &other) {
    fd_ = other.fd_;
  }
  return *this;
}
