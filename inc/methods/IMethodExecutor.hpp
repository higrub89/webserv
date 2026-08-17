#ifndef IMETHODEXECUTOR_HPP_
#define IMETHODEXECUTOR_HPP_

#include "ConfigStructures.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class ClientHandler;
struct LocationConfig;

/**
 * @class IMethodExecutor
 * @brief Strategy interface for executing HTTP methods (GET, POST, DELETE, CGI).
 */
class IMethodExecutor {
public:
  IMethodExecutor() {}
  virtual ~IMethodExecutor() {}

  /**
   * @brief Executes the HTTP method logic for the given request and location.
   * @param req The parsed HTTP request.
   * @param res The HTTP response object to populate.
   * @param client Pointer to the active client connection (used for async CGI delegation).
   * @param location The matching location configuration.
   */
  virtual void handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client, const LocationConfig& location) = 0;
};

#endif  // IMETHODEXECUTOR_HPP_
