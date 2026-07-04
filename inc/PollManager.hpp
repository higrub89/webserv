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

#ifndef POLLMANAGER_HPP
# define POLLMANAGER_HPP

# include "ServerSocket.hpp"
# include "ClientConnection.hpp"
# include "Types.hpp"
# include <poll.h>
# include <map>
# include <vector>

# define MAX_CONNECTIONS	1024
# define POLL_TIMEOUT_MS	5000

class PollManager
{
public:
	PollManager();
	~PollManager();

	// Registrar un socket de escucha (uno por bloque server{} en config)
	void	addServer(ServerSocket* server);

	// Bucle principal. Bloquea hasta señal de parada.
	void	run();

	// Detener el loop (llamado desde signal handler)
	void	stop();

private:
	// Sockets de escucha (propiedad de PollManager: los libera en destructor)
	std::vector<ServerSocket*>			_servers;

	// Conexiones activas: fd → ClientConnection*
	std::map<int, ClientConnection*>	_clients;

	// Mapa rápido: fd del server → puntero ServerSocket (para saber cuál aceptó)
	std::map<int, ServerSocket*>		_serverFdMap;

	// Array que se pasa a poll()
	std::vector<struct pollfd>			_pollfds;

	// Flag de control del loop
	bool								_running;

	// ─── Gestión del array de poll ──────────────────────────────────────────
	void	_rebuildPollFds();
	void	_addFdToPoll(int fd, short events);
	void	_removeFdFromPoll(int fd);
	void	_updatePollEvents(int fd, short events);

	// ─── Handlers de eventos ────────────────────────────────────────────────
	void	_handleNewConnection(ServerSocket* server);
	void	_handleClientRead(ClientConnection* client);
	void	_handleClientWrite(ClientConnection* client);
	void	_handleClientError(ClientConnection* client);

	// ─── Mantenimiento ──────────────────────────────────────────────────────
	void	_cleanupTimedOutConnections();
	void	_closeClient(int fd);
	void	_closeAllConnections();

	// ─── Integración con Alex y Ángel ───────────────────────────────────────
	void	_dispatchRequest(ClientConnection* client);
	void	_enqueueResponse(int fd, const std::string& responseData);

	// Orthodox Canonical Form: prohibir copia
	PollManager(const PollManager&);
	PollManager& operator=(const PollManager&);
};

#endif // POLLMANAGER_HPP
