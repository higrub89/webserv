#ifndef IMETHODHANDLER_HPP_
#define IMETHODHANDLER_HPP_

#include "ConfigStructures.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class ClientHandler;

struct LocationConfig;

/**
 * @class IMethodHandler
 * @brief Strategy interface for handling HTTP methods (GET, POST, DELETE).
 *
 * Replaces: Complex if-else blocks checking request methods in the old handler.
 */
class IMethodHandler {
public:
  IMethodHandler() {}
  virtual ~IMethodHandler() {}

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

#endif  // IMETHODHANDLER_HPP_
