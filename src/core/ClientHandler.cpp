#include "ClientHandler.hpp"

#include <unistd.h>

#include "CgiReadHandler.hpp"
#include "CgiWriteHandler.hpp"
#include "Utils.hpp"

ClientHandler::ClientHandler(int fd, EpollManager& epoll_manager,
                             Router& router, int server_port,
                             const std::string& client_ip)
  : AEventHandler(fd),
    epollManager_(epoll_manager),
    router_(router),
    state_(READING_REQUEST),
    lastActivityTime_(time(NULL)),
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
}

void ClientHandler::onReadReady() {
  // TODO: Implement incremental request reading and parsing
}

void ClientHandler::onWriteReady() {
  // TODO: Implement non-blocking response writing
}

void ClientHandler::onDisconnect() {
  // TODO: Implement clean client socket teardown
}

void ClientHandler::appendToOutput(const std::vector<char>& data) {
  rawOutBuffer_.insert(rawOutBuffer_.end(), data.begin(), data.end());
}

void ClientHandler::appendToOutput(const char* data, size_t len) {
  rawOutBuffer_.insert(rawOutBuffer_.end(), data, data + len);
}

void ClientHandler::changeState(ClientState new_state) {
  state_ = new_state;
}

bool ClientHandler::isTimedOut(time_t current_time) const {
  // TODO: Implement custom client timeout logic
  (void)current_time;
  return false;
}

void ClientHandler::registerCgi(pid_t pid, CgiReadHandler* read_h,
                                CgiWriteHandler* write_h) {
  cgiPid_ = pid;
  cgiReadHandler_ = read_h;
  cgiWriteHandler_ = write_h;
}

void ClientHandler::clearCgi() {
  if (cgiReadHandler_ != NULL) {
    delete cgiReadHandler_;
    cgiReadHandler_ = NULL;
  }
  if (cgiWriteHandler_ != NULL) {
    delete cgiWriteHandler_;
    cgiWriteHandler_ = NULL;
  }
  cgiPid_ = -1;
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
