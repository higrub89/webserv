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
 * @brief Manages an active client socket, implementing HTTP transaction states
 * and non-blocking I/O.
 *
 * Replaces: ClientConnection.hpp
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

  // CGI tracking to prevent dangling pointers and resource leaks on client
  // disconnect
  CgiReadHandler* cgiReadHandler_;
  CgiWriteHandler* cgiWriteHandler_;
  pid_t cgiPid_;

  void processRequest();
  void resetForKeepAlive();

public:
  /**
   * @brief Construct a new ClientHandler.
   * @param fd The client socket file descriptor.
   * @param epoll_manager Reference to the central EpollManager.
   * @param router Reference to the application router.
   * @param server_port The local port this client connected to.
   * @param client_ip The remote IP address of the client.
   */
  ClientHandler(int fd, EpollManager& epoll_manager, Router& router,
                int server_port, const std::string& client_ip);
  virtual ~ClientHandler();

  // Implement AEventHandler interfaces
  virtual void onReadReady();   // Perform 1 recv() call, parse incrementally,
                                // transition to processing when complete.
  virtual void onWriteReady();  // Perform 1 send() call (partial writes safe),
                                // transition state when complete.
  virtual void
  onDisconnect();  // Clean up client resources and trigger socket closure.

  /**
   * @brief Append response bytes to the output buffer to be sent in the next
   * write cycles.
   */
  void appendToOutput(const std::vector<char>& data);

  /**
   * @brief Append raw bytes to the output buffer to be sent in the next write
   * cycles.
   */
  void appendToOutput(const char* data, size_t len);

  /**
   * @brief Transition the client's macro-state.
   */
  void changeState(ClientState new_state);

  /**
   * @brief Check if the connection has been idle for too long.
   */
  bool isTimedOut(time_t current_time) const;

  // Getters for CGI execution context
  EpollManager& getEpollManager() const { return epollManager_; }
  Router& getRouter() const { return router_; }
  int getServerPort() const { return serverPort_; }
  const std::string& getClientIp() const { return clientIp_; }

  /**
   * @brief Register the CGI process ID and handlers for clean up.
   */
  void registerCgi(pid_t pid, CgiReadHandler* read_h, CgiWriteHandler* write_h);

  /**
   * @brief Terminate and clean up the registered CGI handler.
   */
  void clearCgi();

  /**
   * @brief Handle CGI execution error: discards partial output and prepares a
   * 500 response.
   */
  void handleCgiError();
};

#endif  // CLIENTHANDLER_HPP_
