#include <csignal>
#include <iostream>

#include "EpollManager.hpp"
#include "Logger.hpp"

volatile sig_atomic_t g_running = 1;

static void signalHandler(int sig) {
  (void)sig;
  g_running = 0;
}

int main(int argc, char* argv[], char* envp[]) {
  if (argc > 2) {
    std::cerr << "Usage: ./webserver [config_file]" << std::endl;
    return 1;
  }
  Logger::info("WebServer starting...");
  Logger::debug("Debug mode is enabled.");
  signal(SIGINT, signalHandler);   // Ctrl+C
  signal(SIGTERM, signalHandler);  // kill
  signal(SIGPIPE, SIG_IGN);        // Evitar crash por broken pipe

  try {
    // TODO:
    // - Parsear configuración (argv[1]) y crear ServerConfig por cada bloque
    // server{}
    // - Crear EpollManager
    // - Crear ServerHandler por cada ServerConfig y registrarlo en EpollManager
  } catch (const std::exception& e) {
    // TODO
    // Checkear si es necesario loggear algo, o hacer algo especifico
    return 1;
  }
  (void)argc;
  (void)argv;
  (void)envp;
}

/*
#include <csignal>
#include <iostream>

// Puntero global para que el signal handler pueda detener el loop
static PollManager* g_manager = NULL;

static void signalHandler(int sig) {
  (void)sig;
  if (g_manager)
    g_manager->stop();
}

int main(int argc, char* argv[]) {
  // ── Validar argumentos ──────────────────────────────────────────────
  if (argc > 2) {
    std::cerr << "Usage: ./webserver [config_file]" << std::endl;
    return 1;
  }
  (void)argv;  // TODO: pasar a ConfigParser de Alex

  // ── Configurar signal handlers ──────────────────────────────────────
  signal(SIGINT, signalHandler);   // Ctrl+C
  signal(SIGTERM, signalHandler);  // kill
  signal(SIGPIPE, SIG_IGN);        // Evitar crash por broken pipe

  try {
    // ── Parsear configuración (TODO: Alex) ──────────────────────────
    // std::vector<ServerConfig> configs = ConfigParser::parse(argv[1]);
    //
    // STUB TEMPORAL: un servidor hardcoded en puerto 8080
    ServerConfig defaultConfig;
    defaultConfig.host = "0.0.0.0";
    defaultConfig.port = 8080;
    defaultConfig.serverName = "localhost";

    // ── Crear PollManager ───────────────────────────────────────────
    PollManager manager;
    g_manager = &manager;

    // ── Crear ServerSocket por cada bloque server{} ─────────────────
    ServerSocket* server = new ServerSocket(defaultConfig);
    server->init();
    manager.addServer(server);

    // ── Arrancar event-loop (bloquea hasta SIGINT) ──────────────────
    SocketUtils::logInfo("WebServer starting...");
    manager.run();
    SocketUtils::logInfo("WebServer shutdown complete");
  } catch (const std::exception& e) {
    std::cerr << "Fatal: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
*/
