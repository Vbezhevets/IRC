
# ---- Project ----
NAME     := ircserv

# ---- Compiler & flags ----
CXX      := c++
CXXFLAGS :=  -std=c++98 -Wall -Wextra -Werror

SRC_DIR := ./src
UTIL_DIR := $(SRC_DIR)/utils
SERVER_DIR := $(SRC_DIR)/server
CLIENT_DIR := $(SRC_DIR)/client
PROTOCOL_DIR := $(SRC_DIR)/protocol
CHANNEL_DIR := $(SRC_DIR)/channel

DEP_DIR := ./deps
OBJ_DIR := ./obj

UTIL_SRC := $(addprefix $(UTIL_DIR)/, $(addsuffix .cpp, \
				))
SERVER_SRC := $(addprefix $(SERVER_DIR)/, $(addsuffix .cpp, \
					Server \
				))
CLIENT_SRC := $(addprefix $(CLIENT_DIR)/, $(addsuffix .cpp, \
			  		Client \
				))
PROTOCOL_SRC := $(addprefix $(PROTOCOL_DIR)/, $(addsuffix .cpp, \
					Irc handlers \
				))
CHANNEL_SRC := $(addprefix $(CHANNEL_DIR)/, $(addsuffix .cpp, \
					Channel \
				))


# ---- Files ----
SRCS := $(SRC_DIR)/main.cpp $(SERVER_SRC) $(CLIENT_SRC) $(PROTOCOL_SRC) $(UTIL_SRC) $(CHANNEL_SRC)
OBJS := $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.cpp=.o)))
DEPS := $(addprefix $(DEP_DIR)/, $(notdir $(SRCS:.cpp=.d)))

# ---- Default rule ----
all: $(NAME)

# ---- Link ----
$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

# ---- Compile ----
$(OBJ_DIR)/%.o: $(SRC_DIR)/*/%.cpp | $(OBJ_DIR) $(DEP_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MF $(DEP_DIR)/$*.d -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR) $(DEP_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MF $(DEP_DIR)/$*.d -c $< -o $@

# ---- Cleaning ----
clean:
	rm -rf $(OBJ_DIR) $(DEP_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

$(OBJ_DIR):
	mkdir -p $@

$(DEP_DIR):
	mkdir -p $@

# ---- Phony ----
.PHONY: all clean fclean re

# ---- Include generated dependencies ----
-include $(DEPS)
