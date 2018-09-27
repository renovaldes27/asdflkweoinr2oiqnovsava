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
#include <sys/time.h>

void queryLoop(int tel_desc, int server_desc);

/*
    connectToSock

    params: int port : the port to connect to
            string hostName : the host to connect to
    return: int sock_desc
    
    This function takes a port number and an ip address
    and connects to a socket at that location.
*/
void connectToSockets(int telPort, int serverPort, const char *ipString)
{
    static int tel_desc, server_desc;

    tel_desc = socket(PF_INET, SOCK_STREAM, 6);
    if (tel_desc <= 0)
    {
        fprintf(stderr, "ERROR: failed to create socket\n.");
        exit(1);
    }

    struct sockaddr_in sin;

    sin.sin_family = PF_INET;
    sin.sin_port = htons(telPort);
    // TODO: It really should be just localhost since telnet is run local. So not any address
    sin.sin_addr.s_addr = INADDR_ANY; // Accepting connections from any sources
    int result = bind(tel_desc, (struct sockaddr *)&sin, sizeof(sin));
    if (result < 0)
    {
        fprintf(stderr, "ERROR: failed to bind to port %d\n.", telPort);
        close(tel_desc);
        exit(1);
    }

    if (listen(tel_desc, 1) < 0)
    {
        fprintf(stderr, "ERROR: failed to start listening on port %d\n", telPort);
        exit(1);
    }

    int cli_len = sizeof(tel_desc);
    static int new_socket;
    if ((new_socket = accept(tel_desc, (struct sockaddr *)&tel_desc, (socklen_t *)&cli_len)) < 0)
    {
        fprintf(stderr, "ERROR: failed to accept connection\n");
        close(tel_desc);
        exit(1);
    }

    //printf("Accepted telnet connection\n");

    // Connect to Server
    server_desc = socket(PF_INET, SOCK_STREAM, 6);
    struct sockaddr_in sin2;

    sin2.sin_family = PF_INET;
    sin2.sin_port = htons(serverPort);
    in_addr_t address;
    // Convert ip
    address = inet_addr(ipString);
    if (address == -1)
    {
        fprintf(stderr, "ERROR: inet_addr failed to convert ipString '%s'\n", ipString);
        exit(1);
    }
    memcpy(&sin2.sin_addr, &address, sizeof(address));

    if (connect(server_desc, (struct sockaddr *)&sin2, sizeof(sin2)) < 0)
    {
        fprintf(stderr, "ERROR: can't connect to %s.%d - Connection refused\n", ipString, serverPort);
        exit(1);
    }

    queryLoop(new_socket, server_desc);
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
void queryLoop(int tel_desc, int server_desc)
{
    int MAXFD = 0;

    if (tel_desc > server_desc)
    {
        MAXFD = tel_desc;
    }
    else
    {
        MAXFD = server_desc;
    }

    fd_set listen;
    struct timeval timeout; /* timeout for select call */
    int nfound;
    int size;
    timeout.tv_sec = 240;
    timeout.tv_usec = 0;
    int BUFLEN = 1024;
    char buf[BUFLEN];
    int n;

    while (1)
    {
        FD_ZERO(&listen);
        FD_SET(tel_desc, &listen);
        FD_SET(server_desc, &listen);
        //printf("DEBUG on clientProxy selecting...\n");
        nfound = select(MAXFD + 1, &listen, (fd_set *)0, (fd_set *)0, &timeout);
        if (nfound == 0)
        {
            fprintf(stderr, "ERROR: select call timed out\n");
            exit(1);
        }
        else if (nfound < 0)
        {
            /* handle error here... */
        }

        if (FD_ISSET(tel_desc, &listen))
        {
            //printf("Recieved data from telnet\n");

            n = read(tel_desc, &buf, BUFLEN);

            if (n <= 0)
            {
                fprintf(stderr, "Client closed connection.\n");
                exit(0);
            }
           //printf("n=%d", n);

            int num;

            // change num to to a network value to send it
            num = htonl(n);

            // write the size of the buffer
            int result = write(server_desc, &num, sizeof(num));
            if (result < 0)
            {
                fprintf(stderr, "ERROR: Failed to write to server\n");
                exit(1);
            }

            // write the user text to the server
            result = write(server_desc, buf, n);

            if (result < 0)
            {
                fprintf(stderr, "ERROR: Failed to write to server\n");
                exit(1);
            }

            //buf[n] = '\0';
            //printf("%s\n", buf);
        }

        if (FD_ISSET(server_desc, &listen))
        {
            //printf("Recieved data from server\n");

            //n = read(server_desc, &size, sizeof(size));

            //if (n <= 0)
           // {
            //    fprintf(stderr, "clientProxy closed connection at size read.\n");
            //    exit(0);
           // }

            //size = ntohl(size);
            //printf("DEBUG: size = %d\n", size);
            n = read(server_desc, &buf, BUFLEN);
            //printf("DEBUG: AFTER READ %d\n",n);

            //printf("DEBUG: AFTER READ %s\n", buf);

            /*
            int i;
            for (i = 0; i < n; i++){
                printf("'%c'", buf[i]);
            }
            printf("\n");
            for (i = 0; i < n; i++){
                printf("'%x'", buf[i]);
            }
            printf("\n");*/

            if (n <= 0)
            {
                fprintf(stderr, "Client closed connection.\n");
                exit(0);
            }

            int result = write(tel_desc, buf, n);

            if (result < 0)
            {
                fprintf(stderr, "ERROR: Failed to write to telnet\n");
                exit(1);
            }

            //buf[n] = '\0';
            //printf("%s\n", buf);
        }
    }
}

int main(int argc, const char *argv[])
{
    if (argc > 4)
    {
        fprintf(stderr, "ERROR: Too many cmd-line arguments\n");
        exit(1);
    }

    int port1 = atoi(argv[1]);
    int port2 = atoi(argv[3]);

    connectToSockets(port1, port2, argv[2]);

    //queryServer(sock_desc);
    //close(sock_desc);
    return 0;
}