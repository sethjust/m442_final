all: server

obj.o: obj.c obj.h
	gcc -g -c -o obj.o obj.c

network.o: network.c network.h
	gcc -g -c -o network.o network.c

server: server.c server.h network.o obj.o
	gcc -g -o server server.c network.o obj.o

clean:
	rm server obj.o network.o
