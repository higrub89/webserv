#ifndef SERVERHANDLER_HPP_
#define SERVERHANDLER_HPP_

#include <netinet/in.h>

#include "AEventHandler.hpp"
#include "types/ConfigStructures.hpp"

class EpollManager;
class Router;

/**
 * @class ServerHandler
 * @brief Represents a passive listening TCP socket bound to a specific port.
 *
 * Inherits from AEventHandler. Accepts incoming client connections non-blockingly upon
 * EPOLLIN readiness, instantiates ClientHandler objects, and registers them into EpollManager.
 */
class ServerHandler : public AEventHandler {
private:
  const ServerConfig& config_;
  EpollManager& epollManager_;
  Router& router_;
  struct sockaddr_in address_;
  std::string ip_;
  int port_;

public:
  ServerHandler(const std::string& ip, int port, const ServerConfig& config, EpollManager& epoll_manager, Router& router);
  virtual ~ServerHandler();

  void setup();

  virtual void onReadReady();
  virtual void onWriteReady();
  virtual void onDisconnect();
};

#endif  // SERVERHANDLER_HPP_
