#include "ServerHandler.hpp"

#include "ClientHandler.hpp"
#include "EpollManager.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

// ─── Constructor / Destructor ───────────────────────────────────────────────

ServerHandler::ServerHandler(int port, const ServerConfig& config,
                             EpollManager& epoll_manager, Router& router)
    : AEventHandler(-1),
      config_(config),
      epollManager_(epoll_manager),
      router_(router),
      port_(port) {
  std::memset(&address_, 0, sizeof(address_));
}

ServerHandler::~ServerHandler() {
  if (fd_ >= 0)
    close(fd_);
}

// ─── Inicialización: socket → setsockopt → fcntl → bind → listen ───────────

void ServerHandler::setup() {
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ < 0)
    throw std::runtime_error(
        std::string("socket() failed: ") + strerror(errno));

  int opt = 1;
  if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    throw std::runtime_error(
        std::string("setsockopt(SO_REUSEADDR) failed: ") + strerror(errno));

  if (fcntl(fd_, F_SETFL, O_NONBLOCK) < 0)
    throw std::runtime_error(
        std::string("fcntl(O_NONBLOCK) failed: ") + strerror(errno));

   if (fcntl(fd_, F_SETFD, FD_CLOEXEC) < 0)
    throw std::runtime_error(
        std::string("fcntl(FD_CLOEXEC) failed: ") + strerror(errno));

  address_.sin_family = AF_INET;
  address_.sin_port = htons(port_);
  address_.sin_addr.s_addr = INADDR_ANY;

  if (bind(fd_, (struct sockaddr*)&address_, sizeof(address_)) < 0) {
    std::ostringstream oss;
    oss << "bind() failed on port " << port_ << ": " << strerror(errno);
    throw std::runtime_error(oss.str());
  }

  if (listen(fd_, SOMAXCONN) < 0)
    throw std::runtime_error(
        std::string("listen() failed: ") + strerror(errno));

  std::cout << "[INFO]  Listening on 0.0.0.0:" << port_
            << " (fd=" << fd_ << ")" << std::endl;
}

// ─── Aceptar conexiones (loop hasta EAGAIN) ─────────────────────────────────

void ServerHandler::onReadReady() {
  while (true) {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int clientFd = accept(fd_, (struct sockaddr*)&clientAddr, &addrLen);

    if (clientFd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      std::cerr << "[ERROR] accept(): " << strerror(errno) << std::endl;
      break;
    }

    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
      std::cerr << "[ERROR] fcntl client fd: " << strerror(errno) << std::endl;
      close(clientFd);
      continue;
    }
    if (fcntl(clientFd, F_SETFD, FD_CLOEXEC) < 0) {
      std::cerr << "[ERROR] fcntl client fd: " << strerror(errno) << std::endl;
      close(clientFd);
      continue;
    }

    ClientHandler* client =
        new ClientHandler(clientFd, epollManager_, router_, port_);
    epollManager_.addHandler(client, EPOLLIN | EPOLLRDHUP);
  }
}

void ServerHandler::onWriteReady() {}

void ServerHandler::onDisconnect() {
  std::cerr << "[ERROR] Listening socket error on fd=" << fd_ << std::endl;
}
