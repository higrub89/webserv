#ifndef ROUTER_HPP_
#define ROUTER_HPP_

#include <map>
#include <string>

#include "ClientHandler.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "IMethodHandler.hpp"
#include "types/ConfigStructures.hpp"

/**
 * @class Router
 * @brief Resolves target server configs and dispatches requests to method
 * executors.
 *
 * Replaces: Path resolving and request dispatching in the old codebase.
 */
class Router {
private:
  const ConfigMap& globalConfig_;
  std::map<std::string, IMethodHandler*> methodRegistry_;

  char** envp_;

  // Resolves which server block should handle the request based on the Host
  // header string and port.
  const ServerConfig& resolveServer(const std::string& host,
                                    int server_port) const;

  // Matches the request URI to the longest matching location block defined in
  // the server config. (Longest prefix match)
  const LocationConfig* resolveLocation(const std::string& uri,
                                        const ServerConfig& server) const;

  /**
   * @brief setErrorResponse sets the response object to the appropriate error
   * page based on the error code and server configuration.
   * @param res The response object to set.
   * @param errorCode The HTTP error code (e.g., 404, 500).
   * @param server The server configuration to use for error page resolution.
   */
  void setErrorResponse(HttpResponse& res, int errorCode,
                        const ServerConfig& server) const;

public:
  /**
   * @brief Construct a new Router.
   * @param config The global server configuration structure.
   * @param envp The system environment variables array.
   */
  Router(const ConfigMap& config, char** envp);
  ~Router();

  /**
   * @brief Registers an HTTP method handler in the router registry.
   * @param method The HTTP method name (e.g., "GET", "POST").
   * @param handler Pointer to the handler instance.
   */
  void registerMethodHandler(const std::string& method,
                             IMethodHandler* handler);

  /**
   * @brief Resolves the target virtual server, checks route rules, and executes
   * the request.
   * @param req The parsed HTTP request.
   * @param res The response object to build.
   * @param client Pointer to the active client connection.
   * @param server_port The physical port on which the request was received.
   */
  void dispatch(const HttpRequest& req, HttpResponse& res,
                ClientHandler* client, int server_port);
};

#endif  // ROUTER_HPP_
