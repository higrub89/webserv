/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhiguita <rhiguita@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/28                               #+#    #+#             */
/*   Updated: 2026/06/28                              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
/*
#include "ServerSocket.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include "SocketUtils.hpp"

/// ─── Constructor / Destructor ───────────────────────────────────────────────

ServerSocket::ServerSocket(const ServerConfig& config) : config_(config), fd_(-1) {
  std::memset(&addr_, 0, sizeof(addr_));
}

ServerSocket::~ServerSocket() {
  if (fd_ >= 0) {
    close(fd_);
    std::ostringstream oss;
    oss << "ServerSocket closed on fd " << fd_;
    SocketUtils::logInfo(oss.str());
  }
}

// ─── Inicialización pública ─────────────────────────────────────────────────

void ServerSocket::init() {
  createSocket();
  setSocketOptions();
  bindSocket();
  listenSocket();

  std::ostringstream oss;
  oss << "Listening on " << config_.host << ":" << config_.port << " (fd=" << fd_ << ")";
  SocketUtils::logInfo(oss.str());
}

// ─── Aceptar conexión ──────────────────────────────────────────────────────

int ServerSocket::acceptClient(struct sockaddr_in& clientAddr) const {
  socklen_t addrLen = sizeof(clientAddr);
  int clientFd = accept(fd_, (struct sockaddr*)&clientAddr, &addrLen);

  if (clientFd < 0) {
    // EAGAIN/EWOULDBLOCK es normal en non-blocking: no hay clientes pendientes
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return -1;
    SocketUtils::logError(std::string("accept() failed: ") + strerror(errno));
    return -1;
  }

  // Marcar el FD del nuevo cliente como non-blocking
  if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
    SocketUtils::logError("fcntl(O_NONBLOCK) failed on client fd");
    close(clientFd);
    return -1;
  }

  std::ostringstream oss;
  oss << "Accepted client fd=" << clientFd;
  SocketUtils::logDebug(oss.str());
  return clientFd;
}

// ─── Getters ────────────────────────────────────────────────────────────────

int ServerSocket::getFd() const { return fd_; }

int ServerSocket::getPort() const { return config_.port; }

const ServerConfig& ServerSocket::getConfig() const { return config_; }

// ─── Helpers privados ───────────────────────────────────────────────────────

void ServerSocket::createSocket() {
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ < 0)
    throw std::runtime_error(std::string("socket() failed: ") + strerror(errno));
}

void ServerSocket::setSocketOptions() {
  int opt = 1;

  // SO_REUSEADDR: permite reutilizar el puerto inmediatamente tras reiniciar
  // sin esperar al TIME_WAIT del kernel TCP
  if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    throw std::runtime_error(std::string("setsockopt(SO_REUSEADDR) failed: ") + strerror(errno));

  // O_NONBLOCK: crítico para que poll() funcione correctamente.
  // Sin esto, accept()/recv()/send() bloquearían el proceso entero.
  if (fcntl(fd_, F_SETFL, O_NONBLOCK) < 0)
    throw std::runtime_error(std::string("fcntl(O_NONBLOCK) failed: ") + strerror(errno));
}

void ServerSocket::bindSocket() {
  addr_.sin_family = AF_INET;
  addr_.sin_port = htons(config_.port);
  addr_.sin_addr.s_addr = SocketUtils::stringToAddr(config_.host);

  if (bind(fd_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0) {
    std::ostringstream oss;
    oss << "bind() failed on " << config_.host << ":" << config_.port << ": " << strerror(errno);
    throw std::runtime_error(oss.str());
  }
}

void ServerSocket::listenSocket() {
  // SOMAXCONN: máximo de conexiones pendientes en el backlog del kernel.
  // En Linux, suele ser 128 o 4096 según /proc/sys/net/core/somaxconn.
  if (listen(fd_, SOMAXCONN) < 0)
    throw std::runtime_error(std::string("listen() failed: ") + strerror(errno));
}
*/
