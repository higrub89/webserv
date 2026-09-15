*This project has been created as part of the 42 curriculum by rhiguita.*

# Webserv — Non-Blocking HTTP/1.1 Web Server in C++98

## Description

**Webserv** is an asynchronous, event-driven HTTP/1.1 web server implemented in C++98. Inspired by NGINX, it provides a high-performance network service handling multiple concurrent connections using a single-threaded Linux `epoll` event loop without blocking I/O calls.

### Core Architecture & Features

- **Single-Threaded Asynchronous Event Loop (`EpollManager`)**:
  - Centralized multiplexer using Linux `epoll` with `EPOLLIN`, `EPOLLOUT`, and `EPOLLRDHUP`.
  - Non-blocking I/O across listening sockets, client TCP sockets, and asynchronous CGI unidirectional pipes.
  - Strict single-read/single-write call discipline per event dispatch.
  - Automatic connection idle timeout sweeps (60-second inactivity detection).

- **Robust HTTP/1.1 & HTTP/1.0 Parser (`HttpParser`)**:
  - Incremental Finite State Machine (FSM) reading raw byte streams without payload corruption.
  - Full support for `Transfer-Encoding: chunked` (dynamic de-chunking) and `Content-Length` identity encoding.
  - HTTP/1.1 mandatory `Host` header validation and HTTP/1.0 backward compatibility (`Connection: close` default).
  - Pipelining and persistent Keep-Alive connections with preserved buffer state.

- **Routing & Virtual Hosts (`Router`, `GetExecutor`, `PostExecutor`, `DeleteExecutor`)**:
  - Multi-port listening and virtual host resolution via `Host` header and `server_name`.
  - Standard HTTP methods:
    - **GET**: Static file serving, custom MIME type resolution, automatic directory indexing (`autoindex on/off`), and default index resolution.
    - **POST**: Raw binary uploads and `multipart/form-data` parsing when `upload_enable on` is configured; standard payload handling.
    - **DELETE**: File removal with permission validation and directory protection.
  - HTTP Redirections (`302 Found`).
  - Configurable `client_max_body_size` per server and per location route (`413 Payload Too Large`).
  - Customizable error pages with fallback HTML generation.

- **Asynchronous CGI Subsystem (`CgiExecutor`, `CgiReadHandler`, `CgiWriteHandler`)**:
  - Execution of CGI scripts (e.g. Python, Shell, compiled binaries) based on configured file extensions.
  - Compliance with RFC 3875: standard environment variables (`REQUEST_METHOD`, `SCRIPT_FILENAME`, `PATH_INFO`, `QUERY_STRING`, `SERVER_PROTOCOL`, `HTTP_*`).
  - Working directory (`chdir`) isolation to the script parent directory.
  - Concurrent non-blocking pipe writing (request body feeding) and reading (response streaming) without deadlocks or buffer truncation.

- **Bonus Modules**:
  - **Session & Cookie Manager (`SessionManager`)**: In-memory session store generating 32-character secure alphanumeric tokens (`session_id`) with visit counting, client IP tracking, automatic expiration sweeps, and HTTP `Set-Cookie` integration.
  - **Multi-CGI Support**: Independent handler mappings per file extension.

---

## Instructions

### Compilation

Build the executable using GNU Make with strict flags (`-Wall -Wextra -Werror -std=c++98 -pedantic`):

```bash
make        # Compiles the webserv binary
make clean  # Removes object and dependency files
make fclean # Removes objects and webserv executable
make re     # Recompiles from scratch
```

### Execution

Run the server by passing a configuration file path (or omit to use `config/default.conf`):

```bash
./webserv [path/to/configuration.conf]
```

Example:
```bash
./webserv config/default.conf
```

To run with the 42 official test suite configuration:
```bash
./webserv config/tester.conf
```

### Configuration File Syntax

Configuration files follow an NGINX-style block hierarchy:

```nginx
server {
    listen 8080;
    server_name localhost;
    root ./www;
    client_max_body_size 10m;

    error_page 404 /errors/404.html;
    error_page 500 502 /errors/50x.html;

    location / {
        methods GET;
        index index.html;
        autoindex off;
    }

    location /upload {
        methods GET POST DELETE;
        upload_enable on;
        upload_store ./www/uploads;
    }

    location /cgi-bin {
        root ./www/cgi-bin;
        methods GET POST;
        cgi .py /usr/bin/python3;
        cgi .sh /bin/sh;
    }

    location /old {
        redirect /;
    }
}
```

### Verification & Testing

1. **Static Content & Custom Error Pages**:
   ```bash
   curl -i http://localhost:8080/
   curl -i http://localhost:8080/nonexistent
   ```

2. **File Upload & Deletion**:
   ```bash
   # Upload a file
   curl -i -X POST http://localhost:8080/upload/data.txt -d "Sample text"

   # Retrieve uploaded file
   curl -i http://localhost:8080/upload/data.txt

   # Delete file
   curl -i -X DELETE http://localhost:8080/upload/data.txt
   ```

3. **CGI Execution**:
   ```bash
   curl -i http://localhost:8080/cgi-bin/hello.py
   ```

4. **Official 42 Tester Suite**:
   ```bash
   ./tester/tester http://localhost:8000
   ```

5. **Memory Leak Verification (Valgrind)**:
   ```bash
   valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./webserv config/default.conf
   ```

---

## Resources

### Documentation & Standards
- [RFC 2616 — Hypertext Transfer Protocol -- HTTP/1.1](https://datatracker.ietf.org/doc/html/rfc2616)
- [RFC 3875 — The Common Gateway Interface (CGI) Version 1.1](https://datatracker.ietf.org/doc/html/rfc3875)
- [Linux Programmer's Manual: epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [Linux Programmer's Manual: fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html)
- [Linux Programmer's Manual: socket(2)](https://man7.org/linux/man-pages/man2/socket.2.html)

### AI Usage Disclosure
In accordance with Chapter III ("AI Instructions") of the 42 Common Core curriculum:
- **AI Tool**: Antigravity / Gemini DeepMind AI Coding Assistant.
- **Scope of AI Assistance**: Assisted with system architecture auditing, reproducing subtle non-blocking pipe edge-cases (Linux `EPOLLHUP`/`EAGAIN` nuances during high-volume transfers), identifying memory move bottlenecks in large response serialization, and reviewing POSIX/C++98 conformance.
- **Verification & Ownership**: All logic, concurrency flows, and data structures were audited, understood, and validated using Valgrind, custom test requests, and the 42 official tester suite.
