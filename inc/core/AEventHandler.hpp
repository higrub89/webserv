#ifndef AEVENTHANDLER_HPP_
#define AEVENTHANDLER_HPP_

#include <ctime>

/**
 * @class AEventHandler
 * @brief Abstract base class representing any file descriptor monitored by the EpollManager.
 *
 * Replaces: IEventHandler.hpp (or direct raw socket management in old codebase)
 */
class AEventHandler {
protected:
  int fd_;

private:
  // Non-copyable (Orthodox Canonical Form requirement for resource classes)
  AEventHandler(const AEventHandler& other);
  AEventHandler& operator=(const AEventHandler& other);

public:
  /**
   * @brief Construct a new AEventHandler object.
   * @param fd The file descriptor to monitor.
   */
  AEventHandler(int fd);

  /**
   * @brief Virtual destructor to ensure proper cleanup of derived classes.
   */
  virtual ~AEventHandler();

  /**
   * @brief Get the managed file descriptor.
   * @return int The file descriptor.
   */
  int getFd() const;

  /**
   * @brief Called when the file descriptor has data available to read (EPOLLIN).
   */
  virtual void onReadReady() = 0;

  /**
   * @brief Called when the file descriptor is ready for writing without blocking (EPOLLOUT).
   */
  virtual void onWriteReady() = 0;

  /**
   * @brief Called when a disconnection, error, or hang-up occurs (EPOLLERR | EPOLLHUP |
   * EPOLLRDHUP).
   */
  virtual void onDisconnect() = 0;

  /**
   * @brief Check if the handler has timed out. Defaults to false.
   */
  virtual bool isTimedOut(time_t current_time) const;
};

#endif  // AEVENTHANDLER_HPP_
