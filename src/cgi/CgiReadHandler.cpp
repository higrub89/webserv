#include "CgiReadHandler.hpp"

#include <signal.h>
#include <unistd.h>

#include <sstream>
#include <string>
#include <vector>

#include "Utils.hpp"

CgiReadHandler::CgiReadHandler(int stdout_fd, EpollManager& epoll_manager,
                               ClientHandler& client, pid_t cgi_pid)
  : AEventHandler(stdout_fd),
    epollManager_(epoll_manager),
    client_(client),
    cgiPid_(cgi_pid),
    headersParsed_(false) {
  readBuffer_.reserve(kMaxHeadersSize);
  epollManager_.addHandler(this, EPOLLIN | EPOLLRDHUP);
}

CgiReadHandler::~CgiReadHandler() {
  epollManager_.removeHandler(this);
  if (fd_ != -1) {
    close(fd_);
    fd_ = -1;
  }
  if (cgiPid_ > 0) {
    int status;
    pid_t res = waitpid(cgiPid_, &status, WNOHANG);
    if (res == 0) {
      kill(cgiPid_, SIGKILL);
      waitpid(cgiPid_, NULL, 0);
    }
  }
}

void CgiReadHandler::onReadReady() {
  char buffer[kBufferSize];
  ssize_t bytes_read = read(getFd(), buffer, sizeof(buffer));

  if (bytes_read > 0) {
    if (headersParsed_) {
      client_.appendToOutput(buffer, bytes_read);
    } else {
      readBuffer_.insert(readBuffer_.end(), buffer, buffer + bytes_read);
      // Safeguard: Limit CGI headers to prevent memory exhaustion (OOM)
      if (readBuffer_.size() > kMaxHeadersSize) {
        client_.handleCgiError();
        return;
      }
      size_t delimiter_len = 0;
      size_t header_end_idx = Utils::findHeadersEnd(readBuffer_, delimiter_len);
      if (header_end_idx != std::string::npos) {
        std::string header_str(readBuffer_.begin(),
                               readBuffer_.begin() + header_end_idx);
        const char* body_ptr = &readBuffer_[header_end_idx + delimiter_len];
        size_t body_size =
          readBuffer_.size() - (header_end_idx + delimiter_len);

        std::string status_line = "HTTP/1.1 200 OK\r\n";
        std::string extra_headers;
        std::istringstream stream(header_str);
        std::string line;
        while (std::getline(stream, line)) {
          if (!line.empty() && line[line.length() - 1] == '\r') {
            line.resize(line.length() - 1);
          }
          if (line.empty()) {
            continue;
          }
          if (Utils::startsWithStatus(line)) {
            std::string status_val = line.substr(7);
            size_t first_non_space = status_val.find_first_not_of(" \t");
            if (first_non_space != std::string::npos) {
              status_val = status_val.substr(first_non_space);
            }
            status_line = "HTTP/1.1 " + status_val + "\r\n";
          } else {
            extra_headers += line + "\r\n";
          }
        }
        std::string final_headers = status_line + extra_headers + "\r\n";
        client_.appendToOutput(final_headers.data(), final_headers.size());
        if (body_size > 0) {
          client_.appendToOutput(body_ptr, body_size);
        }
        headersParsed_ = true;
        readBuffer_.clear();
      }
    }
  } else if (bytes_read == 0) {  // CGI finished
    int status;
    pid_t reaped = waitpid(cgiPid_, &status, WNOHANG);
    if (reaped == 0) {
      // Child closed stdout but is still running. Kill it to prevent server
      // hang.
      kill(cgiPid_, SIGKILL);
      reaped = waitpid(cgiPid_, &status, 0);  // Now blocking wait is safe
    }

    bool success = false;
    if (reaped == cgiPid_) {
      cgiPid_ = -1;
      if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        success = true;
      }
    }

    if (!headersParsed_) {
      if (success) {
        std::string default_status = "HTTP/1.1 200 OK\r\n\r\n";
        client_.appendToOutput(default_status.data(), default_status.size());
        if (!readBuffer_.empty()) {
          client_.appendToOutput(&readBuffer_[0], readBuffer_.size());
        }
        headersParsed_ = true;
        readBuffer_.clear();
        client_.changeState(ClientHandler::WRITING_RESPONSE);
        client_.clearCgi();
      } else {
        client_.handleCgiError();
      }
    } else {
      if (success) {
        client_.changeState(ClientHandler::WRITING_RESPONSE);
        client_.clearCgi();
      } else {
        client_.handleCgiError();
      }
    }
  } else {  // bytes_read < 0: read error occurred.
    client_.handleCgiError();
  }
}

void CgiReadHandler::onWriteReady() {
  // * No-op for read handler
}

void CgiReadHandler::onDisconnect() {  // premature close / error
  client_.handleCgiError();
}
