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

#include "PollManager.hpp"
#include "SocketUtils.hpp"
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <algorithm>

// ─── Constructor / Destructor ───────────────────────────────────────────────

PollManager::PollManager()
	: _running(false)
{}

PollManager::~PollManager()
{
	_closeAllConnections();
	for (size_t i = 0; i < _servers.size(); ++i)
		delete _servers[i];
}

// ─── Registrar servidor ────────────────────────────────────────────────────

void PollManager::addServer(ServerSocket* server)
{
	_servers.push_back(server);
	_serverFdMap[server->getFd()] = server;

	struct pollfd pfd;
	pfd.fd = server->getFd();
	pfd.events = POLLIN;	// Solo escuchamos conexiones entrantes
	pfd.revents = 0;
	_pollfds.push_back(pfd);

	std::ostringstream oss;
	oss << "Server registered on fd=" << server->getFd()
		<< " port=" << server->getPort();
	SocketUtils::logInfo(oss.str());
}

// ─── Event-Loop Principal ──────────────────────────────────────────────────

void PollManager::run()
{
	_running = true;
	SocketUtils::logInfo("Event loop started");

	while (_running)
	{
		int ready = poll(_pollfds.data(), _pollfds.size(), POLL_TIMEOUT_MS);

		if (ready < 0)
		{
			if (errno == EINTR)
				continue;	// Señal recibida (SIGINT), volver al while
			SocketUtils::logError(
				std::string("poll() failed: ") + strerror(errno));
			break;
		}

		if (ready == 0)
		{
			// Timeout: ningún FD tiene actividad → limpiar muertos
			_cleanupTimedOutConnections();
			continue;
		}

		// Iterar con índice (no iterator) porque podemos modificar _pollfds
		size_t size = _pollfds.size();
		for (size_t i = 0; i < size && ready > 0; ++i)
		{
			if (_pollfds[i].revents == 0)
				continue;
			--ready;

			int fd = _pollfds[i].fd;
			short revents = _pollfds[i].revents;

			// ¿Es un socket de escucha? → nueva conexión
			if (_serverFdMap.count(fd))
			{
				_handleNewConnection(_serverFdMap[fd]);
				continue;
			}

			// ¿Es un cliente?
			if (!_clients.count(fd))
				continue;
			ClientConnection* client = _clients[fd];

			if (revents & (POLLERR | POLLHUP | POLLNVAL))
				_handleClientError(client);
			else if (revents & POLLIN)
				_handleClientRead(client);
			else if (revents & POLLOUT)
				_handleClientWrite(client);
		}

		_cleanupTimedOutConnections();
	}

	SocketUtils::logInfo("Event loop stopped");
}

void PollManager::stop()
{
	_running = false;
}

// ─── Handler: nueva conexión ───────────────────────────────────────────────

void PollManager::_handleNewConnection(ServerSocket* server)
{
	struct sockaddr_in clientAddr;
	int clientFd = server->acceptClient(clientAddr);

	if (clientFd < 0)
		return;

	if (_clients.size() >= MAX_CONNECTIONS)
	{
		const char* msg = "HTTP/1.1 503 Service Unavailable\r\n"
						  "Content-Length: 0\r\n"
						  "Connection: close\r\n\r\n";
		send(clientFd, msg, std::strlen(msg), MSG_NOSIGNAL);
		close(clientFd);
		SocketUtils::logError("Max connections reached, rejecting client");
		return;
	}

	ClientConnection* conn = new ClientConnection(
		clientFd, server->getConfig());
	_clients[clientFd] = conn;
	_addFdToPoll(clientFd, POLLIN);

	std::ostringstream oss;
	oss << "New client fd=" << clientFd
		<< " from " << SocketUtils::addrToString(clientAddr)
		<< " on port " << server->getPort();
	SocketUtils::logInfo(oss.str());
}

// ─── Handler: lectura de cliente ───────────────────────────────────────────

void PollManager::_handleClientRead(ClientConnection* client)
{
	ssize_t n = client->readData();

	if (n <= 0 && client->getState() == STATE_CLOSING)
	{
		_closeClient(client->getFd());
		return;
	}

	// ¿Petición HTTP completa? → despachar a Alex + Ángel
	if (client->isRequestComplete())
	{
		client->setState(STATE_PROCESSING);
		_dispatchRequest(client);
	}
}

