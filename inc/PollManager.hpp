/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PollManager.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhiguita <rhiguita@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/28                               #+#    #+#             */
/*   Updated: 2026/06/28                              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef POLLMANAGER_H_
#define POLLMANAGER_H_

#include <poll.h>

#include <map>
#include <vector>

#include "ClientConnection.hpp"
#include "ServerSocket.hpp"
#include "Types.hpp"

#define MAX_CONNECTIONS 1024
#define POLL_TIMEOUT_MS 5000

class PollManager {
public:
  PollManager();
  ~PollManager();

  // Registrar un socket de escucha (uno por bloque server{} en config)
  void addServer(ServerSocket* server);

  // Bucle principal. Bloquea hasta señal de parada.
  void run();

  // Detener el loop (llamado desde signal handler)
  void stop();

private:
  // Sockets de escucha (propiedad de PollManager: los libera en destructor)
  std::vector<ServerSocket*> servers_;

  // Conexiones activas: fd → ClientConnection*
  std::map<int, ClientConnection*> clients_;

  // Mapa rápido: fd del server → puntero ServerSocket (para saber cuál aceptó)
  std::map<int, ServerSocket*> serverFdMap_;

  // Array que se pasa a poll()
  std::vector<struct pollfd> pollfds_;

  // Flag de control del loop
  bool running_;

  // ─── Gestión del array de poll ──────────────────────────────────────────
  void rebuildPollFds();
  void addFdToPoll(int fd, short events);
  void removeFdFromPoll(int fd);
  void updatePollEvents(int fd, short events);

  // ─── Handlers de eventos ────────────────────────────────────────────────
  void handleNewConnection(ServerSocket* server);
  void handleClientRead(ClientConnection* client);
  void handleClientWrite(ClientConnection* client);
  void handleClientError(ClientConnection* client);

  // ─── Mantenimiento ──────────────────────────────────────────────────────
  void cleanupTimedOutConnections();
  void closeClient(int fd);
  void closeAllConnections();

  // ─── Integración con Alex y Ángel ───────────────────────────────────────
  void dispatchRequest(ClientConnection* client);
  void enqueueResponse(int fd, const std::string& responseData);

  // Orthodox Canonical Form: prohibir copia
  PollManager(const PollManager&);
  PollManager& operator=(const PollManager&);
};

#endif  // POLLMANAGER_H_
