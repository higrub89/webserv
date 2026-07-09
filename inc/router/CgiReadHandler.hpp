#ifndef CGIREADHANDLER_HPP_
#define CGIREADHANDLER_HPP_

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "AEventHandler.hpp"
#include "ClientHandler.hpp"
#include "EpollManager.hpp"

class EpollManager;
class ClientHandler;

/**
 * @class CgiReadHandler
 * @brief Monitors the stdout pipe of a CGI child process asynchronously.
 *
 * Replaces: None (New helper to satisfy the strict non-blocking CGI requirement
 * of webserv)
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
  CgiReadHandler(int stdout_fd, EpollManager* epoll_manager,
                 ClientHandler* client, pid_t cgi_pid);
  virtual ~CgiReadHandler();

  // Implement AEventHandler interfaces
  virtual void onReadReady();   // Reads a chunk from CGI pipe, appends to
                                // client's output, detects EOF.
  virtual void onWriteReady();  // No-op.
  virtual void onDisconnect();  // Cleans up epoll, closes pipe, and reaps the
                                // child process via non-blocking waitpid.
};

#endif  // CGIREADHANDLER_HPP_
