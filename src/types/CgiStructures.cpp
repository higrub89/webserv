#include "CgiStructures.hpp"

CgiRequestContext::CgiRequestContext(const HttpRequest& r, HttpResponse& s, ClientHandler* c)
  : req(r), res(s), client(c) {
}
