#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

void readFromClient(int sock_desc);

int acceptConnection(int port){
    int sock_desc;
    sock_desc = socket(PF_INET, SOCK_STREAM, 6);
    if (sock_desc <= 0) {
        fprintf(stderr,"ERROR: failed to create socket\n.");
        exit(1);
    }

    struct sockaddr_in sin;

    sin.sin_family = PF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;  // Accepting connections from any sources
    int result = bind(sock_desc, (struct sockaddr *)&sin, sizeof(sin));
    if (result < 0) {
        fprintf(stderr,"ERROR: failed to bind to port %d\n.", port);
        exit(1);
    }

    if (listen(sock_desc, 3) < 0) { 
        fprintf(stderr,"ERROR: failed to start listening on port %d\n", port); 
        exit(1); 
    } 

    int cli_len = sizeof(sock_desc);
    int new_socket;
    if ((new_socket = accept(sock_desc, (struct sockaddr *)&sock_desc, (socklen_t*)&cli_len)) < 0) { 
        fprintf(stderr, "ERROR: failed to accept connection\n"); 
        exit(1); 
    } 

    while(1){
        readFromClient(new_socket);
    }

    close(sock_desc);
}


void readFromClient(int new_socket){
    int size;
    int n;
    n = read(new_socket , &size, 4);
    
    if(n <=0){
        fprintf(stderr,"ERROR: client closed connection.\n");
        exit(1);
    }

    size = ntohl(size);
    // Remove new line character from the actual length and print it
    printf("%d\n", size);

    char buf[size+1], *bufptr;

    bufptr = buf;
    int count = size;
    while(count > 0 && (n = read(new_socket, bufptr, count)) > 0){
        bufptr += n;
	    count -= n;	
    }

    buf[size] = '\0';
    printf("%s\n",buf);
}

int main( int argc, const char* argv[] ){

    int port = atoi(argv[1]);
    acceptConnection(port);
}