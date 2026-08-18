#ifndef CGIWRITEHANDLER_HPP_
#define CGIWRITEHANDLER_HPP_

#include <sys/epoll.h>

#include <vector>

#include "AEventHandler.hpp"

class EpollManager;
class ClientHandler;

/**
 * @class CgiWriteHandler
 * @brief Monitors the stdin pipe of a CGI child process to feed it the HTTP request body asynchronously.
 *
 * Inherits from AEventHandler to stream the request payload into the child process's stdin
 * non-blockingly, unregistering and closing the write pipe once transmission is complete.
 */
class CgiWriteHandler : public AEventHandler {
private:
  EpollManager& epollManager_;
  ClientHandler& client_;
  const std::vector<char>& bodyBuffer_;
  size_t bytesWritten_;

public:
  CgiWriteHandler(int stdin_fd, EpollManager& epoll_manager, ClientHandler& client, const std::vector<char>& body);
  virtual ~CgiWriteHandler();

  virtual void onReadReady();
  virtual void onWriteReady();
  virtual void onDisconnect();
};

#endif  // CGIWRITEHANDLER_HPP_
