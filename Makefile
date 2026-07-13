NAME := webserv

CXX ?= c++

SRC_DIR	:= src
OBJ_DIR	:= obj
INC_DIR := inc

INCFLAGS := \
  -I $(INC_DIR) \
  -I $(INC_DIR)/core \
  -I $(INC_DIR)/http \
  -I $(INC_DIR)/router \
  -I $(INC_DIR)/cgi \
  -I $(INC_DIR)/methods \
  -I $(INC_DIR)/types

CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -pedantic $(INCFLAGS) $(OPTFLAGS) -MMD -MP
OPTFLAGS ?= -O2
# LDLIBS :=

SRCS := \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/cgi/CgiReadHandler.cpp \
	$(SRC_DIR)/cgi/CgiWriteHandler.cpp \
	$(SRC_DIR)/core/AEventHandler.cpp \
	$(SRC_DIR)/core/ClientHandler.cpp \
	$(SRC_DIR)/core/Logger.cpp \
	$(SRC_DIR)/core/Utils.cpp \
	$(SRC_DIR)/core/EpollManager.cpp \
	$(SRC_DIR)/core/ServerHandler.cpp \
	$(SRC_DIR)/http/HttpParser.cpp \
	$(SRC_DIR)/http/HttpRequest.cpp \
	$(SRC_DIR)/http/HttpResponse.cpp \
	$(SRC_DIR)/router/Router.cpp

OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@printf "\033[32m✓ %s compiled successfully\033[0m\n" "$(NAME)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) -r $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

debug: CXXFLAGS += -g3 -fno-omit-frame-pointer -fsanitize=address,leak,undefined -DDEBUG
debug: OPTFLAGS = -Og
debug: re

-include $(DEPS)

.PHONY: all clean fclean re debug
