/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhiguita <rhiguita@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/28                               #+#    #+#             */
/*   Updated: 2026/06/28                              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERSOCKET_HPP
# define SERVERSOCKET_HPP

# include "Types.hpp"
# include <netinet/in.h>

class ServerSocket
{
public:
	// Constructor: recibe la config de UN bloque server{}
	explicit ServerSocket(const ServerConfig& config);
	~ServerSocket();

	// Inicialización completa: socket → setsockopt → bind → listen
	void	init();

	// Acepta UNA nueva conexión. Retorna FD del cliente o -1 si EAGAIN.
	int		acceptClient(struct sockaddr_in& clientAddr) const;

	// Getters
	int						getFd() const;
	int						getPort() const;
	const ServerConfig&		getConfig() const;

private:
	ServerConfig			_config;
	int						_fd;
	struct sockaddr_in		_addr;

	// Helpers de inicialización (se llaman desde init())
	void	_createSocket();
	void	_setSocketOptions();
	void	_bindSocket();
	void	_listenSocket();

	// Orthodox Canonical Form: prohibir copia
	ServerSocket(const ServerSocket&);
	ServerSocket& operator=(const ServerSocket&);
};

#endif // SERVERSOCKET_HPP
