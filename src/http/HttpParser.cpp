#include "HttpParser.hpp"

#include <cctype>
#include <cstdlib>

#include "Utils.hpp"

HttpParser::HttpParser(size_t max_body_size)
  : state_(STATE_REQUEST_LINE), clientMaxBodySize_(max_body_size), errorCode_(0), chunkSizeAccumulator_(0), bytesReadInChunk_(0) {
}

HttpParser::~HttpParser() {
}

void HttpParser::reset() {
  state_ = STATE_REQUEST_LINE;
  errorCode_ = 0;
  currentLine_.clear();
  chunkSizeAccumulator_ = 0;
  bytesReadInChunk_ = 0;
}

bool HttpParser::consume(std::vector<char>& raw_buffer, HttpRequest& req) {
  while (state_ != STATE_COMPLETE && state_ != STATE_ERROR) {
    bool progress = false;
    switch (state_) {
      case STATE_REQUEST_LINE:
        progress = handleRequestLine(raw_buffer, req);
        break;

      case STATE_HEADERS:
        progress = handleHeaders(raw_buffer, req);
        break;

      case STATE_BODY_IDENTITY:
        progress = handleBodyIdentity(raw_buffer, req);
        break;

      case STATE_CHUNK_HEADER:
        progress = handleChunkHeader(raw_buffer, req);
        break;

      case STATE_CHUNK_DATA:
        progress = handleChunkData(raw_buffer, req);
        break;

      case STATE_CHUNK_CRLF:
        progress = handleChunkCRLF(raw_buffer, req);
        break;

      default:
        break;
    }

    if (!progress) {
      break;
    }
  }

  return state_ == STATE_COMPLETE;
}

bool HttpParser::readLine(const std::vector<char>& buffer, size_t& pos, std::string& outLine) {
  size_t start = pos;
  while (pos < buffer.size() && buffer[pos] != '\n') {
    ++pos;
  }
  if (pos >= buffer.size()) {
    return false;
  }

  size_t len = pos - start;
  if (len > 0 && buffer[pos - 1] == '\r') {
    --len;
  }

  outLine.assign(&buffer[start], len);
  pos++;
  return true;
}

bool HttpParser::handleRequestLine(std::vector<char>& raw_buffer, HttpRequest& req) {
  size_t pos = 0;
  std::string line;
  if (!readLine(raw_buffer, pos, line)) {
    return false;
  }

  if (line.empty()) {
    raw_buffer.erase(raw_buffer.begin(), raw_buffer.begin() + pos);
    return true;
  }

  size_t methodEnd = line.find(' ');
  if (methodEnd == std::string::npos || methodEnd == 0) {
    state_ = STATE_ERROR;
    errorCode_ = 400;
    return false;
  }

  size_t uriEnd = line.find(' ', methodEnd + 1);
  if (uriEnd == std::string::npos || uriEnd == methodEnd + 1) {
    state_ = STATE_ERROR;
    errorCode_ = 400;
    return false;
  }

  std::string method = line.substr(0, methodEnd);
  std::string uri = line.substr(methodEnd + 1, uriEnd - methodEnd - 1);
  std::string version = line.substr(uriEnd + 1);

  if (version.find(' ') != std::string::npos || !Utils::startsWith(version, "HTTP/")) {
    state_ = STATE_ERROR;
    errorCode_ = 400;
    return false;
  }

  if (version != "HTTP/1.1" && version != "HTTP/1.0") {
    state_ = STATE_ERROR;
    errorCode_ = 505;
    return false;
  }

  if (uri.empty() || uri[0] != '/') {
    state_ = STATE_ERROR;
    errorCode_ = 400;
    return false;
  }

  size_t queryPos = uri.find('?');
  if (queryPos != std::string::npos) {
    req.setPath(uri.substr(0, queryPos));
    req.setQueryString(uri.substr(queryPos + 1));
  } else {
    req.setPath(uri);
    req.setQueryString("");
  }

  req.setMethod(method);
  req.setUri(uri);
  req.setVersion(version);

  state_ = STATE_HEADERS;
  raw_buffer.erase(raw_buffer.begin(), raw_buffer.begin() + pos);
  return true;
}

bool HttpParser::handleHeaders(std::vector<char>& raw_buffer, HttpRequest& req) {
  size_t pos = 0;
  std::string line;
  while (readLine(raw_buffer, pos, line)) {
    if (line.empty()) {
      raw_buffer.erase(raw_buffer.begin(), raw_buffer.begin() + pos);

      if (req.getHeaders().find("host") == req.getHeaders().end()) {
        state_ = STATE_ERROR;
        errorCode_ = 400;
        return false;
      }

      req.parseCookies();

      std::map<std::string, std::string>::const_iterator connIt = req.getHeaders().find("connection");
      if (connIt != req.getHeaders().end() && Utils::equalsIgnoreCase(connIt->second, "close")) {
        req.setKeepAlive(false);
      } else {
        req.setKeepAlive(true);
      }

      return resolveBodyType(req);
    }

    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos || colonPos == 0) {
      state_ = STATE_ERROR;
      errorCode_ = 400;
      return false;
    }

    std::string headerName = line.substr(0, colonPos);
    if (headerName.find(' ') != std::string::npos) {
      state_ = STATE_ERROR;
      errorCode_ = 400;
      return false;
    }
    headerName = Utils::toLowerCase(Utils::trim(headerName));
    std::string headerValue = Utils::trim(line.substr(colonPos + 1));
    req.addHeader(headerName, headerValue);
  }

  if (pos > 0) {
    raw_buffer.erase(raw_buffer.begin(), raw_buffer.begin() + pos);
  }
  return false;
}

