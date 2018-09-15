/*
    Authors: Reno Valdes, Hazza Alkaabi
    Date: September 14, 2018
    Instructor: Patrick Homer
    Computer Science 425: Principles of Networking
    University of Arizona

    Project: Program 2: Mobile TCP Proxy - Milestone 1
    File name: server.c
    Description: This program (the server) will bind to a port passed as a command line
                 argument and wait for a connection from the client. Once the client connects
                 the server reads twice, the first to get the size of the message and
                 the second time to read the message itself. Both the size and message
                 are then printed to stdout. The server will continue reading messages until
                 the client closes the connection.
    
    The command line argument for the client is as follow:
    ./server port_number
    where:
    port_number is the port number that the server will be listneing to.

    Note: A Makefile is provided. 
    Run "make" or "make all" in the command line in the directory where Makefile are located.
    The Makefile will clean first, then compile both of the client and server source files.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

int readFromClient(int sock_desc);

/*
    acceptConnection

    params: int port : the port to bind to

    return: None
    
    This function takes a port number and binds to it then
    listens for a client to make a connection to this port.
    Once a client connects, an infinite loop is started
    that reads from the client. Only when the client disconnects will
    this function return.
*/
void acceptConnection(int port){
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
        close(sock_desc);
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
        close(sock_desc);
        exit(1); 
    } 

    int rv;
    while((rv = readFromClient(new_socket)) > 0) // Strange syntax, but essentialy keep calling this function
                                                 // until it returns -1
    
    close(sock_desc);
}


/*
    readFromClient

    params: int new_socket : the socket description to read from

    return: None
    
    This function takes a socket description and reads
    a single size/message from the client. Once the server
    has read the size/message these two values are printed to stdout.
*/
int readFromClient(int new_socket){
    int size;
    int n;
    n = read(new_socket , &size, 4);
    
    if(n <=0){
        fprintf(stderr,"Client closed connection.\n");
        return -1;
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
    printf("%s\n\n",buf);
    return 1;
}

int main( int argc, const char* argv[] ){

    int port = atoi(argv[1]);
    acceptConnection(port);

    return 0;
}