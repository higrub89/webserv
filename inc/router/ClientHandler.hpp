#ifndef CLIENTHANDLER_HPP_
#define CLIENTHANDLER_HPP_

#include <ctime>
#include <vector>

#include "AEventHandler.hpp"
#include "HttpParser.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class EpollManager;
class Router;

/**
 * @class ClientHandler
 * @brief Manages an active client socket, implementing HTTP transaction states and non-blocking
 * I/O.
 *
 * Replaces: ClientConnection.hpp
 */
class ClientHandler : public AEventHandler {
public:
  enum ClientState { READING_REQUEST, PROCESSING, WAITING_FOR_CGI, WRITING_RESPONSE };

private:
  EpollManager* epollManager_;
  Router* router_;
  ClientState state_;
  time_t lastActivityTime_;

  std::vector<char> rawInBuffer_;
  std::vector<char> rawOutBuffer_;

  HttpParser parser_;
  HttpRequest request_;
  HttpResponse response_;
  int serverPort_;

  void processRequest();
  void resetForKeepAlive();

  // Prevent copying (Orthodox Canonical Form requirement for resource classes)
  ClientHandler(const ClientHandler& other);
  ClientHandler& operator=(const ClientHandler& other);

public:
  /**
   * @brief Construct a new ClientHandler.
   * @param fd The client socket file descriptor.
   * @param epoll_manager Pointer to the central EpollManager.
   * @param router Pointer to the application router.
   * @param server_port The local port this client connected to.
   */
  ClientHandler(int fd, EpollManager* epoll_manager, Router* router, int server_port);
  virtual ~ClientHandler();

  // Implement AEventHandler interfaces
  virtual void onReadReady();  // Perform 1 recv() call, parse incrementally, transition to
                               // processing when complete.
  virtual void
  onWriteReady();  // Perform 1 send() call (partial writes safe), transition state when complete.
  virtual void onDisconnect();  // Clean up client resources and trigger socket closure.

  /**
   * @brief Append response bytes to the output buffer to be sent in the next write cycles.
   */
  void appendToOutput(const std::vector<char>& data);

  /**
   * @brief Transition the client's macro-state.
   */
  void changeState(ClientState new_state);

  /**
   * @brief Check if the connection has been idle for too long.
   */
  bool isTimedOut(time_t current_time) const;
};

#endif  // CLIENTHANDLER_HPP_
