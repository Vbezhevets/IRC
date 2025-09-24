
# ---- Project ----
NAME     := ircserv

# ---- Compiler & flags ----
CXX      := c++
CXXFLAGS :=  -std=c++98 -Wall -Wextra -Werror 

# ---- Files ----
SRCS := main.cpp server.cpp Irc.cpp handlers.cpp Client.cpp
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

# ---- Default rule ----
all: $(NAME)

# ---- Link ----
$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

# ---- Compile ----
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# ---- Cleaning ----
clean:
	rm -f $(OBJS) $(DEPS)

fclean: clean
	rm -f $(NAME)

re: fclean all

# ---- Phony ----
.PHONY: all clean fclean re

# ---- Include generated dependencies ----
-include $(DEPS)