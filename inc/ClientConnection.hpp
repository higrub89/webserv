/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientConnection.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhiguita <rhiguita@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/28                               #+#    #+#             */
/*   Updated: 2026/06/28                              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENTCONNECTION_H_
#define CLIENTCONNECTION_H_

#include <sys/types.h>

#include <ctime>

#include "Types.hpp"

#define CONNECTION_TIMEOUT_SECS 60
#define READ_BUFFER_SIZE 8192

class ClientConnection {
public:
  ClientConnection(int fd, const ServerConfig& serverConfig);
  ~ClientConnection();

  // ─── Ciclo de vida ──────────────────────────────────────────────────────
  // Lee datos disponibles del FD. Retorna bytes leídos, 0=EOF, -1=EAGAIN/error
  ssize_t readData();

  // Envía datos pendientes al cliente. Gestiona envío parcial.
  ssize_t writeData();

  // ¿La petición HTTP está completa? (\r\n\r\n encontrado)
  bool isRequestComplete() const;

  // ¿Se envió toda la respuesta?
  bool isResponseComplete() const;

  // ¿Conexión expirada por inactividad?
  bool isTimedOut() const;

  // ─── Integración con Alex (Parser) ──────────────────────────────────────
  RawRequest extractRequest() const;

  // ─── Integración con Ángel (Response Builder) ───────────────────────────
  void queueResponse(const RawResponse& response);

  // ─── Getters / Setters ──────────────────────────────────────────────────
  int getFd() const;
  ConnectionState getState() const;
  void setState(ConnectionState state);
  const ServerConfig& getServerConfig() const;

  bool isKeepAlive() const;
  void setKeepAlive(bool val);

  // Reset para reutilizar la conexión (keep-alive)
  void reset();

private:
  int fd_;
  ConnectionState state_;
  const ServerConfig& serverConfig_;

  // Buffer de entrada (acumulativo: recv() puede fragmentar)
  std::string readBuffer_;

  // Buffer de salida (con offset para envío parcial)
  std::string writeBuffer_;
  size_t writeOffset_;

  // Gestión de tiempo
  std::time_t lastActivity_;

  // Keep-Alive
  bool keepAlive_;

  // Orthodox Canonical Form: prohibir copia
  ClientConnection(const ClientConnection&);
  ClientConnection& operator=(const ClientConnection&);
};

#endif  // CLIENTCONNECTION_H_
