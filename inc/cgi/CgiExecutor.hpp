#ifndef CGIEXECUTOR_HPP_
#define CGIEXECUTOR_HPP_

#include <sys/types.h>

#include <string>
#include <vector>

#include "CgiStructures.hpp"
#include "ConfigStructures.hpp"
#include "IMethodExecutor.hpp"

class CgiExecutor : public IMethodExecutor {
public:
private:
  char** envp_;

  // Private helper methods
  void parseUri(CgiRequestContext& ctx);
  bool resolveAndValidatePaths(CgiRequestContext& ctx, const LocationConfig& location);
  bool createPipes(CgiRequestContext& ctx, int in_pipe[2], int out_pipe[2]);
  void executeChild(CgiRequestContext& ctx, int in_pipe[2], int out_pipe[2], char** child_env);
  void setupParent(CgiRequestContext& ctx, pid_t pid, int in_pipe[2], int out_pipe[2]);

  std::vector<char*> buildChildEnv(CgiRequestContext& ctx, std::vector<std::string>& env_strings);

public:
  CgiExecutor(char** envp);
  virtual ~CgiExecutor();

  /**
   * @brief Handle the request and build the response.
   * @param req The parsed HTTP request.
   * @param res The HTTP response to be populated.
   * @param client Pointer to the client connection handler, allowing async
   * control (e.g. for CGI).
   * @param location The resolved LocationConfig for the request URI.
   */
  void handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client, const LocationConfig& location);
};

#endif  // CGIEXECUTOR_HPP_
