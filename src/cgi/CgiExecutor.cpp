#include "CgiExecutor.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "CgiReadHandler.hpp"
#include "CgiWriteHandler.hpp"
#include "ClientHandler.hpp"
#include "ConfigStructures.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

CgiExecutor::CgiExecutor(char** envp) : envp_(envp) {
}

CgiExecutor::~CgiExecutor() {
}

void CgiExecutor::parseUri(CgiRequestContext& ctx) {
  std::string uri = ctx.req.getUri();
  ctx.script_name = uri;
  ctx.query_string = "";
  size_t question_mark = uri.find('?');
  if (question_mark != std::string::npos) {
    ctx.script_name = uri.substr(0, question_mark);
    ctx.query_string = uri.substr(question_mark + 1);
  }
}

bool CgiExecutor::resolveAndValidatePaths(CgiRequestContext& ctx, const LocationConfig& location) {
  std::string root = location.root_dir;
  if (root.size() > 1 && root[root.size() - 1] == '/') {
    root.erase(root.size() - 1);
  }
  std::string relPath = ctx.script_name;
  std::string locPrefix = location.path;
  if (!locPrefix.empty()) {
    if (relPath.compare(0, locPrefix.size(), locPrefix) == 0) {
      relPath = relPath.substr(locPrefix.size());
    } else if (locPrefix[locPrefix.size() - 1] == '/' && (relPath + "/").compare(0, locPrefix.size(), locPrefix) == 0) {
      relPath = "";
    }
  }
  if (!relPath.empty() && relPath[0] != '/') {
    relPath = "/" + relPath;
  }
  ctx.script_path = root + relPath;
  std::string ext = Utils::getExtension(ctx.req.getUri());
  ctx.interpreter_path = "";

  std::map<std::string, std::string>::const_iterator it_cgi = location.cgi_handlers.find(ext);
  if (it_cgi != location.cgi_handlers.end()) {
    ctx.interpreter_path = it_cgi->second;
  }

  if (ctx.interpreter_path.empty()) {
    Logger::error("CGI extension not supported or no interpreter mapped for: " + ext);
    ctx.res.setStatusCode(500, "Internal Server Error");
    ctx.client->changeState(ClientHandler::WRITING_RESPONSE);
    return false;
  }

  if (access(ctx.interpreter_path.c_str(), X_OK) == -1) {
    Logger::error("CGI interpreter not executable or not found: " + ctx.interpreter_path);
    ctx.res.setStatusCode(500, "Internal Server Error");
    ctx.client->changeState(ClientHandler::WRITING_RESPONSE);
    return false;
  }

  if (access(ctx.script_path.c_str(), F_OK) == 0 && access(ctx.script_path.c_str(), R_OK) == -1) {
    Logger::error("CGI script not readable: " + ctx.script_path);
    ctx.res.setStatusCode(403, "Forbidden");
    ctx.client->changeState(ClientHandler::WRITING_RESPONSE);
    return false;
  }

  return true;
}

bool CgiExecutor::createPipes(CgiRequestContext& ctx, int in_pipe[2], int out_pipe[2]) {
  if (pipe(in_pipe) == -1) {
    Logger::error("CGI stdin pipe() failed: " + std::string(strerror(errno)));
    ctx.res.setStatusCode(500, "Internal Server Error");
    ctx.client->changeState(ClientHandler::WRITING_RESPONSE);
    return false;
  }
  if (pipe(out_pipe) == -1) {
    Logger::error("CGI stdout pipe() failed: " + std::string(strerror(errno)));
    close(in_pipe[0]);
    close(in_pipe[1]);
    ctx.res.setStatusCode(500, "Internal Server Error");
    ctx.client->changeState(ClientHandler::WRITING_RESPONSE);
    return false;
  }
  return true;
}

