/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Types.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhiguita <rhiguita@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/28                               #+#    #+#             */
/*   Updated: 2026/06/28                              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TYPES_H_
#define TYPES_H_

#include <cstddef>
#include <map>
#include <string>
#include <vector>

// ─── Contrato compartido entre Alex (Parser), Ruben (Sockets), Ángel (Logic) ─

// ── Configuración de una location dentro de un server{} ─────────────────────
struct LocationConfig {
  std::string path;                         // "/uploads"
  std::vector<std::string> allowedMethods;  // ["GET", "POST"]
  std::string root;                         // "./www"
  std::string index;                        // "index.html"
  bool autoindex;                           // listado de directorio
  size_t clientMaxBodySize;                 // bytes
  std::string cgiPass;                      // ruta al intérprete
  std::string redirect;                     // URL de redirección

  LocationConfig()
    : autoindex(false),
      clientMaxBodySize(1048576)  // 1MB default
  {}
};

// ── Configuración de un bloque server{} ─────────────────────────────────────
struct ServerConfig {
  std::string host;                       // "0.0.0.0"
  int port;                               // 8080
  std::string serverName;                 // "localhost"
  std::map<int, std::string> errorPages;  // {404: "./404.html"}
  std::vector<LocationConfig> locations;
  size_t clientMaxBodySize;

  ServerConfig()
    : host("0.0.0.0"), port(8080), serverName("localhost"), clientMaxBodySize(1048576) {}
};

// ── Petición cruda (lo que Ruben entrega a Alex) ────────────────────────────
struct RawRequest {
  int fd;            // FD del cliente (para correlacionar respuesta)
  std::string data;  // bytes crudos: "GET / HTTP/1.1\r\n..."
  bool complete;     // ¿petición HTTP completa?

  RawRequest() : fd(-1), complete(false) {}
};

// ── Respuesta cruda (lo que Ángel devuelve a Ruben) ─────────────────────────
struct RawResponse {
  int fd;            // FD del cliente destino
  std::string data;  // bytes crudos: "HTTP/1.1 200 OK\r\n..."
  size_t bytesSent;  // progreso de envío parcial

  RawResponse() : fd(-1), bytesSent(0) {}
};

// ── Estados de conexión de un cliente ───────────────────────────────────────
enum ConnectionState {
  STATE_READING,     // Leyendo petición del cliente
  STATE_PROCESSING,  // Esperando que Alex/Ángel procesen
  STATE_WRITING,     // Enviando respuesta al cliente
  STATE_CLOSING      // Marcado para cierre
};

#endif  // TYPES_H_
