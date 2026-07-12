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
  return 0;
}
