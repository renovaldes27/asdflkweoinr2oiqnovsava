/*
    Authors: Reno Valdes, Hazza Alkaabi
    Date: September 28, 2018
    Instructor: Patrick Homer
    Computer Science 425: Principles of Networking
    University of Arizona

    Project: Program 2: Mobile TCP Proxy - Milestone 2
    File name: clientProxy.c
    Description: This program (the clientProxy) will open two socket connections,
                 one to the telnet application on the same machine as the client
                 and one to a remote serverProxy. After opening both connections
                 the clientProxy will simply relay data read from one socket to the other.

    
    The command line argument for the client is as follow:
    ./clientProxy telnet_port_number server_ip_address server_port_number
    where:
    telnet_port_number is the port that telnet should connect to when connecting to the clientProxy
    server_ip_address is the ip address of the server.  
    server_port_number is the port number that the server will be listneing to.

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
int readHeartBeat(int client_desc);
void sendHeartBeat(int client_ID, int server_desc);


/*
    connectToSockets

    params: int telPort : the port to listen on for the telnet connection
            int serverPort : the server port to connect to 
            string ipString : the IP address of the serverProxy machine to connect to
    return: None
    
    This function opens the two socket connections necesarry for relaying data.
    First it will make a passive connect call to wait for the incoming telnet connecttion.
    Then it will make an active connect call to connect to the serverProxy.
    Once connected, the program goes into an infinite query loop.
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
        close(tel_desc);
        exit(1);
    }
    memcpy(&sin2.sin_addr, &address, sizeof(address));

    if (connect(server_desc, (struct sockaddr *)&sin2, sizeof(sin2)) < 0)
    {
        fprintf(stderr, "ERROR: can't connect to %s.%d - Connection refused\n", ipString, serverPort);
        close(tel_desc);
        exit(1);
    }

    queryLoop(new_socket, server_desc);
}

/*
    queryLoop

    params: int tel_desc : the telnet socket
            int server_desc : the serverProxy socket

    return: None
    
    This function uses select() calls to check if the telnet socket
    or serverProxy socket have data. Whenever one of the sockets
    has data to read, this data is read into a buffer and written
    to the othe socket. It simply relays messages between the two sockets.
*/
void queryLoop(int tel_desc, int server_desc)
{
    int MAXFD = 0;

    if (tel_desc > server_desc) {
        MAXFD = tel_desc;
    }
    else {
        MAXFD = server_desc;
    }

    fd_set listen;
    struct timeval timeout; /* timeout for select call */
    int nfound;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    int BUFLEN = 1024;
    char buf[BUFLEN];
    int n;
    int missingHeartBeat = 0;
    int clientID = rand();
    int newID;
    char type;

    while (1) {
        if (missingHeartBeat == 3){
            // Need to reconnect
            // TODO: We need to close disconnected socket(s) here
            // TODO: Add a one-second timeout to the call to select in each proxy
            printf("Need to open a new connection!\n");
            exit(1);  // TODO: We do not exit. This code is temporarily. 
        }
        sendHeartBeat(clientID, server_desc);
        printf("ID = %d\n", clientID);

        FD_ZERO(&listen);
        FD_SET(tel_desc, &listen);
        FD_SET(server_desc, &listen);
        nfound = select(MAXFD + 1, &listen, (fd_set *)0, (fd_set *)0, &timeout);
        if (nfound == 0) {
            missingHeartBeat++;
            continue;
            /*
            fprintf(stderr, "ERROR: select call timed out.\n");
            close(server_desc);
            close(tel_desc);
            exit(1);
            */
        }
        else if (nfound < 0) {
            fprintf(stderr, "ERROR: failed on select call\n.");
            close(server_desc);
            close(tel_desc);
            exit(1);
        }
        /*
        // Recieved data from telnet application
        if (FD_ISSET(tel_desc, &listen))
        {
            n = read(tel_desc, &buf, BUFLEN);

            if (n == 0)
            {
                fprintf(stdout, "Telnet closed connection.\n");
                close(server_desc);
                close(tel_desc);
                exit(0);
            }
            
            else if (n < 0)
            {
                fprintf(stderr, "Failed to read from telnet socket.\n");
                close(server_desc);
                close(tel_desc);
                exit(1);
            }

            // write the user text to the server
            n = write(server_desc, buf, n);

            if (n < 0)
            {
                fprintf(stderr, "ERROR: Failed to write to server\n");
                close(server_desc);
                close(tel_desc);
                exit(1);
            }
        }
        */
        // Recieved data from serverProxy
        if (FD_ISSET(server_desc, &listen))
        {

            n = read(server_desc, &type, 1);
            printf("type = %c\n", type);

            if (n <= 0) {
                fprintf(stderr, "clientProxy closed connection.\n");
                close(tel_desc);
                close(server_desc);
                exit(0);
            }
            
            if (type == 'h'){
                missingHeartBeat = 0;
                if((newID = readHeartBeat(server_desc)) != clientID){
                    if(clientID == -1){
                        clientID = newID;
                    }

                    //Else it is a new connection
                }
            }

            /*
            n = read(server_desc, &buf, BUFLEN);

            if (n == 0)
            {
                fprintf(stdout, "Server Proxy closed connection.\n");
                close(server_desc);
                close(tel_desc);
                exit(0);
            }

            else if (n < 0)
            {
                fprintf(stderr, "Failed to read from serverProxy socket.\n");
                close(server_desc);
                close(tel_desc);
                exit(1);
            }

            int result = write(tel_desc, buf, n);

            if (result < 0)
            {
                fprintf(stderr, "ERROR: Failed to write to telnet\n");
                close(server_desc);
                close(tel_desc);
                exit(1);
            }
            */
        }
        
    }
    
}

void sendHeartBeat(int clientID, int server_desc){ 
    int n;
    char type = 'h';
    // write the message type
    n = write(server_desc, &type, 1);
    if (n < 0) {
        fprintf(stderr, "ERROR: Failed to write to server\n");
        close(server_desc); // TODO change to close all sockets
        exit(1);
    }

    int size = htonl(sizeof(clientID));

    // write the size
    n = write(server_desc, &size, sizeof(size));
    if (n < 0) {
        fprintf(stderr, "ERROR: Failed to write to server\n");
        close(server_desc); // TODO change to close all sockets
        exit(1);
    }

    clientID = htonl(clientID);
    n = write(server_desc, &clientID, sizeof(clientID));
    if (n < 0) {
        fprintf(stderr, "ERROR: Failed to write to server\n");
        close(server_desc); // TODO change to close all sockets
        exit(1);
    }

    printf("sent heart beat\n");
}

int readHeartBeat(int server_desc){

    int n;
    int size;
    int newID;

    n = read(server_desc, &size, sizeof(size));

    printf("n = %d\n", n);
    if (n <= 0)
    {
        fprintf(stderr, "clientProxy closed connection.\n");
        close(server_desc);
        exit(0);
    }
    size = ntohl(size);
    printf("size = %d\n", size);

    n = read(server_desc, &newID, size);

    if (n <= 0)
    {
        fprintf(stderr, "clientProxy closed connection.\n");
        close(server_desc);
        exit(0);
    }

    newID = ntohl(newID);
    printf("ID = %d\n", newID);

    printf("recieved heart beat\n");

    return newID;
}

int main(int argc, const char *argv[]) {
    if (argc > 4) {
        fprintf(stderr, "ERROR: Too many cmd-line arguments\n");
        exit(1);
    }

    int port1 = atoi(argv[1]);
    int port2 = atoi(argv[3]);

    connectToSockets(port1, port2, argv[2]);
    
    return 0;
}