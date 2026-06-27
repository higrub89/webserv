/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhiguita <rhiguita@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/28                               #+#    #+#             */
/*   Updated: 2026/06/28                              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ServerSocket.hpp"
#include "SocketUtils.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sstream>

// ─── Constructor / Destructor ───────────────────────────────────────────────

ServerSocket::ServerSocket(const ServerConfig& config)
	: _config(config)
	, _fd(-1)
{
	std::memset(&_addr, 0, sizeof(_addr));
}

ServerSocket::~ServerSocket()
{
	if (_fd >= 0)
	{
		close(_fd);
		SocketUtils::logInfo("ServerSocket closed on fd " + _fd);
	}
}

// ─── Inicialización pública ─────────────────────────────────────────────────

void ServerSocket::init()
{
	_createSocket();
	_setSocketOptions();
	_bindSocket();
	_listenSocket();

	std::ostringstream oss;
	oss << "Listening on " << _config.host << ":" << _config.port
		<< " (fd=" << _fd << ")";
	SocketUtils::logInfo(oss.str());
}

// ─── Aceptar conexión ──────────────────────────────────────────────────────

int ServerSocket::acceptClient(struct sockaddr_in& clientAddr) const
{
	socklen_t addrLen = sizeof(clientAddr);
	int clientFd = accept(_fd, (struct sockaddr*)&clientAddr, &addrLen);

	if (clientFd < 0)
	{
		// EAGAIN/EWOULDBLOCK es normal en non-blocking: no hay clientes pendientes
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return -1;
		SocketUtils::logError(std::string("accept() failed: ") + strerror(errno));
		return -1;
	}

	// Marcar el FD del nuevo cliente como non-blocking
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
	{
		SocketUtils::logError("fcntl(O_NONBLOCK) failed on client fd");
		close(clientFd);
		return -1;
	}

	SocketUtils::logDebug("Accepted client fd=" + clientFd);
	return clientFd;
}

// ─── Getters ────────────────────────────────────────────────────────────────

int ServerSocket::getFd() const
{
	return _fd;
}

int ServerSocket::getPort() const
{
	return _config.port;
}

const ServerConfig& ServerSocket::getConfig() const
{
	return _config;
}

// ─── Helpers privados ───────────────────────────────────────────────────────

void ServerSocket::_createSocket()
{
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0)
		throw std::runtime_error(
			std::string("socket() failed: ") + strerror(errno));
}

void ServerSocket::_setSocketOptions()
{
	int opt = 1;

	// SO_REUSEADDR: permite reutilizar el puerto inmediatamente tras reiniciar
	// sin esperar al TIME_WAIT del kernel TCP
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error(
			std::string("setsockopt(SO_REUSEADDR) failed: ") + strerror(errno));

	// O_NONBLOCK: crítico para que poll() funcione correctamente.
	// Sin esto, accept()/recv()/send() bloquearían el proceso entero.
	if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error(
			std::string("fcntl(O_NONBLOCK) failed: ") + strerror(errno));
}

void ServerSocket::_bindSocket()
{
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(_config.port);
	_addr.sin_addr.s_addr = SocketUtils::stringToAddr(_config.host);

	if (bind(_fd, (struct sockaddr*)&_addr, sizeof(_addr)) < 0)
	{
		std::ostringstream oss;
		oss << "bind() failed on " << _config.host << ":"
			<< _config.port << ": " << strerror(errno);
		throw std::runtime_error(oss.str());
	}
}

void ServerSocket::_listenSocket()
{
	// SOMAXCONN: máximo de conexiones pendientes en el backlog del kernel.
	// En Linux, suele ser 128 o 4096 según /proc/sys/net/core/somaxconn.
	if (listen(_fd, SOMAXCONN) < 0)
		throw std::runtime_error(
			std::string("listen() failed: ") + strerror(errno));
}
