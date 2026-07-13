// STUB — Responsabilidad de Ángel. Implementación mínima para enlazar.
#include "Router.hpp"

Router::Router(const ConfigMap& config, char** envp)
    : globalConfig_(config), envp_(envp) {}

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
  (void)req; (void)client; (void)server_port;
  res.setStatusCode(200, "OK");
  res.setHeader("Content-Type", "text/html");
  res.setBody("<h1>Router stub</h1>");
}
