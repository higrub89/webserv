#include "Router.hpp"

#include <algorithm>

#include "HttpError.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

Router::Router(const ConfigMap& config, char** /* envp */) : globalConfig_(config) {
}

Router::~Router() {
  for (std::map<std::string, IMethodExecutor*>::iterator it = methodRegistry_.begin(); it != methodRegistry_.end(); ++it)
    delete it->second;
}

void Router::registerMethodExecutor(const std::string& method, IMethodExecutor* executor) {
  methodRegistry_[method] = executor;
}

void Router::processSession(const HttpRequest& req, HttpResponse& res, ClientHandler* client) {
  std::string clientIp = "unknown";
  if (client != NULL) {
    clientIp = client->getClientIp();
  }

  std::string sessionId = "";
  const std::map<std::string, std::string>& cookies = req.getCookies();
  std::map<std::string, std::string>::const_iterator it = cookies.find("session_id");
  if (it != cookies.end()) {
    sessionId = it->second;
  }

  std::string resourceInfo = req.getMethod() + " " + req.getPath();

  SessionData session;
  if (!sessionId.empty() && sessionManager_.getSession(sessionId, session)) {
    Logger::info(resourceInfo + " | Session active [" + sessionId + "] | Client: " + session.clientIp +
                 " | Visits: " + Utils::toString(session.visitCount) +
                 " | Active sessions: " + Utils::toString(sessionManager_.getActiveSessionCount()));
  } else {
    std::string newId = sessionManager_.createSession(clientIp);
    res.setCookie("session_id", newId, "/", 1800, true);
    Logger::info(resourceInfo + " | New session created [" + newId + "] | Client: " + clientIp +
                 " | Visit #1 | Active sessions: " + Utils::toString(sessionManager_.getActiveSessionCount()));
  }
}

void Router::dispatch(const HttpRequest& req, HttpResponse& res, ClientHandler* client, int server_port) {
  const ServerConfig* serverPtr = NULL;
  std::map<std::string, std::string>::const_iterator it = req.getHeaders().find("host");
  if (it != req.getHeaders().end()) {
    serverPtr = &resolveServer(it->second, server_port);
  } else {
    serverPtr = &resolveServer("", server_port);
  }
  const ServerConfig& server = *serverPtr;
  std::string normalizedPath = Utils::normalizeUriPath(req.getPath());
  const LocationConfig* location = resolveLocation(normalizedPath, server);
  if (location == NULL) {
    HttpError::populate(res, 404, server);
    processSession(req, res, client);
    if (client) {
      client->changeState(ClientHandler::WRITING_RESPONSE);
    }
    return;
  }

  size_t effectiveLimit = server.client_max_body_size;
  if (location->client_max_body_size > 0) {
    effectiveLimit = location->client_max_body_size;
  }

  if (effectiveLimit > 0 && req.getBody().size() > effectiveLimit) {
    HttpError::populate(res, 413, server);
    processSession(req, res, client);
    if (client) {
      client->changeState(ClientHandler::WRITING_RESPONSE);
    }
    return;
  }

  // HTTP Redirection (location.return_redirect)
  if (!location->return_redirect.empty()) {
    res.reset();
    res.setStatusCode(302, "Found");
    res.setHeader("Location", location->return_redirect);
    res.setHeader("Connection", "close");
    res.setBody("");
    processSession(req, res, client);
    if (client) {
      client->changeState(ClientHandler::WRITING_RESPONSE);
    }
    return;
  }

  // Check if extension matches a configured CGI handler
  std::string ext = Utils::getExtension(normalizedPath);
  if (!ext.empty()) {
    std::map<std::string, std::string>::const_iterator cgiIt = location->cgi_handlers.find(ext);
    if (cgiIt != location->cgi_handlers.end()) {
      std::map<std::string, IMethodExecutor*>::iterator executorIt = methodRegistry_.find("CGI");
      if (executorIt != methodRegistry_.end()) {
        executorIt->second->handle(req, res, client, *location);
        if (!client || client->getState() != ClientHandler::WAITING_FOR_CGI) {
          if (res.getStatusCode() >= 400 && res.getBody().empty()) {
            HttpError::populate(res, res.getStatusCode(), server);
          }
          processSession(req, res, client);
          if (client) {
            client->changeState(ClientHandler::WRITING_RESPONSE);
          }
        }
        return;
      }
    }
  }

  // Allowed methods (location.allowed_methods) check
  if (!location->allowed_methods.empty()) {
    if (std::find(location->allowed_methods.begin(), location->allowed_methods.end(), req.getMethod()) == location->allowed_methods.end()) {
      HttpError::populate(res, 405, server);
      res.setHeader("Allow", Utils::join(location->allowed_methods, ", "));
      processSession(req, res, client);
      if (client) {
        client->changeState(ClientHandler::WRITING_RESPONSE);
      }
      return;
    }
  }

  std::map<std::string, IMethodExecutor*>::iterator methodIt = methodRegistry_.find(req.getMethod());
  if (methodIt == methodRegistry_.end()) {
    HttpError::populate(res, 501, server);
    processSession(req, res, client);
    if (client) {
      client->changeState(ClientHandler::WRITING_RESPONSE);
    }
    return;
  }

  methodIt->second->handle(req, res, client, *location);
  if (!client || client->getState() != ClientHandler::WAITING_FOR_CGI) {
    if (res.getStatusCode() >= 400 && res.getBody().empty()) {
      HttpError::populate(res, res.getStatusCode(), server);
    }
    processSession(req, res, client);
    if (client) {
      client->changeState(ClientHandler::WRITING_RESPONSE);
    }
  }
}

