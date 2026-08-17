#include "CgiWriteHandler.hpp"

#include <unistd.h>

#include "ClientHandler.hpp"
#include "EpollManager.hpp"

CgiWriteHandler::CgiWriteHandler(int stdin_fd, EpollManager& epoll_manager, ClientHandler& client, const std::vector<char>& body) : AEventHandler(stdin_fd), epollManager_(epoll_manager), client_(client), bodyBuffer_(body), bytesWritten_(0) {
  epollManager_.addHandler(this, EPOLLOUT | EPOLLRDHUP);
}

CgiWriteHandler::~CgiWriteHandler() {
  epollManager_.removeHandler(this);
  if (fd_ != -1) {
    close(fd_);
    fd_ = -1;
  }
}

void CgiWriteHandler::onReadReady() {
  // No-op for write handler
}

void CgiWriteHandler::onWriteReady() {
  if (bytesWritten_ < bodyBuffer_.size()) {
    const char* data_ptr = &bodyBuffer_[bytesWritten_];
    size_t data_len = bodyBuffer_.size() - bytesWritten_;
    ssize_t written = write(getFd(), data_ptr, data_len);

    if (written > 0) {
      bytesWritten_ += written;
    } else {
      // written <= 0: write error (e.g. EPIPE, CGI stdin closed prematurely)
      onDisconnect();
      return;
    }
  }

  // If we have written the entire body buffer, close the write pipe to signal
  // EOF to the CGI.
  if (bytesWritten_ == bodyBuffer_.size()) {
    epollManager_.removeHandler(this);
    if (fd_ != -1) {
      close(fd_);
      fd_ = -1;
    }
  }
}

void CgiWriteHandler::onDisconnect() {
  epollManager_.removeHandler(this);
  if (fd_ != -1) {
    close(fd_);
    fd_ = -1;
  }
  client_.handleCgiError();
}
