/*
    Authors: Reno Valdes, Hazza Alkaabi
    Date: September 14, 2018
    Instructor: Patrick Homer
    Computer Science 425: Principles of Networking
    University of Arizona

    Project: Program 2: Mobile TCP Proxy - Milestone 1
    File name: client.c
    Description: This program (the client) will write the server user input from stdin.
                 The server in turn will print the client message to its stdout. The client
                 is limited to 1024 charcaters. The message will be sent without the null
                 terminator and the newline characters.
    
    The command line argument for the client is as follow:
    ./client server_ip_address port_number
    where:
    server_ip_address is the ip address of the server.  
    port_number is the port number that the server will be listneing to.

    Note: A Makefile is provided. 
    Run "make" or "make all" in the command line in the directory where Makefile are located.
    The Makefile will clean first, then compile both of the client and server source files.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/*
    connectToSock

    params: int port : the port to connect to
            string hostName : the host to connect to
    return: int sock_desc
    
    This function takes a port number and an ip address
    and connects to a socket at that location.
*/
int connectToSock(int port, const char* ipString){
    int sock_desc;
    sock_desc = socket(PF_INET, SOCK_STREAM, 6);

    struct sockaddr_in sin;

    sin.sin_family = PF_INET;
    sin.sin_port = htons(port);
    in_addr_t address;
    // Convert ip
    address = inet_addr(ipString);
    if ( address == -1 ) {
         fprintf(stderr, "ERROR: inet_addr failed to convert ipString '%s'\n", ipString);
         exit(1);
    }
    memcpy(&sin.sin_addr, &address, sizeof(address));

    if (connect(sock_desc, (struct sockaddr *)&sin, sizeof(sin)) < 0){
        fprintf(stderr,"ERROR: can't connect to %s.%d - Connection refused\n", ipString, port);
        exit(1);
    } 

    return sock_desc;
}

/*
    queryServer

    params: int sock_desc : the socket to connect to

    return: int sock_desc
    
    This function waits for user input from stdin
    to send it to the server. It is maxed at 1024,
    that is without null terminator and newline
    characters.
*/
void queryServer(int sock_desc){  
    while(1){
        size_t BUFLEN = 1024;
        char* buf = NULL;
        int n;
        n = getline(&buf,&BUFLEN,stdin);
        
        if (n <=0){
            return;
        }

        // 1025 to account for newline character, so max char sent is 1024
        if (n > 1025){
            n = 1025;
        }

        // Don't write the newline character
        int num = n-1;

        // change num to to a network value to send it
        num = htonl(num);

        // write the size of the buffer
        int result = write(sock_desc, &num, sizeof(num));
        if (result < 0){
            fprintf(stderr, "ERROR: Failed to write to server\n");
        }

        // write the user text to the server
        result = write(sock_desc, buf, n-1);

        if (result < 0){
            fprintf(stderr, "ERROR: Failed to write to server\n");
        }
    }
    close(sock_desc);
}

int main( int argc, const char* argv[] )
{
    if (argc > 3){
        fprintf(stderr,"ERROR: Too many cmd-line arguments\n");
        exit(1);
    }
    
    int port = atoi(argv[2]);
    int sock_desc = connectToSock(port,argv[1]);

    queryServer(sock_desc);
    close(sock_desc);
}