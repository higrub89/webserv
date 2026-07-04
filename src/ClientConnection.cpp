/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientConnection.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhiguita <rhiguita@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/28                               #+#    #+#             */
/*   Updated: 2026/06/28                              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ClientConnection.hpp"
#include "SocketUtils.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>

// ─── Constructor / Destructor ───────────────────────────────────────────────

ClientConnection::ClientConnection(int fd, const ServerConfig& serverConfig)
	: _fd(fd)
	, _state(STATE_READING)
	, _serverConfig(serverConfig)
	, _writeOffset(0)
	, _lastActivity(std::time(NULL))
	, _keepAlive(true)
{}

ClientConnection::~ClientConnection()
{
	if (_fd >= 0)
		close(_fd);
}

// ─── Lectura (acumulativa) ──────────────────────────────────────────────────

ssize_t ClientConnection::readData()
{
	char chunk[READ_BUFFER_SIZE];
	ssize_t n = recv(_fd, chunk, sizeof(chunk), 0);

	if (n > 0)
	{
		_readBuffer.append(chunk, n);
		_lastActivity = std::time(NULL);
	}
	else if (n == 0)
		_state = STATE_CLOSING;
	else if (errno != EAGAIN && errno != EWOULDBLOCK)
		_state = STATE_CLOSING;
	return n;
}

// ─── Escritura (parcial con offset) ─────────────────────────────────────────

ssize_t ClientConnection::writeData()
{
	if (_writeOffset >= _writeBuffer.size())
		return 0;

	const char*	ptr = _writeBuffer.c_str() + _writeOffset;
	size_t		remaining = _writeBuffer.size() - _writeOffset;

	ssize_t n = send(_fd, ptr, remaining, MSG_NOSIGNAL);
	if (n > 0)
	{
		_writeOffset += n;
		_lastActivity = std::time(NULL);
	}
	else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
		_state = STATE_CLOSING;
	return n;
}

// ─── Checks de estado ──────────────────────────────────────────────────────

bool ClientConnection::isRequestComplete() const
{
	return _readBuffer.find("\r\n\r\n") != std::string::npos;
}

bool ClientConnection::isResponseComplete() const
{
	return _writeOffset >= _writeBuffer.size() && !_writeBuffer.empty();
}

bool ClientConnection::isTimedOut() const
{
	return (std::time(NULL) - _lastActivity) > CONNECTION_TIMEOUT_SECS;
}

// ─── Integración con Alex ──────────────────────────────────────────────────

RawRequest ClientConnection::extractRequest() const
{
	RawRequest req;
	req.fd = _fd;
	req.data = _readBuffer;
	req.complete = isRequestComplete();
	return req;
}

// ─── Integración con Ángel ─────────────────────────────────────────────────

void ClientConnection::queueResponse(const RawResponse& response)
{
	_writeBuffer = response.data;
	_writeOffset = 0;
	_state = STATE_WRITING;
}

// ─── Getters / Setters ─────────────────────────────────────────────────────

int ClientConnection::getFd() const { return _fd; }
ConnectionState ClientConnection::getState() const { return _state; }
void ClientConnection::setState(ConnectionState state) { _state = state; }
const ServerConfig& ClientConnection::getServerConfig() const { return _serverConfig; }
bool ClientConnection::isKeepAlive() const { return _keepAlive; }
void ClientConnection::setKeepAlive(bool val) { _keepAlive = val; }

// ─── Reset para keep-alive ─────────────────────────────────────────────────

void ClientConnection::reset()
{
	_readBuffer.clear();
	_writeBuffer.clear();
	_writeOffset = 0;
	_state = STATE_READING;
	_lastActivity = std::time(NULL);
}
