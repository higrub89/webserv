#include "ConfigStructures.hpp"

LocationConfig::LocationConfig()
  : autoindex(false), client_max_body_size(0), upload_enable(false) {
}

ServerConfig::ServerConfig() : client_max_body_size(1048576) {
}

ServerGroup::ServerGroup() : port(0) {
}
