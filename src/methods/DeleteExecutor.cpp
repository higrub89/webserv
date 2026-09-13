#include "DeleteExecutor.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include "ClientHandler.hpp"
#include "ConfigStructures.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

DeleteExecutor::DeleteExecutor() {
}

DeleteExecutor::~DeleteExecutor() {
}

void DeleteExecutor::handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client, const LocationConfig& location) {
  (void)client;
  std::string root = location.root_dir;
  if (root.size() > 1 && root[root.size() - 1] == '/') {
    root.erase(root.size() - 1);
  }

  std::string relPath = req.getPath();
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
  std::string filePath = root + relPath;
  if (location.upload_enable && !location.upload_store.empty()) {
    std::string uploadPath = location.upload_store + relPath;
    if (access(uploadPath.c_str(), F_OK) == 0) {
      filePath = uploadPath;
    }
  }

  if (access(filePath.c_str(), F_OK) != 0) {
    res.setStatusCode(404, "Not Found");
    return;
  }

  struct stat st;
  if (stat(filePath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
    // Cannot DELETE a directory directly via standard file DELETE
    res.setStatusCode(403, "Forbidden");
    return;
  }

  if (access(filePath.c_str(), W_OK) != 0) {
    res.setStatusCode(403, "Forbidden");
    return;
  }

  if (unlink(filePath.c_str()) == 0) {
    res.setStatusCode(200, "OK");
    res.setHeader("Content-Type", "text/plain");
    res.setBody("File successfully deleted.\r\n");
  } else {
    res.setStatusCode(500, "Internal Server Error");
  }
}
