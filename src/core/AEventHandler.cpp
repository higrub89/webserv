#include "AEventHandler.hpp"

AEventHandler::AEventHandler(int fd) : fd_(fd) {
}

AEventHandler::~AEventHandler() {
}

int AEventHandler::getFd() const {
  return fd_;
}

bool AEventHandler::isTimedOut(time_t current_time) const {
  // Derived classes can override this method to implement specific timeout
  // logic.
  (void)current_time;
  return false;
}
