#ifndef CGIEXECUTOR_HPP_
#define CGIEXECUTOR_HPP_

#include <sys/types.h>

#include <string>
#include <vector>

#include "CgiStructures.hpp"
#include "ConfigStructures.hpp"
#include "IMethodExecutor.hpp"

/**
 * @class CgiExecutor
 * @brief Handles CGI execution for dynamic requests matching configured CGI extensions.
 *
 * Implements IMethodExecutor to launch external CGI scripts asynchronously using fork,
 * non-blocking pipes, and epoll-driven event handlers (CgiReadHandler and CgiWriteHandler).
 */
class CgiExecutor : public IMethodExecutor {
private:
  char** envp_;

  /**
   * @brief Extracts script path and query string from the request URI.
   * @param ctx The CGI request context.
   */
  void parseUri(CgiRequestContext& ctx);

  /**
   * @brief Resolves the absolute filesystem path to the script and validates interpreter permissions.
   * @param ctx The CGI request context.
   * @param location The matching location configuration block.
   * @return true if paths are valid and executable, false otherwise.
   */
  bool resolveAndValidatePaths(CgiRequestContext& ctx, const LocationConfig& location);

  /**
   * @brief Creates non-blocking unidirectional communication pipes for CGI stdin and stdout.
   * @param ctx The CGI request context.
   * @param in_pipe Array holding the stdin pipe file descriptors [read, write].
   * @param out_pipe Array holding the stdout pipe file descriptors [read, write].
   * @return true if both pipes were created successfully, false otherwise.
   */
  bool createPipes(CgiRequestContext& ctx, int in_pipe[2], int out_pipe[2]);

  /**
   * @brief Child process execution routine: duplicates pipe ends to stdin/stdout, sets working directory, and invokes execve.
   * @param ctx The CGI request context.
   * @param in_pipe Stdin pipe file descriptors.
   * @param out_pipe Stdout pipe file descriptors.
   * @param child_env Null-terminated environment variable vector for execve.
   */
  void executeChild(CgiRequestContext& ctx, int in_pipe[2], int out_pipe[2], char** child_env);

  /**
   * @brief Parent process routine: configures non-blocking descriptors, registers CGI event handlers in epoll, and updates client state.
   * @param ctx The CGI request context.
   * @param pid The process ID of the spawned child process.
   * @param in_pipe Stdin pipe file descriptors.
   * @param out_pipe Stdout pipe file descriptors.
   */
  void setupParent(CgiRequestContext& ctx, pid_t pid, int in_pipe[2], int out_pipe[2]);

  /**
   * @brief Constructs the standard CGI/1.1 environment variables array.
   * @param ctx The CGI request context.
   * @param env_strings Storage vector ensuring string memory remains valid until execve.
   * @return Vector of C-strings (char*) terminated with NULL for execve.
   */
  std::vector<char*> buildChildEnv(CgiRequestContext& ctx, std::vector<std::string>& env_strings);

public:
  CgiExecutor(char** envp);
  virtual ~CgiExecutor();

  /**
   * @brief Dispatches and initiates execution of a CGI script.
   * @param req The parsed HTTP request.
   * @param res The HTTP response object to populate in case of early errors.
   * @param client Pointer to the active client connection handler.
   * @param location The matching location configuration.
   */
  void handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client, const LocationConfig& location);
};

#endif  // CGIEXECUTOR_HPP_