bool HttpParser::resolveBodyType(HttpRequest& req) {
  const std::map<std::string, std::string>& headers = req.getHeaders();

  std::map<std::string, std::string>::const_iterator teIt = headers.find("transfer-encoding");
  if (teIt != headers.end() && Utils::equalsIgnoreCase(teIt->second, "chunked")) {
    state_ = STATE_CHUNK_HEADER;
    req.setChunked(true);
    chunkSizeAccumulator_ = 0;
    bytesReadInChunk_ = 0;
    return true;
  }

  std::map<std::string, std::string>::const_iterator clIt = headers.find("content-length");
  if (clIt != headers.end()) {
    const std::string& clStr = clIt->second;
    if (clStr.empty() || !std::isdigit(static_cast<unsigned char>(clStr[0]))) {
      state_ = STATE_ERROR;
      errorCode_ = 400;
      return false;
    }

    char* endPtr = NULL;
    unsigned long contentLength = std::strtoul(clStr.c_str(), &endPtr, 10);

    if (*endPtr != '\0') {
      state_ = STATE_ERROR;
      errorCode_ = 400;
      return false;
    }

    if (clientMaxBodySize_ > 0 && contentLength > clientMaxBodySize_) {
      state_ = STATE_ERROR;
      errorCode_ = 413;
      return false;
    }

    req.setContentLength(contentLength);
    if (contentLength > 0) {
      state_ = STATE_BODY_IDENTITY;
    } else {
      state_ = STATE_COMPLETE;
    }
    return true;
  }

  state_ = STATE_COMPLETE;
  return true;
}

bool HttpParser::handleBodyIdentity(std::vector<char>& raw_buffer, HttpRequest& req) {
  size_t needed = req.contentLength() - req.getBody().size();
  if (needed == 0) {
    state_ = STATE_COMPLETE;
    return true;
  }
  if (raw_buffer.empty()) {
    return false;
  }

  size_t to_consume = std::min(needed, raw_buffer.size());
  req.appendBody(&raw_buffer[0], to_consume);
  raw_buffer.erase(raw_buffer.begin(), raw_buffer.begin() + to_consume);

  if (req.getBody().size() == req.contentLength()) {
    state_ = STATE_COMPLETE;
    return true;
  }
  return false;
}

bool HttpParser::handleChunkHeader(std::vector<char>& raw_buffer, HttpRequest& req) {
  size_t pos = 0;
  std::string line;
  if (!readLine(raw_buffer, pos, line)) {
    return false;
  }

  if (line.empty() || !std::isxdigit(static_cast<unsigned char>(line[0]))) {
    state_ = STATE_ERROR;
    errorCode_ = 400;
    return false;
  }

  char* endPtr = NULL;
  unsigned long chunkSize = std::strtoul(line.c_str(), &endPtr, 16);

  if (endPtr == line.c_str()) {
    state_ = STATE_ERROR;
    errorCode_ = 400;
    return false;
  }

  while (*endPtr == ' ' || *endPtr == '\t') {
    endPtr++;
  }

  if (*endPtr != '\0' && *endPtr != ';') {
    state_ = STATE_ERROR;
    errorCode_ = 400;
    return false;
  }

  raw_buffer.erase(raw_buffer.begin(), raw_buffer.begin() + pos);

  if (chunkSize == 0) {
    chunkSizeAccumulator_ = 0;
    bytesReadInChunk_ = 0;
    state_ = STATE_CHUNK_CRLF;
    return true;
  }

  if (clientMaxBodySize_ > 0 && req.getBody().size() + chunkSize > clientMaxBodySize_) {
    state_ = STATE_ERROR;
    errorCode_ = 413;
    return false;
  }

  chunkSizeAccumulator_ = chunkSize;
  bytesReadInChunk_ = 0;
  state_ = STATE_CHUNK_DATA;
  return true;
}

bool HttpParser::handleChunkData(std::vector<char>& raw_buffer, HttpRequest& req) {
  size_t needed = chunkSizeAccumulator_ - bytesReadInChunk_;
  if (needed == 0) {
    state_ = STATE_CHUNK_CRLF;
    return true;
  }
  if (raw_buffer.empty()) {
    return false;
  }

  size_t to_consume = std::min(needed, raw_buffer.size());
  req.appendBody(&raw_buffer[0], to_consume);
  bytesReadInChunk_ += to_consume;
  raw_buffer.erase(raw_buffer.begin(), raw_buffer.begin() + to_consume);

  if (bytesReadInChunk_ == chunkSizeAccumulator_) {
    state_ = STATE_CHUNK_CRLF;
    return true;
  }
  return false;
}

bool HttpParser::handleChunkCRLF(std::vector<char>& raw_buffer, HttpRequest& req) {
  if (raw_buffer.size() < 2) {
    return false;
  }

  if (raw_buffer[0] != '\r' || raw_buffer[1] != '\n') {
    state_ = STATE_ERROR;
    errorCode_ = 400;
    return false;
  }

  raw_buffer.erase(raw_buffer.begin(), raw_buffer.begin() + 2);

  if (chunkSizeAccumulator_ == 0) {
    state_ = STATE_COMPLETE;
    return true;
  } else {
    state_ = STATE_CHUNK_HEADER;
    return true;
  }
  (void)req;
}

HttpParser::ParseState HttpParser::getState() const {
  return state_;
}

int HttpParser::getErrorCode() const {
  return errorCode_;
}