// ─── Handler: escritura a cliente ──────────────────────────────────────────

void PollManager::_handleClientWrite(ClientConnection* client)
{
	client->writeData();

	if (client->getState() == STATE_CLOSING)
	{
		_closeClient(client->getFd());
		return;
	}

	if (client->isResponseComplete())
	{
		if (client->isKeepAlive())
		{
			client->reset();
			_updatePollEvents(client->getFd(), POLLIN);
		}
		else
			_closeClient(client->getFd());
	}
}

// ─── Handler: error de cliente ─────────────────────────────────────────────

void PollManager::_handleClientError(ClientConnection* client)
{
	std::ostringstream oss;
	oss << "POLLERR/POLLHUP on fd=" << client->getFd();
	SocketUtils::logDebug(oss.str());
	_closeClient(client->getFd());
}

// ─── Dispatch: punto de integración con Alex y Ángel ───────────────────────

void PollManager::_dispatchRequest(ClientConnection* client)
{
	RawRequest req = client->extractRequest();

	// ──────────────────────────────────────────────────────────────────
	// STUB TEMPORAL: respuesta hardcoded para testing.
	// Cuando Alex y Ángel tengan sus módulos, esto se reemplaza por:
	//
	//   HttpRequest parsed = Parser::parse(req);           // Alex
	//   HttpResponse resp = Handler::handle(parsed, ...);  // Ángel
	//   _enqueueResponse(client->getFd(), resp.serialize());
	// ──────────────────────────────────────────────────────────────────

	std::string body =
		"<html><body>"
		"<h1>WebServer 42</h1>"
		"<p>Socket layer OK — Ruben</p>"
		"<pre>" + req.data + "</pre>"
		"</body></html>";

	std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n"
		<< "Content-Type: text/html\r\n"
		<< "Content-Length: " << body.size() << "\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< body;

	_enqueueResponse(client->getFd(), oss.str());
}

void PollManager::_enqueueResponse(int fd, const std::string& responseData)
{
	if (!_clients.count(fd))
		return;

	RawResponse resp;
	resp.fd = fd;
	resp.data = responseData;
	resp.bytesSent = 0;

	_clients[fd]->queueResponse(resp);
	_updatePollEvents(fd, POLLOUT);
}

// ─── Gestión del array de poll ─────────────────────────────────────────────

void PollManager::_rebuildPollFds()
{
	// No es necesario reconstruir completamente cada ciclo si mantenemos
	// _pollfds sincronizado con add/remove/update. Este método existe como
	// fallback de seguridad si el array se desincroniza.
	(void)this;
}

void PollManager::_addFdToPoll(int fd, short events)
{
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;
	_pollfds.push_back(pfd);
}

void PollManager::_removeFdFromPoll(int fd)
{
	for (size_t i = 0; i < _pollfds.size(); ++i)
	{
		if (_pollfds[i].fd == fd)
		{
			_pollfds.erase(_pollfds.begin() + i);
			return;
		}
	}
}

void PollManager::_updatePollEvents(int fd, short events)
{
	for (size_t i = 0; i < _pollfds.size(); ++i)
	{
		if (_pollfds[i].fd == fd)
		{
			_pollfds[i].events = events;
			return;
		}
	}
}

// ─── Limpieza ──────────────────────────────────────────────────────────────

void PollManager::_cleanupTimedOutConnections()
{
	std::vector<int> toClose;

	std::map<int, ClientConnection*>::iterator it;
	for (it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->isTimedOut())
			toClose.push_back(it->first);
	}

	for (size_t i = 0; i < toClose.size(); ++i)
	{
		std::ostringstream oss;
		oss << "Timeout on fd=" << toClose[i];
		SocketUtils::logInfo(oss.str());
		_closeClient(toClose[i]);
	}
}

void PollManager::_closeClient(int fd)
{
	_removeFdFromPoll(fd);

	std::map<int, ClientConnection*>::iterator it = _clients.find(fd);
	if (it != _clients.end())
	{
		delete it->second;
		_clients.erase(it);
	}
}

void PollManager::_closeAllConnections()
{
	std::map<int, ClientConnection*>::iterator it;
	for (it = _clients.begin(); it != _clients.end(); ++it)
		delete it->second;
	_clients.clear();
	_pollfds.clear();
}
