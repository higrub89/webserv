/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SocketUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhiguita <rhiguita@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/28                               #+#    #+#             */
/*   Updated: 2026/06/28                              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "SocketUtils.hpp"
#include <arpa/inet.h>
#include <ctime>
#include <iostream>
#include <sstream>

// ─── Convierte sockaddr_in a "X.X.X.X:PORT" ────────────────────────────────
std::string SocketUtils::addrToString(const struct sockaddr_in& addr)
{
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));

	std::ostringstream oss;
	oss << ip << ":" << ntohs(addr.sin_port);
	return oss.str();
}

// ─── Convierte string IP a in_addr_t (network byte order) ──────────────────
in_addr_t SocketUtils::stringToAddr(const std::string& ip)
{
	struct in_addr addr;
	if (inet_pton(AF_INET, ip.c_str(), &addr) != 1)
		return INADDR_ANY;
	return addr.s_addr;
}

// ─── Timestamp formateado [28/Jun/2026:00:12:00 +0200] ─────────────────────
std::string SocketUtils::timestamp()
{
	char		buf[64];
	std::time_t	now = std::time(NULL);
	struct tm	tm;

	localtime_r(&now, &tm);
	std::strftime(buf, sizeof(buf), "[%d/%b/%Y:%H:%M:%S %z]", &tm);
	return std::string(buf);
}

// ─── Logging ────────────────────────────────────────────────────────────────
void SocketUtils::logInfo(const std::string& msg)
{
	std::cout << timestamp() << " [INFO]  " << msg << std::endl;
}

void SocketUtils::logError(const std::string& msg)
{
	std::cerr << timestamp() << " [ERROR] " << msg << std::endl;
}

void SocketUtils::logDebug(const std::string& msg)
{
	#ifdef DEBUG
		std::cout << timestamp() << " [DEBUG] " << msg << std::endl;
	#else
		(void)msg;
	#endif
}