void CgiExecutor::executeChild(CgiRequestContext& ctx, int in_pipe[2], int out_pipe[2], char** child_env) {
  close(in_pipe[1]);
  close(out_pipe[0]);
  if (dup2(in_pipe[0], STDIN_FILENO) == -1) {
    Logger::error("CGI dup2 stdin failed: " + std::string(strerror(errno)));
    close(in_pipe[0]);
    close(out_pipe[1]);
    std::exit(1);
  }
  close(in_pipe[0]);
  if (dup2(out_pipe[1], STDOUT_FILENO) == -1) {
    Logger::error("CGI dup2 stdout failed: " + std::string(strerror(errno)));
    close(out_pipe[1]);
    std::exit(1);
  }
  close(out_pipe[1]);

  std::string abs_interpreter = ctx.interpreter_path;
  if (!abs_interpreter.empty() && abs_interpreter[0] != '/') {
    std::string pwd = "";
    if (envp_) {
      for (int i = 0; envp_[i] != NULL; ++i) {
        if (std::strncmp(envp_[i], "PWD=", 4) == 0) {
          pwd = envp_[i] + 4;
          break;
        }
      }
    }
    if (!pwd.empty()) {
      abs_interpreter = pwd + "/" + abs_interpreter;
    }
  }

  std::string script_filename = ctx.script_path;
  size_t last_slash = ctx.script_path.find_last_of('/');
  if (last_slash != std::string::npos) {
    std::string dir = ctx.script_path.substr(0, last_slash);
    script_filename = "./" + ctx.script_path.substr(last_slash + 1);
    if (chdir(dir.c_str()) == -1) {
      Logger::error("CGI chdir failed: " + std::string(strerror(errno)));
      std::exit(1);
    }
  }

  char* argv[3];
  argv[0] = const_cast<char*>(abs_interpreter.c_str());
  argv[1] = const_cast<char*>(script_filename.c_str());
  argv[2] = NULL;

  execve(argv[0], argv, child_env);
  Logger::error("CGI execve failed: " + std::string(strerror(errno)));
  std::exit(127);
}

void CgiExecutor::setupParent(CgiRequestContext& ctx, pid_t pid, int in_pipe[2], int out_pipe[2]) {
  close(in_pipe[0]);
  close(out_pipe[1]);

  fcntl(in_pipe[1], F_SETFL, O_NONBLOCK);
  fcntl(out_pipe[0], F_SETFL, O_NONBLOCK);

  CgiReadHandler* read_handler = NULL;
  CgiWriteHandler* write_handler = NULL;
  try {
    read_handler = new CgiReadHandler(out_pipe[0], ctx.client->getEpollManager(), *ctx.client, pid);
    out_pipe[0] = -1;
    if (!ctx.req.getBody().empty()) {
      write_handler = new CgiWriteHandler(in_pipe[1], ctx.client->getEpollManager(), *ctx.client, ctx.req.getBody());
      in_pipe[1] = -1;
    } else {
      close(in_pipe[1]);
      in_pipe[1] = -1;
    }
    ctx.client->registerCgi(pid, read_handler, write_handler);
    ctx.client->changeState(ClientHandler::WAITING_FOR_CGI);
  } catch (...) {
    delete read_handler;
    delete write_handler;
    if (in_pipe[1] != -1) {
      close(in_pipe[1]);
    }
    if (out_pipe[0] != -1) {
      close(out_pipe[0]);
    }
    kill(pid, SIGKILL);
    waitpid(pid, NULL, WNOHANG);
    ctx.res.setStatusCode(500, "Internal Server Error");
    ctx.client->changeState(ClientHandler::WRITING_RESPONSE);
  }
}

