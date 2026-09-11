#ifndef CONFIGSTRUCTURES_HPP_
#define CONFIGSTRUCTURES_HPP_

#include <map>
#include <string>
#include <vector>

/**
 * @struct LocationConfig
 * @brief Configuration directives for a specific URI location route.
 */
struct LocationConfig {
  std::string route_path;
  std::vector<std::string> allowed_methods;
  std::string root_dir;
  bool autoindex;
  std::string index_file;
  size_t client_max_body_size;  // 0 = inherit from ServerConfig
  std::map<std::string, std::string> cgi_handlers;
  std::string return_redirect;
  bool upload_enable;
  std::string upload_store;

  LocationConfig()
    : autoindex(false),
      client_max_body_size(0),
      upload_enable(false) {}
};

/**
 * @struct ServerConfig
 * @brief Configuration directives defining a Virtual Server block.
 */
struct ServerConfig {
  std::vector<std::string> server_names;
  std::string root_dir;
  size_t client_max_body_size;
  std::map<int, std::string> error_pages;
  std::map<std::string, LocationConfig> locations;

  ServerConfig() : client_max_body_size(1048576) {}
};

/**
 * @struct ServerGroup
 * @brief Groups Virtual Servers sharing the same physical IP and listening port.
 */
struct ServerGroup {
  std::string ip;
  int port;
  std::vector<ServerConfig> servers;  // servers[0] is default_server

  ServerGroup() : port(0) {}
};

typedef std::map<std::string, ServerGroup> ConfigMap;

#endif  // CONFIGSTRUCTURES_HPP_
