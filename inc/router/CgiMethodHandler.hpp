#ifndef CGIMETHODHANDLER_HPP_
#define CGIMETHODHANDLER_HPP_

#include "IMethodHandler.hpp"

class CgiMethodHandler : public IMethodHandler {
private:
  CgiMethodHandler(const CgiMethodHandler& other);
  CgiMethodHandler& operator=(const CgiMethodHandler& other);

public:
  CgiMethodHandler();
  virtual ~CgiMethodHandler();

  /**
   * @brief Handle the request and build the response.
   * @param req The parsed HTTP request.
   * @param res The HTTP response to be populated.
   * @param client Pointer to the client connection handler, allowing async
   * control (e.g. for CGI).
   */
  void handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client);
};
#endif  // CGIMETHODHANDLER_HPP_
