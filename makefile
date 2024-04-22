CC := g++
CFLAGS := -std=c99 -Wall -Wextra
SRCS = main.cpp ArgumentsHandler.cpp server_classes.cpp TCPhandler.cpp UDPhandler.cpp
OBJS = main.o ArgumentsHandler.o server_classes.o TCPhandler.o UDPhandler.o
TARGET = ipk24chat-server

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)