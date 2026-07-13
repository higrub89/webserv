#include "ClientHandler.hpp"

#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>

#include "CgiReadHandler.hpp"
#include "CgiWriteHandler.hpp"
#include "EpollManager.hpp"
#include "Router.hpp"
#include "Utils.hpp"

#define CLIENT_TIMEOUT_SECS 60
#define READ_BUF_SIZE 8192

// ─── Constructor / Destructor ───────────────────────────────────────────────

ClientHandler::ClientHandler(int fd, EpollManager& epoll_manager,
                             Router& router, int server_port,
                             const std::string& client_ip)
  : AEventHandler(fd),
    epollManager_(epoll_manager),
    router_(router),
    state_(READING_REQUEST),
    lastActivityTime_(std::time(NULL)),
    parser_(10485760),  // Initialize parser with a default max body size of
                        // 10MB (TODO)
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
      // Error de parseo: construir respuesta de error
      response_.setStatusCode(parser_.getErrorCode(), "Bad Request");
      response_.setHeader("Connection", "close");
      response_.setBody("Bad Request");
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
  if (rawOutBuffer_.empty())
    return;

  ssize_t n = send(fd_, &rawOutBuffer_[0], rawOutBuffer_.size(), MSG_NOSIGNAL);

  if (n > 0) {
    rawOutBuffer_.erase(rawOutBuffer_.begin(), rawOutBuffer_.begin() + n);
    lastActivityTime_ = std::time(NULL);

    if (rawOutBuffer_.empty()) {
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
// STUB TEMPORAL: respuesta hardcoded hasta que Alex y Ángel tengan sus módulos

void ClientHandler::processRequest() {
  state_ = PROCESSING;

  // TODO: cuando Router esté implementado, descomentar:
  // router_.dispatch(request_, response_, this, serverPort_);

  std::string body =
    "<html><body>"
    "<h1>WebServer 42</h1>"
    "<p>epoll layer OK &mdash; Ruben</p>"
    "<pre>";

  // Incluir la petición raw en la respuesta para debug
  body.append(rawInBuffer_.begin(), rawInBuffer_.end());
  body += "</pre></body></html>";

  std::ostringstream oss;
  oss << "HTTP/1.1 200 OK\r\n"
      << "Content-Type: text/html\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Connection: close\r\n"
      << "\r\n"
      << body;

  std::string raw = oss.str();
  std::vector<char> data(raw.begin(), raw.end());
  appendToOutput(data);

  changeState(WRITING_RESPONSE);
}

// ─── resetForKeepAlive ──────────────────────────────────────────────────────

void ClientHandler::resetForKeepAlive() {
  rawInBuffer_.clear();
  rawOutBuffer_.clear();
  parser_.reset();
  request_.reset();
  response_.reset();
  lastActivityTime_ = std::time(NULL);
  changeState(READING_REQUEST);
}

// ─── Utilidades ─────────────────────────────────────────────────────────────

void ClientHandler::appendToOutput(const std::vector<char>& data) {
  rawOutBuffer_.insert(rawOutBuffer_.end(), data.begin(), data.end());
}

void ClientHandler::appendToOutput(const char* data, size_t len) {
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
void ClientHandler::registerCgi(pid_t pid, CgiReadHandler* read_h,
                                CgiWriteHandler* write_h) {
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

  clearCgi();

  // TODO:
  // Replace this hardcoded default page generation with a centralized error
  // response builder (e.g., buildErrorResponse(500)). It should look up the
  // configured ServerConfig error pages (ServerConfig::error_pages) for a
  // custom 500 error page. If found and readable, use that file's content as
  // the body; otherwise, fall back to this default HTML page.
  response_.reset();
  response_.setStatusCode(500, "Internal Server Error");

  std::string err_body =
    "<html>\r\n"
    "<head><title>500 Internal Server Error</title></head>\r\n"
    "<body>\r\n"
    "<center><h1>500 Internal Server Error</h1></center>\r\n"
    "<hr><center>Webserv/1.0</center>\r\n"
    "</body>\r\n"
    "</html>\r\n";

  response_.setBody(err_body);
  response_.setHeader("Content-Type", "text/html");
  response_.setHeader("Content-Length", Utils::toString(err_body.size()));
  response_.setHeader("Connection", "close");  // Close connection after error

  appendToOutput(response_.serialize());
  changeState(WRITING_RESPONSE);
}
