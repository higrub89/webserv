#ifndef HTTPPARSER_HPP_
#define HTTPPARSER_HPP_

#include <string>
#include <vector>

#include "HttpRequest.hpp"

/**
 * @class HttpParser
 * @brief Incremental non-blocking HTTP parser using a Finite State Machine.
 *
 * Replaces: Request parsing logic in the old codebase.
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

  bool parseRequestLine(HttpRequest& req);
  bool parseHeaders(HttpRequest& req);
  bool parseChunkHeader();
  void resolveBodyType(HttpRequest& req);

  // Prevent copying (Orthodox Canonical Form requirement for non-copyable classes)
  HttpParser(const HttpParser& other);
  HttpParser& operator=(const HttpParser& other);

public:
  /**
   * @brief Construct a new HttpParser.
   * @param max_body_size Maximum body size limit.
   */
  HttpParser(size_t max_body_size);
  ~HttpParser();

  /**
   * @brief Resets parser states so the parser instance can be reused.
   */
  void reset();

  /**
   * @brief Consume incoming network bytes, parsing them incrementally.
   * @param raw_buffer Buffer containing received network bytes. Bytes parsed are erased from it.
   * @param req Target HttpRequest object to populate.
   * @return true If parsing has successfully completed.
   * @return false If more bytes are required, or if a parse error occurred.
   */
  bool consume(std::vector<char>& raw_buffer, HttpRequest& req);

  ParseState getState() const;
  int getErrorCode() const;
};

#endif  // HTTPPARSER_HPP_
