#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void readFromClient(int sock_desc);

int acceptConnection(int port){
    int sock_desc;
    sock_desc = socket(PF_INET, SOCK_STREAM, 6);
    if (sock_desc < 0) {
        fprintf(stderr,"Failed to create socket\n.");
        exit(1);
    }

    struct sockaddr_in sin;

    sin.sin_family = PF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;
    int result = bind(sock_desc, (struct sockaddr *)&sin, sizeof(sin));
    if (result < 0) {
        fprintf(stderr,"Failed to bind to port %d\n.", port);
        exit(1);
    }

    if (listen(sock_desc, 3) < 0) { 
        fprintf(stderr,"Failed to start listening on port %d\n", port); 
        exit(1); 
    } 

    printf("listening\n");
    int sinLen = sizeof(sin);
    int new_socket;
    if ((new_socket = accept(sock_desc, (struct sockaddr *)&sin,  (socklen_t*)&sinLen) < 0)) { 
        fprintf(stderr, "Failed to accept connection\n"); 
        exit(1); 
    } 

    printf("accepted connection\n");
    while(1){
        readFromClient(new_socket);
    }

    close(sock_desc);
}


void readFromClient(int new_socket){
    int size;
    int n;
    printf("reading\n");
    read(new_socket , &size, 4); 
    size = ntohl(size);

    char buf[size+1], *bufptr;

    printf("size=%d", size);
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