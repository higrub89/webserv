#ifndef IMETHODHANDLER_HPP_
#define IMETHODHANDLER_HPP_

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class ClientHandler;

/**
 * @class IMethodHandler
 * @brief Strategy interface for handling HTTP methods (GET, POST, DELETE).
 *
 * Replaces: Complex if-else blocks checking request methods in the old handler.
 */
class IMethodHandler {
private:
  // Prevent copying (Orthodox Canonical Form requirement for interface classes)
  IMethodHandler(const IMethodHandler& other);
  IMethodHandler& operator=(const IMethodHandler& other);

public:
  IMethodHandler() {}
  virtual ~IMethodHandler() {}

  /**
   * @brief Handle the request and build the response.
   * @param req The parsed HTTP request.
   * @param res The HTTP response to be populated.
   * @param client Pointer to the client connection handler, allowing async control (e.g. for CGI).
   */
  virtual void handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client) = 0;
};

#endif  // IMETHODHANDLER_HPP_
