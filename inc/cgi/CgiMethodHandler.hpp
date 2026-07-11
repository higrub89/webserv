#ifndef CGIMETHODHANDLER_HPP_
#define CGIMETHODHANDLER_HPP_

#include "ConfigStructures.hpp"
#include "IMethodHandler.hpp"

class CgiMethodHandler : public IMethodHandler {
private:
  char** envp_;

  std::string getCgiExecutable(const std::string& script_path,
                               const LocationConfig& locationConfig) const;

public:
  /**
   * @class ChildProcessExitException
   * @brief Exception thrown when a CGI child process exits with an error.
   */
  class ChildProcessExitException : public std::exception {
  public:
    virtual const char* what() const throw() {
      return "CGI child process exited with an error.";
    }
  };

  CgiMethodHandler(char** envp);
  virtual ~CgiMethodHandler();

  /**
   * @brief Handle the request and build the response.
   * @param req The parsed HTTP request.
   * @param res The HTTP response to be populated.
   * @param client Pointer to the client connection handler, allowing async
   * control (e.g. for CGI).
   * @param location The resolved LocationConfig for the request URI.
   */
  void handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client,
              const LocationConfig& location);
};
#endif  // CGIMETHODHANDLER_HPP_
