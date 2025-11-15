CXX=c++
CXXFLAGS=-Wall -Wextra -Werror -std=c++98 -Iincludes
STRICT_FLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic -pedantic-errors
DEBUG_FLAGS = -g -O0 -DDEBUG -fsanitize=address -fno-omit-frame-pointer

SRC_DIR = srcs
CMD_DIR = $(SRC_DIR)/commands
OBJDIR = obj

SRCS = $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/IrcServer.cpp \
       $(SRC_DIR)/Client.cpp \
       $(SRC_DIR)/Channel.cpp \
       $(SRC_DIR)/CommandHandler.cpp \
	   $(SRC_DIR)/utils.cpp \
	   $(CMD_DIR)/Bot.cpp \
	   $(CMD_DIR)/Cap.cpp \
	   $(CMD_DIR)/Invite.cpp \
	   $(CMD_DIR)/Join.cpp \
	   $(CMD_DIR)/Kick.cpp \
	   $(CMD_DIR)/Mode.cpp \
	   $(CMD_DIR)/Names.cpp \
	   $(CMD_DIR)/Nick.cpp \
	   $(CMD_DIR)/Part.cpp \
	   $(CMD_DIR)/Pass.cpp \
	   $(CMD_DIR)/Ping.cpp \
	   $(CMD_DIR)/Privmsg.cpp \
       $(CMD_DIR)/Quit.cpp \
	   $(CMD_DIR)/Topic.cpp \
	   $(CMD_DIR)/User.cpp \
	   $(CMD_DIR)/Utils.cpp \
	   $(CMD_DIR)/Who.cpp \
	   $(CMD_DIR)/Whois.cpp

OBJDIR = obj
OBJ = $(SRCS:%.cpp=$(OBJDIR)/%.o)
TARGET=ircserv

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

strict: CXXFLAGS = $(STRICT_FLAGS)
strict: re

debug: CXXFLAGS += -g -DDEBUG
debug: re

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(TARGET)

re: fclean all

.PHONY: all clean fclean re strict debug
