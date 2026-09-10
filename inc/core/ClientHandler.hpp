#ifndef CLIENTHANDLER_HPP_
#define CLIENTHANDLER_HPP_

#include <sys/types.h>

#include <ctime>
#include <vector>

#include "AEventHandler.hpp"
#include "HttpParser.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class EpollManager;
class Router;
class CgiReadHandler;
class CgiWriteHandler;

/**
 * @class ClientHandler
 * @brief Manages an active client socket, implementing HTTP transaction states and non-blocking I/O.
 *
 * Handles reading request chunks, invoking HttpParser incrementally, dispatching parsed requests
 * through Router, managing CGI execution state, and sending serialized responses back to the client.
 */
class ClientHandler : public AEventHandler {
public:
  enum ClientState {
    READING_REQUEST,
    PROCESSING,
    WAITING_FOR_CGI,
    WRITING_RESPONSE
  };

private:
  EpollManager& epollManager_;
  Router& router_;
  ClientState state_;
  time_t lastActivityTime_;

  std::vector<char> rawInBuffer_;
  std::vector<char> rawOutBuffer_;

  HttpParser parser_;
  HttpRequest request_;
  HttpResponse response_;
  int serverPort_;
  std::string clientIp_;

  // CGI tracking to prevent dangling pointers and resource leaks on client disconnect
  CgiReadHandler* cgiReadHandler_;
  CgiWriteHandler* cgiWriteHandler_;
  pid_t cgiPid_;

  /**
   * @brief Dispatches the fully parsed request through the Router.
   */
  void processRequest();

  /**
   * @brief Resets request, response, and parser states for HTTP Keep-Alive connections.
   */
  void resetForKeepAlive();

public:
  ClientHandler(int fd, EpollManager& epoll_manager, Router& router, int server_port, const std::string& client_ip);
  virtual ~ClientHandler();

  virtual void onReadReady();
  virtual void onWriteReady();
  virtual void onDisconnect();

  void appendToOutput(const std::vector<char>& data);
  void appendToOutput(const char* data, size_t len);

  void changeState(ClientState new_state);
  ClientState getState() const { return state_; }
  bool isTimedOut(time_t current_time) const;

  EpollManager& getEpollManager() const { return epollManager_; }
  Router& getRouter() const { return router_; }
  int getServerPort() const { return serverPort_; }
  const std::string& getClientIp() const { return clientIp_; }

  void registerCgi(pid_t pid, CgiReadHandler* read_h, CgiWriteHandler* write_h);
  void clearCgi();
  void handleCgiError();
};

#endif  // CLIENTHANDLER_HPP_
