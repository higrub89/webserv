/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientConnection.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhiguita <rhiguita@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/28                               #+#    #+#             */
/*   Updated: 2026/06/28                              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
/*
#include "ClientConnection.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <sstream>

#include "SocketUtils.hpp"

// ─── Constructor / Destructor ───────────────────────────────────────────────

ClientConnection::ClientConnection(int fd, const ServerConfig& serverConfig)
  : fd_(fd),
    state_(STATE_READING),
    serverConfig_(serverConfig),
    writeOffset_(0),
    lastActivity_(std::time(NULL)),
    keepAlive_(true) {}

ClientConnection::~ClientConnection() {
  if (fd_ >= 0)
    close(fd_);
}

// ─── Lectura (acumulativa) ──────────────────────────────────────────────────

ssize_t ClientConnection::readData() {
  char chunk[READ_BUFFER_SIZE];
  ssize_t n = recv(fd_, chunk, sizeof(chunk), 0);

  if (n > 0) {
    readBuffer_.append(chunk, n);
    lastActivity_ = std::time(NULL);
  } else if (n == 0) {
    state_ = STATE_CLOSING;
  } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
    state_ = STATE_CLOSING;
  }
  return n;
}

// ─── Escritura (parcial con offset) ─────────────────────────────────────────

ssize_t ClientConnection::writeData() {
  if (writeOffset_ >= writeBuffer_.size())
    return 0;

  const char* ptr = writeBuffer_.c_str() + writeOffset_;
  size_t remaining = writeBuffer_.size() - writeOffset_;

  ssize_t n = send(fd_, ptr, remaining, MSG_NOSIGNAL);
  if (n > 0) {
    writeOffset_ += n;
    lastActivity_ = std::time(NULL);
  } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
    state_ = STATE_CLOSING;
  }
  return n;
}

// ─── Checks de estado ──────────────────────────────────────────────────────

bool ClientConnection::isRequestComplete() const {
  return readBuffer_.find("\r\n\r\n") != std::string::npos;
}

bool ClientConnection::isResponseComplete() const {
  return writeOffset_ >= writeBuffer_.size() && !writeBuffer_.empty();
}

bool ClientConnection::isTimedOut() const {
  return (std::time(NULL) - lastActivity_) > CONNECTION_TIMEOUT_SECS;
}

// ─── Integración con Alex ──────────────────────────────────────────────────

RawRequest ClientConnection::extractRequest() const {
  RawRequest req;
  req.fd = fd_;
  req.data = readBuffer_;
  req.complete = isRequestComplete();
  return req;
}

// ─── Integración con Ángel ─────────────────────────────────────────────────

void ClientConnection::queueResponse(const RawResponse& response) {
  writeBuffer_ = response.data;
  writeOffset_ = 0;
  state_ = STATE_WRITING;
}

// ─── Getters / Setters ─────────────────────────────────────────────────────

int ClientConnection::getFd() const { return fd_; }
ConnectionState ClientConnection::getState() const { return state_; }
void ClientConnection::setState(ConnectionState state) { state_ = state; }
const ServerConfig& ClientConnection::getServerConfig() const { return serverConfig_; }
bool ClientConnection::isKeepAlive() const { return keepAlive_; }
void ClientConnection::setKeepAlive(bool val) { keepAlive_ = val; }

// ─── Reset para keep-alive ─────────────────────────────────────────────────

void ClientConnection::reset() {
  readBuffer_.clear();
  writeBuffer_.clear();
  writeOffset_ = 0;
  state_ = STATE_READING;
  lastActivity_ = std::time(NULL);
}
*/
