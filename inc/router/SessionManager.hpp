#ifndef SESSIONMANAGER_HPP_
#define SESSIONMANAGER_HPP_

#include <ctime>
#include <map>
#include <string>

/**
 * @struct SessionData
 * @brief Represents active session data for a single client.
 */
struct SessionData {
  std::string username;
  time_t lastActivityTime;
};

/**
 * @class SessionManager
 * @brief Manages active sessions in memory, handles session lifecycles and
 * expirations.
 *
 * Part of: Cookies and Session Management (Bonus requirement)
 */
class SessionManager {
private:
  std::map<std::string, SessionData> activeSessions_;
  time_t sessionTimeout_;  // Timeout duration in seconds.

  // Generates a random alphanumeric session ID.
  std::string generateSessionId() const;

public:
  /**
   * @brief Construct a new SessionManager.
   * @param timeoutInSeconds Expiration timeout (defaults to 1800s / 30
   * minutes).
   */
  SessionManager(time_t timeoutInSeconds = 1800);
  ~SessionManager();

  /**
   * @brief Create a new session for a user.
   * @param username The authenticated username.
   * @return std::string The unique generated session ID.
   */
  std::string createSession(const std::string& username);

  /**
   * @brief Get and validate session data. Updates lastActivityTime.
   * @param sessionId The session ID token.
   * @param outData Target struct to copy the session data into if found.
   * @return true If session is active and not expired, false otherwise.
   */
  bool getSession(const std::string& sessionId, SessionData& outData);

  /**
   * @brief Destroy an existing session (e.g. on logout).
   * @param sessionId The session ID token.
   */
  void destroySession(const std::string& sessionId);

  /**
   * @brief Clean up all sessions that have exceeded the timeout limit.
   */
  void cleanExpiredSessions();
};

#endif  // SESSIONMANAGER_HPP_
