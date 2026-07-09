#include "CgiReadHandler.hpp"

CgiReadHandler::CgiReadHandler(int stdout_fd, EpollManager* epoll_manager,
                               ClientHandler* client, pid_t cgi_pid)
  : AEventHandler(stdout_fd),
    epollManager_(epoll_manager),
    client_(client),
    cgiPid_(cgi_pid) {
  // Register this handler with the EpollManager for read the EPOLLIN (data
  // available) and EPOLLRDHUP (peer closed connection) events.
  epollManager_->addHandler(this, EPOLLIN | EPOLLRDHUP);
}

void CgiReadHandler::onReadReady() {
  // TODO: Must read
}

void CgiReadHandler::onWriteReady() {
  // No-op
}

void CgiReadHandler::onDisconnect() {
  // Clean up: remove from epoll, close the pipe, and reap the child process.
  epollManager_->removeHandler(this);
  close(getFd());

  if (cgiPid_ > 0) {
    waitpid(cgiPid_, NULL, WNOHANG);
    client_->clearCgi();  // Notify the client that the CGI process is done.
  }
}