const ServerConfig& Router::resolveServer(const std::string& host, int server_port) const {
  size_t hostLen = host.length();
  size_t colon = host.find(':');
  if (colon != std::string::npos) {
    hostLen = colon;
  }

  for (std::map<std::string, ServerGroup>::const_iterator it = globalConfig_.begin(); it != globalConfig_.end(); ++it) {
    const ServerGroup& serverGroup = it->second;
    if (serverGroup.port == server_port) {
      if (hostLen == 0) {
        return serverGroup.servers[0];
      }
      for (std::vector<ServerConfig>::const_iterator serverIt = serverGroup.servers.begin(); serverIt != serverGroup.servers.end(); ++serverIt) {
        const ServerConfig& server = *serverIt;
        for (std::vector<std::string>::const_iterator nameIt = server.server_names.begin(); nameIt != server.server_names.end(); ++nameIt) {
          if (nameIt->length() == hostLen && host.compare(0, hostLen, *nameIt) == 0) {
            return server;
          }
        }
      }
      return serverGroup.servers[0];
    }
  }
  return globalConfig_.begin()->second.servers[0];
}

const LocationConfig* Router::resolveLocation(const std::string& uri, const ServerConfig& server) const {
  const LocationConfig* bestMatch = NULL;
  size_t bestMatchLength = 0;
  for (std::map<std::string, LocationConfig>::const_iterator it = server.locations.begin(); it != server.locations.end(); ++it) {
    const std::string& locationPath = it->first;
    if (locationPath.length() <= uri.length() && uri.compare(0, locationPath.length(), locationPath) == 0) {
      if (locationPath.length() > bestMatchLength) {
        bestMatch = &it->second;
        bestMatchLength = locationPath.length();
      }
    } else if (locationPath.length() == uri.length() + 1 &&
               locationPath[locationPath.length() - 1] == '/' &&
               locationPath.compare(0, uri.length(), uri) == 0) {
      if (locationPath.length() > bestMatchLength) {
        bestMatch = &it->second;
        bestMatchLength = locationPath.length();
      }
    }
  }
  return bestMatch;
}

void Router::setErrorResponse(HttpResponse& res, int errorCode, const ServerConfig& server) const {
  HttpError::populate(res, errorCode, server);
}

void Router::setErrorResponse(HttpResponse& res, int errorCode, const std::string& host, int server_port) const {
  const ServerConfig& server = resolveServer(host, server_port);
  HttpError::populate(res, errorCode, server);
}

size_t Router::getMaxBodySizeForPort(int server_port) const {
  size_t maxLimit = 0;
  for (ConfigMap::const_iterator it = globalConfig_.begin(); it != globalConfig_.end(); ++it) {
    if (it->second.port == server_port) {
      for (size_t i = 0; i < it->second.servers.size(); ++i) {
        const ServerConfig& s = it->second.servers[i];
        if (s.client_max_body_size > maxLimit) {
          maxLimit = s.client_max_body_size;
        }
        for (std::map<std::string, LocationConfig>::const_iterator lit = s.locations.begin(); lit != s.locations.end(); ++lit) {
          if (lit->second.client_max_body_size > maxLimit) {
            maxLimit = lit->second.client_max_body_size;
          }
        }
      }
    }
  }
  return maxLimit;
}
