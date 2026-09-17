#ifndef CGISTRUCTURES_HPP_
#define CGISTRUCTURES_HPP_

#include <string>

#include "ClientHandler.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

/**
 * @struct CgiRequestContext
 * @brief Context and resolved paths for a single CGI execution lifecycle.
 */
struct CgiRequestContext {
  const HttpRequest& req;
  HttpResponse& res;
  ClientHandler* client;

  std::string script_name;
  std::string script_path;
  std::string query_string;
  std::string interpreter_path;

  CgiRequestContext(const HttpRequest& r, HttpResponse& s, ClientHandler* c);
};

#endif  // CGISTRUCTURES_HPP_
