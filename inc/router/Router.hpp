#ifndef ROUTER_HPP_
#define ROUTER_HPP_

#include <map>
#include <string>

#include "ClientHandler.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "IMethodExecutor.hpp"
#include "SessionManager.hpp"
#include "types/ConfigStructures.hpp"

/**
 * @class Router
 * @brief Resolves target server configs and dispatches requests to method executors or CGI.
 *
 * Implements Virtual Host matching (via Host header and port), Longest Prefix Matching for locations,
 * HTTP redirects (302), body size limits validation (413), allowed methods validation (405),
 * custom error page rendering, and delegation to CGI or static IMethodExecutors.
 */
class Router {
private:
  const ConfigMap& globalConfig_;
  std::map<std::string, IMethodExecutor*> methodRegistry_;
  char** envp_;
  SessionManager sessionManager_;

  void processSession(const HttpRequest& req, HttpResponse& res, ClientHandler* client);

  /**
   * @brief Resolves the matching ServerConfig based on the Host header string and listening port.
   * @param host The Host header value from the request.
   * @param server_port The physical port where the request arrived.
   * @return Reference to the matched ServerConfig (or default server for that port).
   */
  const ServerConfig& resolveServer(const std::string& host, int server_port) const;

  /**
   * @brief Matches the request URI against location blocks using the Longest Prefix Match algorithm.
   * @param uri The request path.
   * @param server The resolved server configuration.
   * @return Pointer to the matched LocationConfig, or NULL if no prefix matched.
   */
  const LocationConfig* resolveLocation(const std::string& uri, const ServerConfig& server) const;

  /**
   * @brief Populates the HttpResponse object with custom or default error pages.
   * @param res The response object to populate.
   * @param errorCode The HTTP status error code (e.g., 404, 405, 413, 500, 501).
   * @param server The server configuration containing configured error_pages.
   */
  void setErrorResponse(HttpResponse& res, int errorCode, const ServerConfig& server) const;

public:
  Router(const ConfigMap& config, char** envp);
  ~Router();

  void registerMethodExecutor(const std::string& method, IMethodExecutor* executor);
  void dispatch(const HttpRequest& req, HttpResponse& res, ClientHandler* client, int server_port);
  void setErrorResponse(HttpResponse& res, int errorCode, const std::string& host, int server_port) const;
  size_t getMaxBodySizeForPort(int server_port) const;
};

#endif  // ROUTER_HPP_
