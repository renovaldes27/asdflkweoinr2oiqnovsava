# CSC425 Program 2 Milestone 2 Makefile

all: clean serverProxy clientProxy

server: serverProxy.c
	gcc -g -Wall serverProxy.c -o serverProxy

client: clientProxy.c
	gcc -g -Wall clientProxy.c -o clientProxy

clean:
	rm -f *.o
	rm -f serverProxy
	rm -f clientProxy