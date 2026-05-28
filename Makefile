# Compiler
CXX = g++

# Flags
CXXFLAGS = -std=c++17 -Wall -Wextra -g -fsanitize=address

# Executables
SERVER_TARGET = server
CLIENT_TARGET = client

# Source files
SERVER_SRCS = server_main.cpp server.cpp orderbook.cpp
CLIENT_SRCS = client_main.cpp client.cpp orderbook.cpp

# Object files
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

# Default target
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Server executable
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(SERVER_TARGET) $(SERVER_OBJS)

# Client executable
$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_TARGET) $(CLIENT_OBJS)

# Compile source -> object
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f *.o $(SERVER_TARGET) $(CLIENT_TARGET)

# Rebuild
rebuild: clean all

.PHONY: all clean rebuild