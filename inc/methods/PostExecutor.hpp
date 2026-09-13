#ifndef POSTEXECUTOR_HPP_
#define POSTEXECUTOR_HPP_

#include "IMethodExecutor.hpp"

/**
 * @class PostExecutor
 * @brief Concrete implementation of IMethodExecutor for handling HTTP POST requests.
 *
 * Implements file uploads (raw octet-stream and multipart/form-data) when upload_enable
 * is active in the matching location, or processes standard POST payload submissions.
 */
class PostExecutor : public IMethodExecutor {
public:
  PostExecutor();
  virtual ~PostExecutor();

  virtual void handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client, const LocationConfig& location);

private:
  void handleUpload(const HttpRequest& req, HttpResponse& res, const LocationConfig& location);
  void handleStandardPost(const HttpRequest& req, HttpResponse& res);
  std::string extractBoundary(const std::string& contentType) const;
  bool parseMultipart(const std::vector<char>& body, const std::string& boundary, std::string& filename, std::vector<char>& fileContent) const;
};

#endif  // POSTEXECUTOR_HPP_
