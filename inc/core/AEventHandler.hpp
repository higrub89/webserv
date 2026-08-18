#ifndef AEVENTHANDLER_HPP_
#define AEVENTHANDLER_HPP_

#include <ctime>

/**
 * @class AEventHandler
 * @brief Abstract base class representing any file descriptor monitored by the EpollManager.
 *
 * Provides a polymorphic interface for epoll event dispatching across different I/O components
 * (listening server sockets, connected client sockets, and CGI pipes).
 */
class AEventHandler {
protected:
  int fd_;

public:
  AEventHandler(int fd);
  virtual ~AEventHandler();

  int getFd() const;

  virtual void onReadReady() = 0;
  virtual void onWriteReady() = 0;
  virtual void onDisconnect() = 0;

  virtual bool isTimedOut(time_t current_time) const;
};

#endif  // AEVENTHANDLER_HPP_
