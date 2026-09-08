#include "HttpError.hpp"

#include <fstream>
#include <sstream>

#include "Logger.hpp"
#include "Utils.hpp"

namespace HttpError {

void populate(HttpResponse& res, int errorCode, const ServerConfig& server) {
  res.reset();
  res.setStatusCode(errorCode);
  res.setHeader("Content-Type", "text/html");
  res.setHeader("Connection", "close");

  std::map<int, std::string>::const_iterator it = server.error_pages.find(errorCode);
  if (it != server.error_pages.end()) {
    const std::string& errorPagePath = it->second;
    std::ifstream errorPageFile(errorPagePath.c_str(), std::ios::binary);
    if (errorPageFile.is_open()) {
      std::stringstream buffer;
      buffer << errorPageFile.rdbuf();
      res.setBody(buffer.str());
      return;
    } else {
      Logger::info("Custom error page not found or unreadable: " + errorPagePath);
    }
  }

  std::string phrase = HttpResponse::reasonPhrase(errorCode);
  std::string defaultHtml =
    "<html>\r\n"
    "<head><title>" +
    Utils::toString(errorCode) + " " + phrase +
    "</title></head>\r\n"
    "<body>\r\n"
    "<center><h1>" +
    Utils::toString(errorCode) + " " + phrase +
    "</h1></center>\r\n"
    "<hr><center>webserv/1.0</center>\r\n"
    "</body>\r\n"
    "</html>\r\n";
  res.setBody(defaultHtml);
}

void populateDefault(HttpResponse& res, int errorCode) {
  res.reset();
  res.setStatusCode(errorCode);
  res.setHeader("Content-Type", "text/html");
  res.setHeader("Connection", "close");

  std::string phrase = HttpResponse::reasonPhrase(errorCode);
  std::string defaultHtml =
    "<html>\r\n"
    "<head><title>" +
    Utils::toString(errorCode) + " " + phrase +
    "</title></head>\r\n"
    "<body>\r\n"
    "<center><h1>" +
    Utils::toString(errorCode) + " " + phrase +
    "</h1></center>\r\n"
    "<hr><center>webserv/1.0</center>\r\n"
    "</body>\r\n"
    "</html>\r\n";
  res.setBody(defaultHtml);
}

}  // namespace HttpError
