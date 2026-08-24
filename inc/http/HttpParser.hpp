#ifndef HTTPPARSER_HPP_
#define HTTPPARSER_HPP_

#include <string>
#include <vector>

#include "HttpRequest.hpp"

/**
 * @class HttpParser
 * @brief Incremental non-blocking HTTP/1.1 request parser driven by a Finite State Machine (FSM).
 *
 * Consumes raw byte chunks from the socket buffer, parses the Request-Line and Headers,
 * handles Content-Length and Transfer-Encoding: chunked bodies, and enforces body limits.
 */
class HttpParser {
public:
  enum ParseState {
    STATE_REQUEST_LINE,
    STATE_HEADERS,
    STATE_BODY_IDENTITY,
    STATE_BODY_CHUNKED,
    STATE_CHUNK_HEADER,
    STATE_CHUNK_DATA,
    STATE_CHUNK_CRLF,
    STATE_COMPLETE,
    STATE_ERROR
  };

private:
  ParseState state_;
  size_t clientMaxBodySize_;
  int errorCode_;

  std::string currentLine_;
  size_t chunkSizeAccumulator_;
  size_t bytesReadInChunk_;

  bool readLine(const std::vector<char>& buffer, size_t& pos, std::string& outLine);

  bool resolveBodyType(HttpRequest& req);

  bool handleRequestLine(std::vector<char>& raw_buffer, HttpRequest& req);
  bool handleHeaders(std::vector<char>& raw_buffer, HttpRequest& req);
  bool handleBodyIdentity(std::vector<char>& raw_buffer, HttpRequest& req);

public:
  HttpParser(size_t max_body_size);
  ~HttpParser();

  void reset();

  /**
   * @brief Consumes incoming network bytes incrementally, advancing the state machine.
   * @param raw_buffer Buffer containing received network bytes. Consumed bytes are erased from the front.
   * @param req Target HttpRequest object to populate.
   * @return true if request parsing is complete, false if more bytes needed or error occurred.
   */
  bool consume(std::vector<char>& raw_buffer, HttpRequest& req);

  ParseState getState() const;
  int getErrorCode() const;
};

#endif  // HTTPPARSER_HPP_
