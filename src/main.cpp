#include <csignal>
#include <iostream>

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
    // Simula una configuración completa con múltiples puertos, servidores virtuales y rutas
    ConfigMap configMap;

    // ─── Puerto 8080: Servidor Principal y Virtual Host API ──────────
    {
      ServerGroup group8080;
      group8080.ip = "0.0.0.0";
      group8080.port = 8080;

      // 1. Servidor Principal (Default Server en 8080)
      ServerConfig mainServer;
      mainServer.server_names.push_back("localhost");
      mainServer.server_names.push_back("127.0.0.1");
      mainServer.root_dir = "./www";
      mainServer.client_max_body_size = 10485760;  // 10 MB
      mainServer.error_pages[404] = "./www/errors/404.html";
      mainServer.error_pages[500] = "./www/errors/500.html";

      // Ruta raíz "/"
      LocationConfig rootLoc;
      rootLoc.allowed_methods.push_back("GET");
      rootLoc.allowed_methods.push_back("POST");
      rootLoc.root_dir = "./www";
      rootLoc.index_file = "index.html";
      rootLoc.autoindex = false;
      rootLoc.client_max_body_size = 0;
      rootLoc.upload_enable = false;
      mainServer.locations["/"] = rootLoc;

      // Ruta de subida de archivos "/uploads"
      LocationConfig uploadLoc;
      uploadLoc.allowed_methods.push_back("GET");
      uploadLoc.allowed_methods.push_back("POST");
      uploadLoc.allowed_methods.push_back("DELETE");
      uploadLoc.root_dir = "./www/uploads";
      uploadLoc.upload_enable = true;
      uploadLoc.upload_store = "./www/uploads";
      uploadLoc.client_max_body_size = 52428800;  // 50 MB
      uploadLoc.autoindex = true;
      mainServer.locations["/uploads"] = uploadLoc;

      // Ruta de scripts CGI "/cgi-bin"
      LocationConfig cgiLoc;
      cgiLoc.allowed_methods.push_back("GET");
      cgiLoc.allowed_methods.push_back("POST");
      cgiLoc.root_dir = "./www/cgi-bin";
      cgiLoc.autoindex = false;
      cgiLoc.client_max_body_size = 0;
      cgiLoc.upload_enable = false;
      cgiLoc.cgi_handlers[".py"] = "/usr/bin/python3";
      cgiLoc.cgi_handlers[".sh"] = "/bin/bash";
      mainServer.locations["/cgi-bin"] = cgiLoc;

      // Ruta con Autoindex activado "/docs"
      LocationConfig docsLoc;
      docsLoc.allowed_methods.push_back("GET");
      docsLoc.root_dir = "./www/docs";
      docsLoc.autoindex = true;
      docsLoc.client_max_body_size = 0;
      docsLoc.upload_enable = false;
      mainServer.locations["/docs"] = docsLoc;

      // Ruta con Redirección HTTP "/intra"
      LocationConfig redirLoc;
      redirLoc.allowed_methods.push_back("GET");
      redirLoc.return_redirect = "https://profile.intra.42.fr";
      redirLoc.autoindex = false;
      redirLoc.client_max_body_size = 0;
      redirLoc.upload_enable = false;
      mainServer.locations["/intra"] = redirLoc;

      group8080.servers.push_back(mainServer);

      // 2. Servidor Virtual secundario en el mismo puerto 8080 ("api.local")
      ServerConfig apiServer;
      apiServer.server_names.push_back("api.local");
      apiServer.root_dir = "./www/api";
      apiServer.client_max_body_size = 2097152;  // 2 MB

      LocationConfig apiLoc;
      apiLoc.allowed_methods.push_back("GET");
      apiLoc.allowed_methods.push_back("POST");
      apiLoc.root_dir = "./www/api";
      apiLoc.index_file = "api.json";
      apiLoc.autoindex = false;
      apiLoc.client_max_body_size = 0;
      apiLoc.upload_enable = false;
      apiServer.locations["/"] = apiLoc;

      group8080.servers.push_back(apiServer);

      configMap["8080"] = group8080;
    }

    // ─── Puerto 8081: Servidor Secundario / Pruebas ──────────────────
    {
      ServerGroup group8081;
      group8081.ip = "0.0.0.0";
      group8081.port = 8081;

      ServerConfig testServer;
      testServer.server_names.push_back("localhost");
      testServer.server_names.push_back("test.local");
      testServer.root_dir = "./www/test";
      testServer.client_max_body_size = 1048576;  // 1 MB

      LocationConfig testLoc;
      testLoc.allowed_methods.push_back("GET");
      testLoc.root_dir = "./www/test";
      testLoc.index_file = "test.html";
      testLoc.autoindex = true;
      testLoc.client_max_body_size = 0;
      testLoc.upload_enable = false;
      testServer.locations["/"] = testLoc;

      group8081.servers.push_back(testServer);

      configMap["8081"] = group8081;
    }

    // ── Crear componentes ───────────────────────────────────────────
    Router router(configMap, envp);
    EpollManager epoll;
    epoll.init();

    // ── Crear ServerHandler por cada puerto del ConfigMap ───────────
    std::vector<ServerHandler*> listeningServers;
    for (ConfigMap::const_iterator it = configMap.begin(); it != configMap.end(); ++it) {
      const ServerGroup& group = it->second;
      ServerHandler* server = new ServerHandler(group.port, group.servers[0], epoll, router);
      server->setup();
      epoll.addHandler(server, EPOLLIN);
      listeningServers.push_back(server);
    }

    // ── Arrancar event-loop ─────────────────────────────────────────
    epoll.run();

    // ── Limpieza de recursos al apagar ──────────────────────────────
    for (size_t i = 0; i < listeningServers.size(); ++i) {
      delete listeningServers[i];
    }
    listeningServers.clear();

    Logger::info("WebServer shutdown complete");
  } catch (const std::exception& e) {
    Logger::error(std::string("Fatal error: ") + e.what());
    return 1;
  }
  return 0;
}