std::vector<char*> CgiExecutor::buildChildEnv(CgiRequestContext& ctx, std::vector<std::string>& env_strings) {
  const std::map<std::string, std::string>& headers = ctx.req.getHeaders();
  env_strings.reserve(10 + headers.size());  // 10 static env vars + headers

  env_strings.push_back("REQUEST_METHOD=" + ctx.req.getMethod());
  env_strings.push_back("SCRIPT_NAME=" + ctx.script_name);
  env_strings.push_back("SCRIPT_FILENAME=" + ctx.script_path);
  env_strings.push_back("PATH_INFO=" + ctx.script_name);
  env_strings.push_back("PATH_TRANSLATED=" + ctx.script_path);
  env_strings.push_back("QUERY_STRING=" + ctx.query_string);
  env_strings.push_back("REQUEST_URI=" + ctx.req.getUri());

  std::string server_name = "localhost";
  std::string content_length = "";
  std::string content_type = "";

  // Single pass to collect headers and optimize lookups
  for (std::map<std::string, std::string>::const_iterator it_h = headers.begin(); it_h != headers.end(); ++it_h) {
    const std::string& key = it_h->first;
    const std::string& value = it_h->second;

    if (key == "host") {
      server_name = value;
      size_t colon = server_name.find(':');
      if (colon != std::string::npos) {
        server_name = server_name.substr(0, colon);
      }
      env_strings.push_back("HTTP_HOST=" + value);
    } else if (key == "content-length") {
      content_length = value;
    } else if (key == "content-type") {
      content_type = value;
    } else {
      env_strings.push_back("HTTP_" + Utils::toHeaderEnvKey(key) + "=" + value);
    }
  }

  env_strings.push_back("SERVER_NAME=" + server_name);
  env_strings.push_back("REMOTE_ADDR=" + ctx.client->getClientIp());
  env_strings.push_back("SERVER_PORT=" + Utils::toString(ctx.client->getServerPort()));

  if (content_length.empty() && !ctx.req.getBody().empty()) {
    content_length = Utils::toString(ctx.req.getBody().size());
  }

  if (!content_length.empty()) {
    env_strings.push_back("CONTENT_LENGTH=" + content_length);
  }
  if (!content_type.empty()) {
    env_strings.push_back("CONTENT_TYPE=" + content_type);
  }

  size_t system_env_size = 0;
  if (envp_) {
    while (envp_[system_env_size] != NULL) {
      system_env_size++;
    }
  }

  size_t static_constants_size = 4;
  size_t total_size = system_env_size + static_constants_size + env_strings.size();

  std::vector<char*> child_env;
  child_env.reserve(total_size + 1);

  if (envp_) {
    for (size_t i = 0; i < system_env_size; ++i) {
      child_env.push_back(envp_[i]);
    }
  }

  child_env.push_back(const_cast<char*>("GATEWAY_INTERFACE=CGI/1.1"));
  child_env.push_back(const_cast<char*>("SERVER_PROTOCOL=HTTP/1.1"));
  child_env.push_back(const_cast<char*>("SERVER_SOFTWARE=Webserv/1.0"));
  child_env.push_back(const_cast<char*>("REDIRECT_STATUS=200"));

  for (size_t i = 0; i < env_strings.size(); ++i) {
    child_env.push_back(const_cast<char*>(env_strings[i].c_str()));
  }
  child_env.push_back(NULL);

  return child_env;
}

void CgiExecutor::handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client, const LocationConfig& location) {
  CgiRequestContext ctx(req, res, client);

  parseUri(ctx);

  if (!resolveAndValidatePaths(ctx, location)) {
    return;
  }

  int in_pipe[2];
  int out_pipe[2];
  if (!createPipes(ctx, in_pipe, out_pipe)) {
    return;
  }

  std::vector<std::string> env_strings;
  std::vector<char*> child_env = buildChildEnv(ctx, env_strings);

  pid_t pid = fork();
  if (pid == -1) {
    Logger::error("CGI fork() failed: " + std::string(strerror(errno)));
    close(in_pipe[0]);
    close(in_pipe[1]);
    close(out_pipe[0]);
    close(out_pipe[1]);
    ctx.res.setStatusCode(500, "Internal Server Error");
    ctx.client->changeState(ClientHandler::WRITING_RESPONSE);
    return;
  }

  if (pid == 0) {
    executeChild(ctx, in_pipe, out_pipe, &child_env[0]);
  }

  setupParent(ctx, pid, in_pipe, out_pipe);
}
