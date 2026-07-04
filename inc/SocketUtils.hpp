/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SocketUtils.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhiguita <rhiguita@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/28                               #+#    #+#             */
/*   Updated: 2026/06/28                              ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SOCKETUTILS_HPP
# define SOCKETUTILS_HPP

# include <string>
# include <netinet/in.h>

namespace SocketUtils
{
	// Convierte sockaddr_in a string legible "X.X.X.X:PORT"
	std::string	addrToString(const struct sockaddr_in& addr);

	// Convierte string IP "0.0.0.0" a in_addr_t (network byte order)
	in_addr_t	stringToAddr(const std::string& ip);

	// Logging con timestamp ISO 8601
	void		logInfo(const std::string& msg);
	void		logError(const std::string& msg);
	void		logDebug(const std::string& msg);

	// Timestamp actual formateado [DD/Mon/YYYY:HH:MM:SS +ZZZZ]
	std::string	timestamp();
}

#endif // SOCKETUTILS_HPP
