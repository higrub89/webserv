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

  (void)server;
  (void)client;
  res.setStatusCode(200, "OK");
  res.setHeader("Content-Type", "text/html");
  res.setBody("<h1>Server Resolved: " + host + " :)</h1>");
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

const LocationConfig& Router::resolveLocation(
  const std::string& uri, const ServerConfig& server) const {
  (void)uri;
  (void)server;
  // TODO
  return server.locations.begin()->second;
}
