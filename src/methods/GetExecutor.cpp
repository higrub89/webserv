#include "GetExecutor.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <vector>

#include "ClientHandler.hpp"
#include "ConfigStructures.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Utils.hpp"

namespace {
struct MimeEntry {
  const char* ext;
  const char* mime;
};

static const char* kDefaultMimeType = "application/octet-stream";

static const MimeEntry kMimeTable[] = {
  {".html", "text/html"},
  {".htm", "text/html"},
  {".css", "text/css"},
  {".js", "application/javascript"},
  {".json", "application/json"},
  {".txt", "text/plain"},
  {".png", "image/png"},
  {".jpg", "image/jpeg"},
  {".jpeg", "image/jpeg"},
  {".gif", "image/gif"},
  {".ico", "image/x-icon"},
  {".svg", "image/svg+xml"},
  {".pdf", "application/pdf"},
  {".mp4", "video/mp4"},
  {".mp3", "audio/mpeg"}};

static const size_t kMimeTableSize = sizeof(kMimeTable) / sizeof(kMimeTable[0]);

}  // namespace

GetExecutor::GetExecutor() {
}

GetExecutor::~GetExecutor() {
}

std::string GetExecutor::getMimeType(const std::string& ext) const {
  if (ext.empty()) {
    return kDefaultMimeType;
  }

  std::string lowerExt = Utils::toLowerCase(ext);
  for (size_t i = 0; i < kMimeTableSize; ++i) {
    if (lowerExt == kMimeTable[i].ext) {
      return kMimeTable[i].mime;
    }
  }

  return kDefaultMimeType;
}

void GetExecutor::handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client, const LocationConfig& location) {
  std::string root = location.root_dir;
  if (root.size() > 1 && root[root.size() - 1] == '/') {
    root.erase(root.size() - 1);
  }
  std::string filePath = root + req.getPath();

  if (access(filePath.c_str(), F_OK) != 0) {
    res.setStatusCode(404);
    res.setHeader("Content-Type", "text/html");
    res.setHeader("Connection", "close");
    res.setBody("<html><body><h1>404 Not Found</h1></body></html>");
    if (client) {
      client->changeState(ClientHandler::WRITING_RESPONSE);
    }
    return;
  }

  if (access(filePath.c_str(), R_OK) != 0) {
    res.setStatusCode(403);
    res.setHeader("Content-Type", "text/html");
    res.setHeader("Connection", "close");
    res.setBody("<html><body><h1>403 Forbidden</h1></body></html>");
    if (client) {
      client->changeState(ClientHandler::WRITING_RESPONSE);
    }
    return;
  }

  struct stat fileStat;
  if (stat(filePath.c_str(), &fileStat) != 0) {
    res.setStatusCode(500);
    res.setHeader("Content-Type", "text/html");
    res.setHeader("Connection", "close");
    res.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
    if (client) {
      client->changeState(ClientHandler::WRITING_RESPONSE);
    }
    return;
  }

  if (S_ISREG(fileStat.st_mode)) {
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
      res.setStatusCode(500);
      res.setHeader("Content-Type", "text/html");
      res.setHeader("Connection", "close");
      res.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
      if (client) {
        client->changeState(ClientHandler::WRITING_RESPONSE);
      }
      return;
    }

    std::vector<char> fileContent((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
    res.setStatusCode(200);
    res.setHeader("Content-Type", getMimeType(Utils::getExtension(filePath)));
    res.setBody(fileContent);
    if (client) {
      client->changeState(ClientHandler::WRITING_RESPONSE);
    }
    return;
  } else if (S_ISDIR(fileStat.st_mode)) {
    // TODO: Directory handling (index.html / autoindex)
  }

  client->changeState(ClientHandler::WRITING_RESPONSE);
}
