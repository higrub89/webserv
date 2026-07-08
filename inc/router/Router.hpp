#ifndef ROUTER_HPP_
#define ROUTER_HPP_

#include <map>
#include <string>

#include "ClientHandler.hpp"
#include "ConfigStructures.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "IMethodHandler.hpp"

/**
 * @class Router
 * @brief Resolves target server configs and dispatches requests to method executors.
 *
 * Replaces: Path resolving and request dispatching in the old codebase.
 */
class Router {
private:
  const ConfigMap& globalConfig_;
  std::map<std::string, IMethodHandler*> methodRegistry_;

  // Resolves which virtual host (server block) should handle the request based on Host header/port.
  const ServerConfig& resolveVirtualHost(const HttpRequest& req, int server_port) const;

  // Matches the request URI to the longest matching location block defined in the server config.
  const LocationConfig& resolveLocation(const std::string& uri, const ServerConfig& server) const;

  // Prevent copying (Orthodox Canonical Form requirement for non-copyable classes)
  Router(const Router& other);
  Router& operator=(const Router& other);

public:
  /**
   * @brief Construct a new Router.
   * @param config The global server configuration structure.
   */
  Router(const ConfigMap& config);
  ~Router();

  /**
   * @brief Registers an HTTP method handler in the router registry.
   * @param method The HTTP method name (e.g., "GET", "POST").
   * @param handler Pointer to the handler instance.
   */
  void registerMethodHandler(const std::string& method, IMethodHandler* handler);

  /**
   * @brief Resolves the target virtual server, checks route rules, and executes the request.
   * @param req The parsed HTTP request.
   * @param res The response object to build.
   * @param client Pointer to the active client connection.
   * @param server_port The physical port on which the request was received.
   */
  void dispatch(const HttpRequest& req, HttpResponse& res, ClientHandler* client, int server_port);
};

#endif  // ROUTER_HPP_
