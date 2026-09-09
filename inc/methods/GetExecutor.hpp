#ifndef GETEXECUTOR_HPP_
#define GETEXECUTOR_HPP_

#include "IMethodExecutor.hpp"

/**
 * @class GetExecutor
 * @brief Concrete implementation of IMethodExecutor for handling HTTP GET requests.
 */
class GetExecutor : public IMethodExecutor {
public:
  GetExecutor();
  virtual ~GetExecutor();

  /**
   * @brief Executes the GET method logic for the given request and location.
   * @param req The parsed HTTP request.
   * @param res The HTTP response object to populate.
   * @param client Pointer to the active client connection (used for async CGI delegation).
   * @param location The matching location configuration.
   */
  virtual void handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client, const LocationConfig& location);

private:
  std::string getMimeType(const std::string& ext) const;
  void serveFile(const std::string& path, HttpResponse& res) const;
  void generateAutoindex(const std::string& dirPath, const std::string& uriPath, HttpResponse& res) const;
};

#endif  // GETEXECUTOR_HPP_
