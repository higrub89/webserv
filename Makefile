# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: rhiguita <rhiguita@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/06/28                               #+#    #+#              #
#    Updated: 2026/06/28                              ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME		= webserver

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 -I inc

SRC_DIR		= src
OBJ_DIR		= obj

SRCS		= $(SRC_DIR)/main.cpp \
			  $(SRC_DIR)/ServerSocket.cpp \
			  $(SRC_DIR)/ClientConnection.cpp \
			  $(SRC_DIR)/PollManager.cpp \
			  $(SRC_DIR)/SocketUtils.cpp

OBJS		= $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# ─── Reglas ──────────────────────────────────────────────────────────────────

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "\033[32m✓ $(NAME) compiled successfully\033[0m"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

# ─── Extras ──────────────────────────────────────────────────────────────────

debug: CXXFLAGS += -g3 -fsanitize=address -DDEBUG
debug: re

.PHONY: all clean fclean re debug
