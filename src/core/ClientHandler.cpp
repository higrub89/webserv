#include "ClientHandler.hpp"

#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "CgiReadHandler.hpp"
#include "CgiWriteHandler.hpp"
#include "EpollManager.hpp"
#include "Router.hpp"

#define CLIENT_TIMEOUT_SECS 60
#define READ_BUF_SIZE 65536

// ─── Constructor / Destructor ───────────────────────────────────────────────

ClientHandler::ClientHandler(int fd, EpollManager& epoll_manager, Router& router, int server_port, const std::string& client_ip)
  : AEventHandler(fd),
    epollManager_(epoll_manager),
    router_(router),
    state_(READING_REQUEST),
    lastActivityTime_(std::time(NULL)),
    rawOutBufferOffset_(0),
    parser_(router_.getMaxBodySizeForPort(server_port)),
    serverPort_(server_port),
    clientIp_(client_ip),
    cgiReadHandler_(NULL),
    cgiWriteHandler_(NULL),
    cgiPid_(-1) {
}

ClientHandler::~ClientHandler() {
  clearCgi();
  if (fd_ >= 0)
    close(fd_);
}

// ─── onReadReady: UNA sola llamada a recv() ─────────────────────────────────

void ClientHandler::onReadReady() {
  if (state_ != READING_REQUEST)
    return;

  char buf[READ_BUF_SIZE];
  ssize_t n = recv(fd_, buf, sizeof(buf), 0);

  if (n > 0) {
    rawInBuffer_.insert(rawInBuffer_.end(), buf, buf + n);
    lastActivityTime_ = std::time(NULL);

    bool complete = parser_.consume(rawInBuffer_, request_);
    if (complete) {
      processRequest();
    } else if (parser_.getState() == HttpParser::STATE_ERROR) {
      const std::map<std::string, std::string>& headers = request_.getHeaders();
      std::map<std::string, std::string>::const_iterator it = headers.find("host");
      if (it != headers.end()) {
        router_.setErrorResponse(response_, parser_.getErrorCode(), it->second, serverPort_);
      } else {
        router_.setErrorResponse(response_, parser_.getErrorCode(), "", serverPort_);
      }
      appendToOutput(response_.serialize());
      changeState(WRITING_RESPONSE);
    }
  } else if (n == 0) {
    onDisconnect();
  } else {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      onDisconnect();
  }
}

// ─── onWriteReady: UNA sola llamada a send() ────────────────────────────────

void ClientHandler::onWriteReady() {
  if (rawOutBufferOffset_ >= rawOutBuffer_.size())
    return;

  ssize_t n = send(fd_, &rawOutBuffer_[rawOutBufferOffset_], rawOutBuffer_.size() - rawOutBufferOffset_, MSG_NOSIGNAL);

  if (n > 0) {
    rawOutBufferOffset_ += n;
    lastActivityTime_ = std::time(NULL);

    if (rawOutBufferOffset_ >= rawOutBuffer_.size()) {
      rawOutBuffer_.clear();
      rawOutBufferOffset_ = 0;
      if (request_.isKeepAlive())
        resetForKeepAlive();
      else
        onDisconnect();
    }
  } else if (n < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      onDisconnect();
  }
}

// ─── onDisconnect: última instrucción (delete this) ─────────────────────────

void ClientHandler::onDisconnect() {
  epollManager_.removeHandler(this);
  delete this;
}

// ─── processRequest: integración con Router ─────────────────────────────────

void ClientHandler::processRequest() {
  state_ = PROCESSING;

  router_.dispatch(request_, response_, this, serverPort_);

  if (state_ == WRITING_RESPONSE) {
    std::vector<char> raw = response_.serialize();
    appendToOutput(raw);
  }
}

// ─── resetForKeepAlive ──────────────────────────────────────────────────────

