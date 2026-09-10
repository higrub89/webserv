#include <csignal>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "CgiExecutor.hpp"
#include "ConfigParser.hpp"
#include "EpollManager.hpp"
#include "GetExecutor.hpp"
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
  Logger::info("WebServer starting...");
  Logger::debug("Debug mode is enabled.");

  signal(SIGINT, signalHandler);   // Ctrl+C
  signal(SIGTERM, signalHandler);  // kill
  signal(SIGPIPE, SIG_IGN);        // Evitar crash por broken pipe

  try {
    // ── Parsear configuración ───────────────────────────────────────
    std::string configPath = "config/default.conf";
    if (argc == 2)
      configPath = argv[1];
    ConfigParser parser;
    ConfigMap configMap = parser.parse(configPath);

    // ── Crear componentes ───────────────────────────────────────────
    Router router(configMap, envp);
    router.registerMethodExecutor("GET", new GetExecutor());
    router.registerMethodExecutor("CGI", new CgiExecutor(envp));

    EpollManager epoll;
    epoll.init();

    // ── Crear ServerHandler por cada grupo ip:port ──────────────────
    // servers[0] es el default server del grupo (decisión #6).
    // NOTA(Ruben): ServerHandler aún bindea solo por puerto; el bind por
    // IP del grupo (decisión #17) queda pendiente en su constructor.
    for (ConfigMap::const_iterator it = configMap.begin(); it != configMap.end(); ++it) {
      ServerHandler* server = new ServerHandler(it->second.port, it->second.servers[0], epoll, router);
      server->setup();
      epoll.addHandler(server, EPOLLIN);
    }

    // ── Arrancar event-loop ─────────────────────────────────────────
    epoll.run();
    Logger::info("WebServer shutdown complete");
  } catch (const std::exception& e) {
    // Decisión #5: config inválida → mensaje claro a stderr y no arrancar.
    // Logger::error escribe a std::cerr.
    Logger::error(std::string("Fatal error: ") + e.what());
    return 1;
  }
  return 0;
}
