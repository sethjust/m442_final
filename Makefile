CC = gcc
CFLAGS = -ggdb -c -Wall -Wextra -std=gnu99
LFLAGS = -ggdb -Wall -Wextra -std=gnu99 -lsqlite3 -pthread

all: server

test: LFLAGS += -include printing.h
test: clean sql.c sql.h obj.c obj.h network.c network.h server.c server.h
	$(CC) $(LFLAGS) -o server server.c network.c obj.c sql.c

sql.o: sql.c sql.h obj.h
	$(CC) $(CFLAGS) -o sql.o sql.c

obj.o: obj.c obj.h
	$(CC) $(CFLAGS) -o obj.o obj.c

network.o: network.c network.h
	$(CC) $(CFLAGS) -o network.o network.c

server: server.c server.h network.o obj.o sql.o
	$(CC) $(LFLAGS) -o server server.c network.o obj.o sql.o

clean:
	-rm server obj.o network.o sql.o client.pyc
