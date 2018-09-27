/*
    Authors: Reno Valdes, Hazza Alkaabi
    Date: September 14, 2018
    Instructor: Patrick Homer
    Computer Science 425: Principles of Networking
    University of Arizona

    Project: Program 2: Mobile TCP Proxy - Milestone 2
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
#include <arpa/inet.h>
#include <string.h>

int readFromClient(int client_desc);
void queryLoop(int tel_desc, int client_desc);

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
void acceptConnection(int port)
{
    static int tel_desc, client_desc;

    // Begining of socket, bind, and listen to client on server

    client_desc = socket(PF_INET, SOCK_STREAM, 6);
    if (client_desc <= 0)
    {
        fprintf(stderr, "serverProxy ERROR: failed to create socket.\n");
        exit(1);
    }

    struct sockaddr_in sin;

    sin.sin_family = PF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY; // Accepting connections from any sources
    int result = bind(client_desc, (struct sockaddr *)&sin, sizeof(sin));
    if (result < 0)
    {
        fprintf(stderr, "serverProxy ERROR: failed to bind to port %d\n", port);
        close(client_desc);
        exit(1);
    }

    if (listen(client_desc, 3) < 0)
    {
        fprintf(stderr, "serverProxy ERROR: failed to start listening on port %d\n", port);
        exit(1);
    }

    int cli_len = sizeof(client_desc);
    static int new_socket;
    if ((new_socket = accept(client_desc, (struct sockaddr *)&client_desc, (socklen_t *)&cli_len)) < 0)
    {
        fprintf(stderr, "serverProxy ERROR: failed to accept connection\n");
        close(client_desc);
        exit(1);
    }

    //printf("DEBUG serverProxy: Accepted telnet connection\n");

    // End of socket, bind, and listen to client on server

    // Let's connect to the telnet daemon on localhost
    // address 127.0.0.1, port 23
    char *telnetAddress = "127.0.0.1";
    int telnetPort = 23;

    // Connect to Server
    tel_desc = socket(PF_INET, SOCK_STREAM, 6);
    struct sockaddr_in telnet_sin;

    telnet_sin.sin_family = PF_INET;
    telnet_sin.sin_port = htons(telnetPort);
    in_addr_t address;
    // Convert ip
    address = inet_addr(telnetAddress);
    if (address == -1)
    {
        fprintf(stderr, "serverProxy ERROR: inet_addr failed to convert telnetAddress '%s'\n", telnetAddress);
        exit(1);
    }
    memcpy(&telnet_sin.sin_addr, &address, sizeof(address));

    if (connect(tel_desc, (struct sockaddr *)&telnet_sin, sizeof(telnet_sin)) < 0)
    {
        fprintf(stderr, "serverProxy ERROR: can't connect to %s.%d - Connection refused\n", telnetAddress, telnetPort);
        exit(1);
    }

    // End of socket, and connect of telnet daemon on server

    // This function will run select than accept a connection
    queryLoop(tel_desc, new_socket);

    // TODO: close both socket now?
    close(tel_desc);
    close(new_socket);
}

void queryLoop(int tel_desc, int client_desc)
{
    int MAXFD = 0;

    if (tel_desc > client_desc)
    {
        MAXFD = tel_desc;
    }
    else
    {
        MAXFD = client_desc;
    }

    fd_set listen;
    struct timeval timeout; /* timeout for select call */
    int nfound;
    timeout.tv_sec = 240;
    timeout.tv_usec = 0;
    int BUFLEN = 1024;
    char buf[BUFLEN];
    char clientBuf[BUFLEN];
    int n;

    while (1)
    {
        FD_ZERO(&listen);
        FD_SET(tel_desc, &listen);
        FD_SET(client_desc, &listen);
        //printf("DEBUG on serverProxy selecting...\n");
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
            //printf("FD_ISSET is telnet DEBUG on serverProxy: Recieved data from telnet daemon\n");

            n = read(tel_desc, &buf, BUFLEN);

            if (n <= 0)
            {
                fprintf(stderr, "telnet daemon closed connection.\n");
                exit(0);
            }
            //printf("DEBUG FD_ISSET is telnet: n=%d\n", n);
            //buf[n] = '\0';
            //printf("DEBUG FD_ISSET is telnet: %s\n", buf);

            //int num;

            // change num to to a network value to send it
            //num = htonl(n);

            // write the size of the buffer
            //int result = write(client_desc, &num, sizeof(num));
            //if (result < 0)
            //{
            //    fprintf(stderr, "serverProxy ERROR: Failed to write to the clientProxy...\n");
            //    exit(1);
            //}

            // write the user text to the server
            n = write(client_desc, buf, n);

            if (n < 0)
            {
                fprintf(stderr, "serverProxy ERROR: Failed to write to the clientProxy...\n");
                exit(1);
            }

            //buf[n] = '\0'; // Commented out cause it should already be null terminated
            //printf("DEBUG FD_ISSET is telnet: %s\n", buf);
        }
        if (FD_ISSET(client_desc, &listen))
        {
            //printf("FD_ISSET is client DEBUG on serverProxy: Recieved data from the client\n");

            //int telnetTextSize = 0;
            //n = read(client_desc, &telnetTextSize, sizeof(telnetTextSize));

            //if (n <= 0)
            //{
            //    fprintf(stderr, "clientProxy closed connection at size read.\n");
            //    exit(0);
            //}

            //telnetTextSize = ntohl(telnetTextSize);
            //printf("FD_ISSET is client DEBUG: n=%d\n", n);

            n = read(client_desc, &clientBuf, BUFLEN);

            if (n <= 0)
            {
                fprintf(stderr, "clientProxy closed connection.\n");
                exit(0);
            }
            
            //printf("FD_ISSET is client DEBUG: %s\n", clientBuf);

            int result = write(tel_desc, &clientBuf, n);
            if (result < 0)
            {
                fprintf(stderr, "serverProxy ERROR: Failed to write the clientProxy text to the serverProxy...\n");
                exit(1);
            }
        }
    }
}

int main(int argc, const char *argv[])
{
    int port = atoi(argv[1]);
    acceptConnection(port);

    return 0;
}