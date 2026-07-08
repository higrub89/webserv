#ifndef EPOLLMANAGER_HPP_
#define EPOLLMANAGER_HPP_

#include <sys/epoll.h>

#include <map>

#include "AEventHandler.hpp"

#define MAX_EVENTS 1024
#define EPOLL_TIMEOUT_MS 5000

/**
 * @class EpollManager
 * @brief Manages the epoll event loop and dispatches events to registered handlers.
 *
 * Replaces: PollManager.hpp
 */
class EpollManager {
private:
  int epollFd_;
  bool running_;
  struct epoll_event events_[MAX_EVENTS];

  // Map tracking active handlers (fd -> AEventHandler*).
  std::map<int, AEventHandler*> handlers_;

  // Sweeps monitored handlers to close connections that have timed out
  void cleanupTimeouts();

  // Prevent copying (Orthodox Canonical Form requirement for resource classes)
  EpollManager(const EpollManager& other);
  EpollManager& operator=(const EpollManager& other);

public:
  EpollManager();
  ~EpollManager();

  /**
   * @brief Creates the epoll instance.
   */
  void init();

  /**
   * @brief Runs the main loop, blocking on epoll_wait and dispatching events.
   */
  void run();

  /**
   * @brief Signals the event loop to stop running.
   */
  void stop();

  /**
   * @brief Registers a handler in epoll.
   * @param handler The handler to register.
   * @param events The epoll events bitmask (e.g., EPOLLIN, EPOLLOUT, EPOLLRDHUP).
   */
  void addHandler(AEventHandler* handler, uint32_t events);

  /**
   * @brief Modifies the registered events for a handler.
   * @param handler The handler to modify.
   * @param events The new epoll events bitmask.
   */
  void updateHandlerEvents(AEventHandler* handler, uint32_t events);

  /**
   * @brief Removes a handler from epoll monitoring.
   * @param handler The handler to remove.
   */
  void removeHandler(AEventHandler* handler);
};

#endif  // EPOLLMANAGER_HPP_
