#ifndef CONFIGSTRUCTURES_HPP_
#define CONFIGSTRUCTURES_HPP_

#include <map>
#include <string>
#include <vector>

/**
 * @struct LocationConfig
 * @brief Configuration rules for a specific URI route block.
 *
 * Replaces: Location-related configuration structures in Types.hpp
 */
struct LocationConfig {
  std::vector<std::string> allowed_methods;
  std::string root_dir;
  bool autoindex;
  std::string index_file;
  size_t client_max_body_size;  // 0 = inherit from ServerConfig

  // Maps CGI extensions (e.g., ".py", ".php") to their respective executable
  // binaries. Resolves: Multi-CGI parallel support bonus requirement.
  std::map<std::string, std::string> cgi_handlers;

  std::string return_redirect;
  bool upload_enable;
  std::string upload_store;  // Path where uploaded files should be stored.
};

/**
 * @struct ServerConfig
 * @brief Configuration rules for a specific virtual server.
 *
 * Replaces: ServerConfig structures in Types.hpp
 */
struct ServerConfig {
  std::vector<std::string> server_names;
  std::string root_dir;
  size_t client_max_body_size;
  std::map<int, std::string> error_pages;
  std::map<std::string, LocationConfig> locations;
};

/**
 * @struct ServerGroup
 * @brief Groups servers sharing the same physical bind port.
 *
 * Replaces: None (Introduced to prevent EADDRINUSE by allowing one socket bind
 * per port)
 */
struct ServerGroup {
  std::string ip;
  int port;
  std::vector<ServerConfig> servers;  // Index 0 represents the default_server for this port.
};

// Type definition mapping "IP:Port" or "Port" string to its ServerGroup.
typedef std::map<std::string, ServerGroup> ConfigMap;

#endif  // CONFIGSTRUCTURES_HPP_
