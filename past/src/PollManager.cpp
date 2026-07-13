/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PollManager.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhiguita <rhiguita@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/28                               #+#    #+#             */
/*   Updated: 2026/06/28                              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
/*
#include "PollManager.hpp"

#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <sstream>

#include "SocketUtils.hpp"

// ─── Constructor / Destructor ───────────────────────────────────────────────

PollManager::PollManager() : running_(false) {}

PollManager::~PollManager() {
  closeAllConnections();
  for (size_t i = 0; i < servers_.size(); ++i)
    delete servers_[i];
}

// ─── Registrar servidor ────────────────────────────────────────────────────

void PollManager::addServer(ServerSocket* server) {
  servers_.push_back(server);
  serverFdMap_[server->getFd()] = server;

  struct pollfd pfd;
  pfd.fd = server->getFd();
  pfd.events = POLLIN;  // Solo escuchamos conexiones entrantes
  pfd.revents = 0;
  pollfds_.push_back(pfd);

  std::ostringstream oss;
  oss << "Server registered on fd=" << server->getFd() << " port=" <<
server->getPort(); SocketUtils::logInfo(oss.str());
}

// ─── Event-Loop Principal ──────────────────────────────────────────────────

void PollManager::run() {
  running_ = true;
  SocketUtils::logInfo("Event loop started");

  while (running_) {
    int ready = poll(pollfds_.data(), pollfds_.size(), POLL_TIMEOUT_MS);

    if (ready < 0) {
      if (errno == EINTR)
        continue;  // Señal recibida (SIGINT), volver al while
      SocketUtils::logError(std::string("poll() failed: ") + strerror(errno));
      break;
    }

    if (ready == 0) {
      // Timeout: ningún FD tiene actividad → limpiar muertos
      cleanupTimedOutConnections();
      continue;
    }

    // Iterar con índice (no iterator) porque podemos modificar pollfds_
    size_t size = pollfds_.size();
    for (size_t i = 0; i < size && ready > 0; ++i) {
      if (pollfds_[i].revents == 0)
        continue;
      --ready;

      int fd = pollfds_[i].fd;
      short revents = pollfds_[i].revents;

      // ¿Es un socket de escucha? → nueva conexión
      if (serverFdMap_.count(fd)) {
        handleNewConnection(serverFdMap_[fd]);
        continue;
      }

      // ¿Es un cliente?
      if (!clients_.count(fd))
        continue;
      ClientConnection* client = clients_[fd];

      if (revents & (POLLERR | POLLHUP | POLLNVAL))
        handleClientError(client);
      else if (revents & POLLIN)
        handleClientRead(client);
      else if (revents & POLLOUT)
        handleClientWrite(client);
    }

    cleanupTimedOutConnections();
  }

  SocketUtils::logInfo("Event loop stopped");
}

void PollManager::stop() { running_ = false; }

// ─── Handler: nueva conexión ───────────────────────────────────────────────

void PollManager::handleNewConnection(ServerSocket* server) {
  struct sockaddr_in clientAddr;
  int clientFd = server->acceptClient(clientAddr);

  if (clientFd < 0)
    return;

  if (clients_.size() >= MAX_CONNECTIONS) {
    const char* msg =
      "HTTP/1.1 503 Service Unavailable\r\n"
      "Content-Length: 0\r\n"
      "Connection: close\r\n\r\n";
    send(clientFd, msg, std::strlen(msg), MSG_NOSIGNAL);
    close(clientFd);
    SocketUtils::logError("Max connections reached, rejecting client");
    return;
  }

  ClientConnection* conn = new ClientConnection(clientFd, server->getConfig());
  clients_[clientFd] = conn;
  addFdToPoll(clientFd, POLLIN);

  std::ostringstream oss;
  oss << "New client fd=" << clientFd << " from " <<
SocketUtils::addrToString(clientAddr)
      << " on port " << server->getPort();
  SocketUtils::logInfo(oss.str());
}

// ─── Handler: lectura de cliente ───────────────────────────────────────────

void PollManager::handleClientRead(ClientConnection* client) {
  ssize_t n = client->readData();

  if (n <= 0 && client->getState() == STATE_CLOSING) {
    closeClient(client->getFd());
    return;
  }

  // ¿Petición HTTP completa? → despachar a Alex + Ángel
  if (client->isRequestComplete()) {
    client->setState(STATE_PROCESSING);
    dispatchRequest(client);
  }
}

// ─── Handler: escritura a cliente ──────────────────────────────────────────

void PollManager::handleClientWrite(ClientConnection* client) {
  client->writeData();

  if (client->getState() == STATE_CLOSING) {
    closeClient(client->getFd());
    return;
  }

  if (client->isResponseComplete()) {
    if (client->isKeepAlive()) {
      client->reset();
      updatePollEvents(client->getFd(), POLLIN);
    } else
      closeClient(client->getFd());
  }
}

// ─── Handler: error de cliente ─────────────────────────────────────────────

void PollManager::handleClientError(ClientConnection* client) {
  std::ostringstream oss;
  oss << "POLLERR/POLLHUP on fd=" << client->getFd();
  SocketUtils::logDebug(oss.str());
  closeClient(client->getFd());
}

// ─── Dispatch: punto de integración con Alex y Ángel ───────────────────────

void PollManager::dispatchRequest(ClientConnection* client) {
  RawRequest req = client->extractRequest();

  // ──────────────────────────────────────────────────────────────────
  // STUB TEMPORAL: respuesta hardcoded para testing.
  // Cuando Alex y Ángel tengan sus módulos, esto se reemplaza por:
  //
  //   HttpRequest parsed = Parser::parse(req);           // Alex
  //   HttpResponse resp = Handler::handle(parsed, ...);  // Ángel
  //   enqueueResponse(client->getFd(), resp.serialize());
  // ──────────────────────────────────────────────────────────────────

  std::string body =
    "<html><body>"
    "<h1>WebServer 42</h1>"
    "<p>Socket layer OK — Ruben</p>"
    "<pre>" +
    req.data +
    "</pre>"
    "</body></html>";

  std::ostringstream oss;
  oss << "HTTP/1.1 200 OK\r\n"
      << "Content-Type: text/html\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Connection: close\r\n"
      << "\r\n"
      << body;

  enqueueResponse(client->getFd(), oss.str());
}

void PollManager::enqueueResponse(int fd, const std::string& responseData) {
  if (!clients_.count(fd))
    return;

  RawResponse resp;
  resp.fd = fd;
  resp.data = responseData;
  resp.bytesSent = 0;

  clients_[fd]->queueResponse(resp);
  updatePollEvents(fd, POLLOUT);
}

// ─── Gestión del array de poll ─────────────────────────────────────────────

void PollManager::rebuildPollFds() {
  // No es necesario reconstruir completamente cada ciclo si mantenemos
  // pollfds_ sincronizado con add/remove/update. Este método existe como
  // fallback de seguridad si el array se desincroniza.
  (void)this;
}

void PollManager::addFdToPoll(int fd, short events) {
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = events;
  pfd.revents = 0;
  pollfds_.push_back(pfd);
}

void PollManager::removeFdFromPoll(int fd) {
  for (size_t i = 0; i < pollfds_.size(); ++i) {
    if (pollfds_[i].fd == fd) {
      pollfds_.erase(pollfds_.begin() + i);
      return;
    }
  }
}

void PollManager::updatePollEvents(int fd, short events) {
  for (size_t i = 0; i < pollfds_.size(); ++i) {
    if (pollfds_[i].fd == fd) {
      pollfds_[i].events = events;
      return;
    }
  }
}

// ─── Limpieza ──────────────────────────────────────────────────────────────

void PollManager::cleanupTimedOutConnections() {
  std::vector<int> toClose;

  std::map<int, ClientConnection*>::iterator it;
  for (it = clients_.begin(); it != clients_.end(); ++it) {
    if (it->second->isTimedOut())
      toClose.push_back(it->first);
  }

  for (size_t i = 0; i < toClose.size(); ++i) {
    std::ostringstream oss;
    oss << "Timeout on fd=" << toClose[i];
    SocketUtils::logInfo(oss.str());
    closeClient(toClose[i]);
  }
}

void PollManager::closeClient(int fd) {
  removeFdFromPoll(fd);

  std::map<int, ClientConnection*>::iterator it = clients_.find(fd);
  if (it != clients_.end()) {
    delete it->second;
    clients_.erase(it);
  }
}

void PollManager::closeAllConnections() {
  std::map<int, ClientConnection*>::iterator it;
  for (it = clients_.begin(); it != clients_.end(); ++it)
    delete it->second;
  clients_.clear();
  pollfds_.clear();
}
*/
