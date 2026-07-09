#ifndef CGIWRITEHANDLER_HPP_
#define CGIWRITEHANDLER_HPP_

#include <vector>

#include "AEventHandler.hpp"

class EpollManager;

/**
 * @class CgiWriteHandler
 * @brief Monitors the stdin pipe of a CGI child process to feed it the HTTP request body
 * asynchronously.
 *
 * Replaces: None (New helper to satisfy the strict non-blocking CGI requirement of webserv)
 */
class CgiWriteHandler : public AEventHandler {
private:
  EpollManager* epollManager_;
  std::vector<char> bodyBuffer_;
  size_t bytesWritten_;

  // Prevent copying (Orthodox Canonical Form requirement for resource classes)
  CgiWriteHandler(const CgiWriteHandler& other);
  CgiWriteHandler& operator=(const CgiWriteHandler& other);

public:
  /**
   * @brief Construct a new CgiWriteHandler.
   * @param stdin_fd The write-end of the pipe connected to CGI stdin.
   * @param epoll_manager Pointer to the central EpollManager.
   * @param body The HTTP request body payload to write to the CGI.
   */
  CgiWriteHandler(int stdin_fd, EpollManager* epoll_manager, const std::vector<char>& body);
  virtual ~CgiWriteHandler();

  // Implement AEventHandler interfaces
  virtual void onReadReady();  // No-op.
  virtual void
  onWriteReady();  // Writes a chunk of body to CGI, unregisters and closes pipe when done.
  virtual void onDisconnect();  // Cleans up and closes pipe on error.
};

#endif  // CGIWRITEHANDLER_HPP_
