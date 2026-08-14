#include <csignal>
#include <iostream>
#include <stdexcept>

#include "EpollManager.hpp"
#include "Logger.hpp"
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
  Logger::info("WebServer starting...");
  Logger::debug("Debug mode is enabled.");

  signal(SIGINT, signalHandler);   // Ctrl+C
  signal(SIGTERM, signalHandler);  // kill
  signal(SIGPIPE, SIG_IGN);        // Evitar crash por broken pipe

  try {
    // ── Config stub (TODO: Alex — ConfigParser) ─────────────────────
    ServerConfig defaultConfig;
    defaultConfig.client_max_body_size = 1048576;
    defaultConfig.server_names.push_back("localhost");

    ServerGroup serverGroup;
    serverGroup.ip = "0.0.0.0";
    serverGroup.port = 8080;
    serverGroup.servers.push_back(defaultConfig);

    ConfigMap configMap;
    configMap["8080"] = serverGroup;

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
    epoll.run();
    Logger::info("WebServer shutdown complete");
  } catch (const std::exception& e) {
    // TODO
    // Checkear si es necesario loggear algo, o hacer algo especifico
    return 1;
  }
  (void)argc;
  (void)argv;
  (void)envp;
  return 0;
}
