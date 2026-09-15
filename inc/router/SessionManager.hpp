#ifndef SESSIONMANAGER_HPP_
#define SESSIONMANAGER_HPP_

#include <ctime>
#include <map>
#include <string>

/**
 * @struct SessionData
 * @brief Holds authentication and timestamp state for an active client session.
 */
struct SessionData {
  std::string clientIp;
  size_t visitCount;
  time_t createdAt;
  time_t lastActivityTime;

  SessionData();
};

/**
 * @class SessionManager
 * @brief In-memory session store providing session creation, lookup, expiration, and invalidation.
 */
class SessionManager {
private:
  std::map<std::string, SessionData> activeSessions_;
  time_t sessionTimeout_;  // Timeout duration in seconds

  /**
   * @brief Generates a cryptographically-suitable random alphanumeric session token.
   * @return Generated session ID string.
   */
  std::string generateSessionId() const;

public:
  SessionManager(time_t timeoutInSeconds = 1800);
  ~SessionManager();

  std::string createSession(const std::string& clientIp);
  bool getSession(const std::string& sessionId, SessionData& outData);
  void destroySession(const std::string& sessionId);
  void cleanExpiredSessions();
  size_t getActiveSessionCount() const;
};

#endif  // SESSIONMANAGER_HPP_
