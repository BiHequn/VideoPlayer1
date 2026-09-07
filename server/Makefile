CC = gcc
CFLAGS = -Wall -O2 -I./include
LDFLAGS = -ljson-c -lsqlite3 -lssl -lcrypto -lpthread -lm

SRCS = src/main.c src/epoll_server.c
OBJS = $(SRCS:.c=.o)
TARGET = videoserver

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	mkdir -p data/uploads/files data/uploads/gif data/uploads/chunks
	./$(TARGET) 0.0.0.0 8888

.PHONY: all clean run
