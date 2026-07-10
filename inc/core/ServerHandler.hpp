#ifndef SERVERHANDLER_HPP_
#define SERVERHANDLER_HPP_

#include <netinet/in.h>

#include "AEventHandler.hpp"
#include "types/ConfigStructures.hpp"

class EpollManager;
class Router;

/**
 * @class ServerHandler
 * @brief Represents a listening socket on a specific port.
 *
 * Replaces: ServerSocket.hpp
 */
class ServerHandler : public AEventHandler {
private:
  const ServerConfig& config_;
  EpollManager& epollManager_;
  Router& router_;
  struct sockaddr_in address_;
  int port_;

public:
  /**
   * @brief Construct a new ServerHandler.
   * @param port The port to bind and listen on.
   * @param config Configuration rules for this virtual server.
   * @param epoll_manager Reference to the event loop manager.
   * @param router Reference to the router for dispatching requests.
   */
  ServerHandler(int port, const ServerConfig& config,
                EpollManager& epoll_manager, Router& router);
  virtual ~ServerHandler();

  /**
   * @brief Binds the socket to the port, sets it to non-blocking, and calls
   * listen().
   */
  void setup();

  // Implement AEventHandler interfaces
  virtual void onReadReady();   // Accept incoming connection, instantiate
                                // ClientHandler, and register it in epoll.
  virtual void onWriteReady();  // No-op for listening sockets.
  virtual void
  onDisconnect();  // Error handling / recovery for the listening socket.
};

#endif  // SERVERHANDLER_HPP_
