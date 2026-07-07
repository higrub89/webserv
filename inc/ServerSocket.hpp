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

#ifndef SERVERSOCKET_H_
#define SERVERSOCKET_H_

#include <netinet/in.h>

#include "Types.hpp"

class ServerSocket {
public:
  // Constructor: recibe la config de UN bloque server{}
  explicit ServerSocket(const ServerConfig& config);
  ~ServerSocket();

  // Inicialización completa: socket → setsockopt → bind → listen
  void init();

  // Acepta UNA nueva conexión. Retorna FD del cliente o -1 si EAGAIN.
  int acceptClient(struct sockaddr_in& clientAddr) const;

  // Getters
  int getFd() const;
  int getPort() const;
  const ServerConfig& getConfig() const;

private:
  ServerConfig config_;
  int fd_;
  struct sockaddr_in addr_;

  // Helpers de inicialización (se llaman desde init())
  void createSocket();
  void setSocketOptions();
  void bindSocket();
  void listenSocket();

  // Orthodox Canonical Form: prohibir copia
  ServerSocket(const ServerSocket&);
  ServerSocket& operator=(const ServerSocket&);
};

#endif  // SERVERSOCKET_H_
