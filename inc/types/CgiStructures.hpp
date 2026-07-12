#ifndef CGISTRUCTURES_HPP_
#define CGISTRUCTURES_HPP_

#include "ClientHandler.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

/**
 * @struct CgiRequestContext
 * @brief Holds the context for a CGI request
 */
struct CgiRequestContext {
  const HttpRequest& req;
  HttpResponse& res;
  ClientHandler* client;

  std::string script_name;
  std::string script_path;
  std::string query_string;
  std::string interpreter_path;

  CgiRequestContext(const HttpRequest& r, HttpResponse& s, ClientHandler* c)
    : req(r), res(s), client(c) {}
};

#endif  // CGISTRUCTURES_HPP_
