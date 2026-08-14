#include "Router.hpp"

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
    res.reset();
    res.setStatusCode(404);
    res.setHeader("Content-Type", "text/html");
    res.setHeader("Connection", "close");
    res.setBody("<h1>404 Not Found (No location match)</h1>");
    client->changeState(ClientHandler::WRITING_RESPONSE);
    return;
  }

  // TODO: Add methods and CGI validation / dispatching.
  (void)client;

  res.setStatusCode(200, "OK");
  res.setHeader("Content-Type", "text/html");
  res.setBody("<h1>Server: " + host + ", Location matched!</h1>");
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
