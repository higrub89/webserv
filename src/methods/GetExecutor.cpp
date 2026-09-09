#include "GetExecutor.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
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

void GetExecutor::serveFile(const std::string& path, HttpResponse& res) const {
  std::ifstream file(path.c_str(), std::ios::binary);
  if (!file.is_open()) {
    res.setStatusCode(500);
    return;
  }

  std::vector<char> fileContent((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
  res.setStatusCode(200);
  res.setHeader("Content-Type", getMimeType(Utils::getExtension(path)));
  res.setBody(fileContent);
}

void GetExecutor::generateAutoindex(const std::string& dirPath, const std::string& uriPath, HttpResponse& res) const {
  DIR* dir = opendir(dirPath.c_str());
  if (!dir) {
    res.setStatusCode(403);
    return;
  }

  std::string body = "<html><body><h1>Index of " + uriPath + "</h1><ul>";
  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    std::string name = entry->d_name;
    if (name == ".") {
      continue;
    }
    std::string href = uriPath;
    if (href.empty() || href[href.size() - 1] != '/') {
      href += "/";
    }
    href += name;
    body += "<li><a href=\"" + href + "\">" + name + "</a></li>";
  }
  closedir(dir);
  body += "</ul></body></html>";

  res.setStatusCode(200);
  res.setHeader("Content-Type", "text/html");
  res.setBody(body);
}

void GetExecutor::handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client, const LocationConfig& location) {
  (void)client;
  std::string root = location.root_dir;
  if (root.size() > 1 && root[root.size() - 1] == '/') {
    root.erase(root.size() - 1);
  }
  std::string filePath = root + req.getPath();

  if (access(filePath.c_str(), F_OK) != 0) {
    res.setStatusCode(404);
    return;
  }

  if (access(filePath.c_str(), R_OK) != 0) {
    res.setStatusCode(403);
    return;
  }

  struct stat fileStat;
  if (stat(filePath.c_str(), &fileStat) != 0) {
    res.setStatusCode(500);
    return;
  }

  if (S_ISREG(fileStat.st_mode)) {
    serveFile(filePath, res);
    return;
  }

  if (S_ISDIR(fileStat.st_mode)) {
    if (req.getPath().empty() || req.getPath()[req.getPath().size() - 1] != '/') {
      res.setStatusCode(301);
      res.setHeader("Location", req.getPath() + "/");
      return;
    }

    if (!location.index_file.empty()) {
      std::string indexPath = filePath;
      if (indexPath.empty() || indexPath[indexPath.size() - 1] != '/') {
        indexPath += "/";
      }
      indexPath += location.index_file;

      if (access(indexPath.c_str(), F_OK) == 0) {
        if (access(indexPath.c_str(), R_OK) != 0) {
          res.setStatusCode(403);
          return;
        }
        serveFile(indexPath, res);
        return;
      }
    }

    if (location.autoindex) {
      generateAutoindex(filePath, req.getPath(), res);
      return;
    }
  }

  res.setStatusCode(403);
}
