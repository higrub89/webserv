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

  const ServerConfig& server = resolveVirtualHost(host, server_port);

  (void)server;
  (void)client;
  res.setStatusCode(200, "OK");
  res.setHeader("Content-Type", "text/html");
  res.setBody("<h1>Virtual Host Resolved: " + host + " :)</h1>");
}

const ServerConfig& Router::resolveVirtualHost(const std::string& host,
                                               int server_port) const {
  for (std::map<std::string, VirtualHostGroup>::const_iterator it =
         globalConfig_.begin();
       it != globalConfig_.end(); ++it) {
    const VirtualHostGroup& vhostGroup = it->second;
    if (vhostGroup.port == server_port) {
      if (host.empty()) {
        return vhostGroup.servers[0];
      }
      for (std::vector<ServerConfig>::const_iterator serverIt =
             vhostGroup.servers.begin();
           serverIt != vhostGroup.servers.end(); ++serverIt) {
        const ServerConfig& server = *serverIt;
        for (std::vector<std::string>::const_iterator nameIt =
               server.server_names.begin();
             nameIt != server.server_names.end(); ++nameIt) {
          if (*nameIt == host) {
            return server;
          }
        }
      }
      return vhostGroup.servers[0];
    }
  }
  return globalConfig_.begin()->second.servers[0];
}

/* TODO
const RouteConfig& Router::resolveLocation(const std::string& uri,
                                           const ServerConfig& server) const {
  (void)uri;
  (void)server;
}
*/
