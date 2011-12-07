CC = gcc
CFLAGS = -ggdb -c -Wall -Wextra -std=gnu99
LFLAGS = -ggdb -lsqlite3

all: server

sql.o: sql.c sql.h obj.h
	$(CC) $(CFLAGS) -o sql.o sql.c

obj.o: obj.c obj.h
	$(CC) $(CFLAGS) -o obj.o obj.c

network.o: network.c network.h
	$(CC) $(CFLAGS) -o network.o network.c

server: server.c server.h network.o obj.o sql.o
	$(CC) $(LFLAGS) -o server server.c network.o obj.o sql.o

clean:
	-rm server obj.o network.o sql.o
