#ifndef EPOLLMANAGER_HPP_
#define EPOLLMANAGER_HPP_

#include <sys/epoll.h>

#include <map>

#include "AEventHandler.hpp"

#define MAX_EVENTS 1024
#define EPOLL_TIMEOUT_MS 5000

/**
 * @class EpollManager
 * @brief Manages the central Linux epoll event loop and dispatches I/O notifications.
 *
 * Maintains a registry of active AEventHandler pointers mapped by file descriptor,
 * invokes their callbacks upon epoll_wait events, and periodically cleans up timed-out connections.
 */
class EpollManager {
private:
  int epollFd_;
  bool running_;
  struct epoll_event events_[MAX_EVENTS];

  // Map tracking active handlers (fd -> AEventHandler*).
  std::map<int, AEventHandler*> handlers_;

  /**
   * @brief Sweeps monitored handlers to detect and close connections that exceeded idle timeouts.
   */
  void cleanupTimeouts();

public:
  EpollManager();
  ~EpollManager();

  void init();
  void run();
  void stop();

  void addHandler(AEventHandler* handler, uint32_t events);
  void updateHandlerEvents(AEventHandler* handler, uint32_t events);
  void removeHandler(AEventHandler* handler);
};

#endif  // EPOLLMANAGER_HPP_