void ClientHandler::resetForKeepAlive() {
  rawOutBuffer_.clear();
  rawOutBufferOffset_ = 0;
  parser_.reset();
  request_.reset();
  response_.reset();
  lastActivityTime_ = std::time(NULL);
  changeState(READING_REQUEST);

  if (!rawInBuffer_.empty()) {
    bool complete = parser_.consume(rawInBuffer_, request_);
    if (complete) {
      processRequest();
    } else if (parser_.getState() == HttpParser::STATE_ERROR) {
      const std::map<std::string, std::string>& headers = request_.getHeaders();
      std::map<std::string, std::string>::const_iterator it = headers.find("host");
      if (it != headers.end()) {
        router_.setErrorResponse(response_, parser_.getErrorCode(), it->second, serverPort_);
      } else {
        router_.setErrorResponse(response_, parser_.getErrorCode(), "", serverPort_);
      }
      appendToOutput(response_.serialize());
      changeState(WRITING_RESPONSE);
    }
  }
}

// ─── Utilidades ─────────────────────────────────────────────────────────────

void ClientHandler::appendToOutput(const std::vector<char>& data) {
  if (rawOutBufferOffset_ > 0 && rawOutBufferOffset_ == rawOutBuffer_.size()) {
    rawOutBuffer_.clear();
    rawOutBufferOffset_ = 0;
  }
  rawOutBuffer_.insert(rawOutBuffer_.end(), data.begin(), data.end());
}

void ClientHandler::appendToOutput(const char* data, size_t len) {
  if (rawOutBufferOffset_ > 0 && rawOutBufferOffset_ == rawOutBuffer_.size()) {
    rawOutBuffer_.clear();
    rawOutBufferOffset_ = 0;
  }
  rawOutBuffer_.insert(rawOutBuffer_.end(), data, data + len);
}

void ClientHandler::changeState(ClientState new_state) {
  state_ = new_state;
  if (state_ == READING_REQUEST) {
    epollManager_.updateHandlerEvents(this, EPOLLIN | EPOLLRDHUP);
  } else if (state_ == WAITING_FOR_CGI || state_ == PROCESSING) {
    epollManager_.updateHandlerEvents(this, EPOLLRDHUP);
  } else if (state_ == WRITING_RESPONSE) {
    epollManager_.updateHandlerEvents(this, EPOLLOUT | EPOLLRDHUP);
  }
}

bool ClientHandler::isTimedOut(time_t current_time) const {
  return (current_time - lastActivityTime_) > CLIENT_TIMEOUT_SECS;
}

// ─── Gestión de CGI ─────────────────────────────────────────────────────────
void ClientHandler::registerCgi(pid_t pid, CgiReadHandler* read_h, CgiWriteHandler* write_h) {
  cgiPid_ = pid;
  cgiReadHandler_ = read_h;
  cgiWriteHandler_ = write_h;
}

void ClientHandler::clearCgi() {
  if (cgiPid_ > 0) {
    kill(cgiPid_, SIGTERM);
    waitpid(cgiPid_, NULL, WNOHANG);
    cgiPid_ = -1;
  }
  if (cgiReadHandler_ != NULL) {
    delete cgiReadHandler_;
    cgiReadHandler_ = NULL;
  }
  if (cgiWriteHandler_ != NULL) {
    delete cgiWriteHandler_;
    cgiWriteHandler_ = NULL;
  }
}

void ClientHandler::handleCgiError() {
  rawOutBuffer_.clear();
  rawOutBufferOffset_ = 0;

  clearCgi();

  const std::map<std::string, std::string>& headers = request_.getHeaders();
  std::map<std::string, std::string>::const_iterator it = headers.find("host");
  if (it != headers.end()) {
    router_.setErrorResponse(response_, 500, it->second, serverPort_);
  } else {
    router_.setErrorResponse(response_, 500, "", serverPort_);
  }

  appendToOutput(response_.serialize());
  changeState(WRITING_RESPONSE);
}

ClientHandler::ClientState ClientHandler::getState() const {
  return state_;
}

EpollManager& ClientHandler::getEpollManager() const {
  return epollManager_;
}

Router& ClientHandler::getRouter() const {
  return router_;
}

int ClientHandler::getServerPort() const {
  return serverPort_;
}

const std::string& ClientHandler::getClientIp() const {
  return clientIp_;
}
