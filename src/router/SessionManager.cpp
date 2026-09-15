#include "SessionManager.hpp"

#include <cstdlib>
#include <ctime>
#include <vector>

SessionData::SessionData()
  : clientIp(""), visitCount(0), createdAt(0), lastActivityTime(0) {
}

SessionManager::SessionManager(time_t timeoutInSeconds) : sessionTimeout_(timeoutInSeconds) {
}

SessionManager::~SessionManager() {
}

std::string SessionManager::generateSessionId() const {
  static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  static const size_t charsetSize = sizeof(charset) - 1;

  std::string id;
  id.reserve(32);
  for (size_t i = 0; i < 32; ++i) {
    id += charset[std::rand() % charsetSize];
  }
  return id;
}

std::string SessionManager::createSession(const std::string& clientIp) {
  cleanExpiredSessions();
  std::string id = generateSessionId();
  while (activeSessions_.find(id) != activeSessions_.end()) {
    id = generateSessionId();
  }
  SessionData data;
  data.clientIp = clientIp;
  data.visitCount = 1;
  data.createdAt = std::time(NULL);
  data.lastActivityTime = data.createdAt;
  activeSessions_[id] = data;
  return id;
}

bool SessionManager::getSession(const std::string& sessionId, SessionData& outData) {
  cleanExpiredSessions();
  std::map<std::string, SessionData>::iterator it = activeSessions_.find(sessionId);
  if (it == activeSessions_.end()) {
    return false;
  }
  it->second.visitCount++;
  it->second.lastActivityTime = std::time(NULL);
  outData = it->second;
  return true;
}

void SessionManager::destroySession(const std::string& sessionId) {
  activeSessions_.erase(sessionId);
}

void SessionManager::cleanExpiredSessions() {
  time_t now = std::time(NULL);
  std::vector<std::string> expired;
  for (std::map<std::string, SessionData>::const_iterator it = activeSessions_.begin();
       it != activeSessions_.end(); ++it) {
    if (now - it->second.lastActivityTime > sessionTimeout_) {
      expired.push_back(it->first);
    }
  }
  for (size_t i = 0; i < expired.size(); ++i) {
    activeSessions_.erase(expired[i]);
  }
}

size_t SessionManager::getActiveSessionCount() const {
  return activeSessions_.size();
}
