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
