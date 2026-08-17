#include "Router.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "CgiMethodHandler.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

Router::Router(const ConfigMap& config, char** envp)
  : globalConfig_(config), envp_(envp) {
}

Router::~Router() {
  for (std::map<std::string, IMethodHandler*>::iterator it =
         methodRegistry_.begin();
       it != methodRegistry_.end(); ++it)
    delete it->second;
}

void Router::registerMethodHandler(const std::string& method,
                                   IMethodHandler* handler) {
  methodRegistry_[method] = handler;
}

void Router::dispatch(const HttpRequest& req, HttpResponse& res,
                      ClientHandler* client, int server_port) {
  std::string host = "";
  std::map<std::string, std::string>::const_iterator it =
    req.getHeaders().find("host");
  if (it != req.getHeaders().end()) {
    host = it->second;
    size_t colon = host.find(':');
    if (colon != std::string::npos) {
      host = host.substr(0, colon);
    }
  }

  const ServerConfig& server = resolveServer(host, server_port);
  const LocationConfig* location = resolveLocation(req.getPath(), server);
  if (location == NULL) {
    setErrorResponse(res, 404, server);
    client->changeState(ClientHandler::WRITING_RESPONSE);
    return;
  }

  size_t effectiveLimit = server.client_max_body_size;
  if (location->client_max_body_size > 0) {
    effectiveLimit = location->client_max_body_size;
  }

  if (effectiveLimit > 0 && req.getBody().size() > effectiveLimit) {
    setErrorResponse(res, 413, server);
    client->changeState(ClientHandler::WRITING_RESPONSE);
    return;
  }

  // Allowed methods (location.allowed_methods) check
  if (!location->allowed_methods.empty()) {
    if (std::find(location->allowed_methods.begin(),
                  location->allowed_methods.end(),
                  req.getMethod()) == location->allowed_methods.end()) {
      setErrorResponse(res, 405, server);
      res.setHeader("Allow", Utils::join(location->allowed_methods, ", "));
      client->changeState(ClientHandler::WRITING_RESPONSE);
      return;
    }
  }

  // HTTP Redirect
  if (!location->return_redirect.empty()) {
    res.reset();
    res.setStatusCode(302);
    res.setHeader("Location", location->return_redirect);
    res.setHeader("Content-Type", "text/html");
    res.setHeader("Connection", "close");
    res.setBody("<h1>302 Found</h1><p>Redirecting to <a href=\"" +
                location->return_redirect + "\">" + location->return_redirect +
                "</a>...</p>");
    client->changeState(ClientHandler::WRITING_RESPONSE);
    return;
  }

  std::string extension = Utils::getExtension(req.getPath());
  std::map<std::string, std::string>::const_iterator cgiIt =
    location->cgi_handlers.find(extension);
  if (cgiIt != location->cgi_handlers.end()) {  // CGI request
    CgiMethodHandler cgiHandler(envp_);
    cgiHandler.handle(req, res, client, *location);
    return;
  }

  // Static Method Dispatch
  std::map<std::string, IMethodHandler*>::const_iterator methodIt =
    methodRegistry_.find(req.getMethod());
  if (methodIt == methodRegistry_.end()) {
    setErrorResponse(res, 501, server);
    client->changeState(ClientHandler::WRITING_RESPONSE);
    return;
  }

  methodIt->second->handle(req, res, client, *location);
}

const ServerConfig& Router::resolveServer(const std::string& host,
                                          int server_port) const {
  for (std::map<std::string, ServerGroup>::const_iterator it =
         globalConfig_.begin();
       it != globalConfig_.end(); ++it) {
    const ServerGroup& serverGroup = it->second;
    if (serverGroup.port == server_port) {
      if (host.empty()) {
        return serverGroup.servers[0];
      }
      for (std::vector<ServerConfig>::const_iterator serverIt =
             serverGroup.servers.begin();
           serverIt != serverGroup.servers.end(); ++serverIt) {
        const ServerConfig& server = *serverIt;
        for (std::vector<std::string>::const_iterator nameIt =
               server.server_names.begin();
             nameIt != server.server_names.end(); ++nameIt) {
          if (*nameIt == host) {
            return server;
          }
        }
      }
      return serverGroup.servers[0];
    }
  }
  return globalConfig_.begin()->second.servers[0];
}

const LocationConfig* Router::resolveLocation(
  const std::string& uri, const ServerConfig& server) const {
  const LocationConfig* bestMatch = NULL;
  size_t bestMatchLength = 0;
  for (std::map<std::string, LocationConfig>::const_iterator it =
         server.locations.begin();
       it != server.locations.end(); ++it) {
    const std::string& locationPath = it->first;
    if (locationPath.length() <= uri.length() &&
        uri.compare(0, locationPath.length(), locationPath) == 0) {
      if (locationPath.length() > bestMatchLength) {
        bestMatch = &it->second;
        bestMatchLength = locationPath.length();
      }
    }
  }
  return bestMatch;
}

void Router::setErrorResponse(HttpResponse& res, int errorCode,
                              const ServerConfig& server) const {
  res.reset();
  res.setStatusCode(errorCode);
  res.setHeader("Content-Type", "text/html");
  res.setHeader("Connection", "close");

  std::map<int, std::string>::const_iterator it =
    server.error_pages.find(errorCode);
  if (it != server.error_pages.end()) {
    const std::string& errorPagePath = it->second;
    std::ifstream errorPageFile(errorPagePath.c_str());
    if (errorPageFile.is_open()) {
      std::stringstream buffer;
      buffer << errorPageFile.rdbuf();
      res.setBody(buffer.str());
      return;
    } else {
      // warning level does not exist yet, using info for now
      Logger::info("Custom error page not found or unreadable: " +
                   errorPagePath);
    }
  }

  res.setBody("<h1>" + Utils::toString(errorCode) + " " +
              HttpResponse::reasonPhrase(errorCode) + "</h1>");
}
