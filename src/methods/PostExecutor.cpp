#include "PostExecutor.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <sstream>

#include "ClientHandler.hpp"
#include "ConfigStructures.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Utils.hpp"

PostExecutor::PostExecutor() {
}

PostExecutor::~PostExecutor() {
}

std::string PostExecutor::extractBoundary(const std::string& contentType) const {
  std::string lowerType = Utils::toLowerCase(contentType);
  size_t boundaryPos = lowerType.find("boundary=");
  if (boundaryPos == std::string::npos) {
    return "";
  }
  std::string boundary = contentType.substr(boundaryPos + 9);
  if (!boundary.empty() && boundary[0] == '"') {
    boundary = boundary.substr(1);
    size_t quotePos = boundary.find('"');
    if (quotePos != std::string::npos) {
      boundary = boundary.substr(0, quotePos);
    }
  } else {
    size_t semi = boundary.find(';');
    if (semi != std::string::npos) {
      boundary = boundary.substr(0, semi);
    }
    boundary = Utils::trim(boundary);
  }
  return boundary;
}

bool PostExecutor::parseMultipart(const std::vector<char>& body, const std::string& boundary, std::string& filename, std::vector<char>& fileContent) const {
  std::string bodyStr(body.begin(), body.end());
  std::string delimiter = "--" + boundary;
  size_t startPos = bodyStr.find(delimiter);
  if (startPos == std::string::npos) {
    return false;
  }
  startPos += delimiter.length();
  if (startPos < bodyStr.length() && bodyStr.substr(startPos, 2) == "\r\n") {
    startPos += 2;
  } else if (startPos < bodyStr.length() && bodyStr[startPos] == '\n') {
    startPos += 1;
  }

  size_t headerEnd = bodyStr.find("\r\n\r\n", startPos);
  size_t headerEndLen = 4;
  if (headerEnd == std::string::npos) {
    headerEnd = bodyStr.find("\n\n", startPos);
    headerEndLen = 2;
  }
  if (headerEnd == std::string::npos) {
    return false;
  }

  std::string partHeaders = bodyStr.substr(startPos, headerEnd - startPos);
  size_t fnPos = partHeaders.find("filename=\"");
  if (fnPos != std::string::npos) {
    size_t fnStart = fnPos + 10;
    size_t fnEnd = partHeaders.find("\"", fnStart);
    if (fnEnd != std::string::npos) {
      filename = partHeaders.substr(fnStart, fnEnd - fnStart);
    }
  }

  size_t dataStart = headerEnd + headerEndLen;
  std::string nextDelimiter = "\r\n--" + boundary;
  size_t dataEnd = bodyStr.find(nextDelimiter, dataStart);
  if (dataEnd == std::string::npos) {
    nextDelimiter = "\n--" + boundary;
    dataEnd = bodyStr.find(nextDelimiter, dataStart);
  }
  if (dataEnd == std::string::npos) {
    dataEnd = body.size();
  }

  fileContent.assign(body.begin() + dataStart, body.begin() + dataEnd);
  return true;
}

void PostExecutor::handleUpload(const HttpRequest& req, HttpResponse& res, const LocationConfig& location) {
  std::string uploadDir = location.upload_store;
  if (uploadDir.empty()) {
    res.setStatusCode(500);
    return;
  }

  struct stat st;
  if (stat(uploadDir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
    res.setStatusCode(500);
    return;
  }
  if (access(uploadDir.c_str(), W_OK) != 0) {
    res.setStatusCode(403);
    return;
  }

  const std::map<std::string, std::string>& headers = req.getHeaders();
  std::string contentType = "";
  std::map<std::string, std::string>::const_iterator ctIt = headers.find("content-type");
  if (ctIt != headers.end()) {
    contentType = ctIt->second;
  }

  std::string filename = "";
  std::vector<char> fileContent;
  std::string boundary = extractBoundary(contentType);

  if (!boundary.empty() && parseMultipart(req.getBody(), boundary, filename, fileContent)) {
    // Multipart parsed successfully
  } else {
    // Raw upload: extract filename from URI
    std::string relPath = req.getPath();
    std::string locPrefix = location.path;
    if (!locPrefix.empty() && relPath.compare(0, locPrefix.size(), locPrefix) == 0) {
      relPath = relPath.substr(locPrefix.size());
    }
    while (!relPath.empty() && relPath[0] == '/') {
      relPath.erase(0, 1);
    }
    filename = relPath;
    fileContent = req.getBody();
  }

  // Sanitize filename to avoid path traversal
  size_t slashPos = filename.find_last_of("/\\");
  if (slashPos != std::string::npos) {
    filename = filename.substr(slashPos + 1);
  }
  if (filename.empty()) {
    std::ostringstream ss;
    ss << "upload_" << std::time(NULL);
    filename = ss.str();
  }

  std::string targetPath = uploadDir;
  if (targetPath[targetPath.length() - 1] != '/') {
    targetPath += "/";
  }
  targetPath += filename;

  std::ofstream outfile(targetPath.c_str(), std::ios::binary | std::ios::trunc);
  if (!outfile.is_open()) {
    res.setStatusCode(500);
    return;
  }
  if (!fileContent.empty()) {
    outfile.write(&fileContent[0], fileContent.size());
  }
  outfile.close();

  res.setStatusCode(201, "Created");
  res.setHeader("Content-Type", "text/plain");
  std::string locHeader = req.getPath();
  if (locHeader.find(filename) == std::string::npos) {
    if (locHeader.empty() || locHeader[locHeader.length() - 1] != '/') {
      locHeader += "/";
    }
    locHeader += filename;
  }
  res.setHeader("Location", locHeader);
  res.setBody("File uploaded successfully.\r\n");
}

void PostExecutor::handleStandardPost(const HttpRequest& req, HttpResponse& res) {
  (void)req;
  res.setStatusCode(200, "OK");
  res.setHeader("Content-Type", "text/plain");
  res.setBody("POST request processed successfully.\r\n");
}

void PostExecutor::handle(const HttpRequest& req, HttpResponse& res, ClientHandler* client, const LocationConfig& location) {
  (void)client;
  if (location.upload_enable) {
    handleUpload(req, res, location);
  } else {
    handleStandardPost(req, res);
  }
}
