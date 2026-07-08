#ifndef CGIHANDLER_HPP_
#define CGIHANDLER_HPP_

#include <sys/types.h>

#include <vector>

#include "AEventHandler.hpp"

class EpollManager;
class ClientHandler;

/**
 * @class CgiReadHandler
 * @brief Monitors the stdout pipe of a CGI child process asynchronously.
 *
 * Replaces: None (New helper to satisfy the strict non-blocking CGI requirement of webserv)
 */
class CgiReadHandler : public AEventHandler {
private:
  EpollManager* epollManager_;
  ClientHandler* client_;
  pid_t cgiPid_;

  // Prevent copying (Orthodox Canonical Form requirement for resource classes)
  CgiReadHandler(const CgiReadHandler& other);
  CgiReadHandler& operator=(const CgiReadHandler& other);

public:
  /**
   * @brief Construct a new CgiReadHandler.
   * @param stdout_fd The read-end of the pipe connected to CGI stdout.
   * @param epoll_manager Pointer to the central EpollManager.
   * @param client Pointer to the ClientHandler waiting for the CGI output.
   * @param cgi_pid The process ID of the CGI child process to reap later.
   */
  CgiReadHandler(int stdout_fd, EpollManager* epoll_manager, ClientHandler* client, pid_t cgi_pid);
  virtual ~CgiReadHandler();

  // Implement AEventHandler interfaces
  virtual void
  onReadReady();  // Reads a chunk from CGI pipe, appends to client's output, detects EOF.
  virtual void onWriteReady();  // No-op.
  virtual void onDisconnect();  // Cleans up epoll, closes pipe, and reaps the child process via
                                // non-blocking waitpid.
};

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

#endif  // CGIHANDLER_HPP_
