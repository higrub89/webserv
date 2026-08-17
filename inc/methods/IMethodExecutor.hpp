#ifndef IMETHODEXECUTOR_HPP_
#define IMETHODEXECUTOR_HPP_

#include "ConfigStructures.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class ClientHandler;

struct LocationConfig;

/**
 * @class IMethodExecutor
 * @brief Strategy interface for executing HTTP methods (GET, POST, DELETE,
 * CGI).
 *
 * Replaces: Complex if-else blocks checking request methods in the old handler.
 */
class IMethodExecutor {
public:
  IMethodExecutor() {}
  virtual ~IMethodExecutor() {}

  /**
   * @brief Handle the request and build the response.
   * @param req The parsed HTTP request.
   * @param res The HTTP response to be populated.
   * @param client Pointer to the client connection handler, allowing async
   * control (e.g. for CGI).
   * @param location The resolved LocationConfig for the request URI.
   */
  virtual void handle(const HttpRequest& req, HttpResponse& res,
                      ClientHandler* client,
                      const LocationConfig& location) = 0;
};

#endif  // IMETHODEXECUTOR_HPP_
