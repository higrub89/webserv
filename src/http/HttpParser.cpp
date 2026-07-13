// STUB — Responsabilidad de Alex. Implementación mínima para enlazar.
#include "HttpParser.hpp"

HttpParser::HttpParser(size_t max_body_size)
    : state_(STATE_REQUEST_LINE),
      clientMaxBodySize_(max_body_size),
      errorCode_(0),
      chunkSizeAccumulator_(0),
      bytesReadInChunk_(0) {}

HttpParser::~HttpParser() {}

void HttpParser::reset() {
  state_ = STATE_REQUEST_LINE;
  errorCode_ = 0;
  currentLine_.clear();
  chunkSizeAccumulator_ = 0;
  bytesReadInChunk_ = 0;
}

bool HttpParser::consume(std::vector<char>& raw_buffer, HttpRequest& req) {
  (void)req;
  // Stub: detectar fin de cabeceras HTTP (\r\n\r\n)
  std::string data(raw_buffer.begin(), raw_buffer.end());
  if (data.find("\r\n\r\n") != std::string::npos) {
    state_ = STATE_COMPLETE;
    return true;
  }
  return false;
}

HttpParser::ParseState HttpParser::getState() const { return state_; }
int HttpParser::getErrorCode() const { return errorCode_; }

bool HttpParser::parseRequestLine(HttpRequest& req) { (void)req; return false; }
bool HttpParser::parseHeaders(HttpRequest& req) { (void)req; return false; }
bool HttpParser::parseChunkHeader() { return false; }
void HttpParser::resolveBodyType(HttpRequest& req) { (void)req; }
