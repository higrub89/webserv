#include <csignal>
#include <iostream>
#include <stdexcept>

#include "EpollManager.hpp"
#include "Router.hpp"
#include "ServerHandler.hpp"
#include "types/ConfigStructures.hpp"

volatile sig_atomic_t g_running = 1;

static void signalHandler(int sig) {
  (void)sig;
  g_running = 0;
}

int main(int argc, char* argv[], char* envp[]) {
  if (argc > 2) {
    std::cerr << "Usage: ./webserv [config_file]" << std::endl;
    return 1;
  }
  (void)argv;

  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGPIPE, SIG_IGN);

  try {
    // ── Config stub (TODO: Alex — ConfigParser) ─────────────────────
    ServerConfig defaultConfig;
    defaultConfig.client_max_body_size = 1048576;
    defaultConfig.server_names.push_back("localhost");

    VirtualHostGroup vhg;
    vhg.ip = "0.0.0.0";
    vhg.port = 8080;
    vhg.servers.push_back(defaultConfig);

    ConfigMap configMap;
    configMap["8080"] = vhg;

    // ── Crear componentes ───────────────────────────────────────────
    Router router(configMap, envp);
    EpollManager epoll;
    epoll.init();

    // ── Crear ServerHandler por cada puerto ──────────────────────────
    ServerHandler* server =
        new ServerHandler(8080, defaultConfig, epoll, router);
    server->setup();
    epoll.addHandler(server, EPOLLIN);

    // ── Arrancar event-loop ─────────────────────────────────────────
    std::cout << "[INFO]  WebServer starting..." << std::endl;
    epoll.run();
    std::cout << "[INFO]  WebServer shutdown complete" << std::endl;

  } catch (const std::exception& e) {
    std::cerr << "Fatal: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
