# CSC425 Program 2 Makefile

all: clean server client

server: server.c
	gcc -g server.c -o server

client: client.c
	gcc -g client.c -o client

clean:
	rm -f *.o
	rm -f server
	rm -f client