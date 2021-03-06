/*
    Authors: Reno Valdes, Hazza Alkaabi
    Date: September 28, 2018
    Instructor: Patrick Homer
    Computer Science 425: Principles of Networking
    University of Arizona

    Project: Program 2: Mobile TCP Proxy - Milestone 2
    File name: serverProxy.c
    Description: This program (the Server Proxy) will open two connections via
   sockets, one with the clientProxy, and the other with telnet daemon. This
   program will relay telnet commands from the clientProxy to the telnet daemon
   which in turn will write back to the serverProxy the results of the telnet
   commands it received. The serverProxy will write back to the clientProxy
   which in turn will write to telnet.

    The command line argument for the client is as follow:
    ./serverProxy port_number
    where:
    port_number is the port number that the serverProxy will be listneing to.

    Note: A Makefile is provided.
    Run "make" or "make all" in the command line in the directory where Makefile
   are located. The Makefile will clean first, then compile both of the client
   and server source files.
*/

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void connectToSockets(int port);
void relayLoop(int tel_desc, int client_desc);
int readHeartBeat(int client_desc);
void sendHeartBeat(int clientID, int client_desc);

/*
    connectToSockets

    params: int port : the port to bind to

    return: None

    This function binds, and listen to the clientProxy socket. It will then
    block at accept until there is communcation with clientProxy. It will
    then create a socket for a connection with telnet on localhost.
*/
void connectToSockets(int port) {
  static int tel_desc, client_desc;

  // Begining of socket, bind, and listen to client on server

  client_desc = socket(PF_INET, SOCK_STREAM, 6);
  if (client_desc <= 0) {
    fprintf(stderr, "serverProxy ERROR: failed to create socket.\n");
    exit(1);
  }

  struct sockaddr_in sin;

  sin.sin_family = PF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = INADDR_ANY; // Accepting connections from any sources
  int result = bind(client_desc, (struct sockaddr *)&sin, sizeof(sin));
  if (result < 0) {
    fprintf(stderr, "serverProxy ERROR: failed to bind to port %d\n", port);
    close(client_desc);
    exit(1);
  }

  if (listen(client_desc, 3) < 0) {
    fprintf(stderr, "serverProxy ERROR: failed to start listening on port %d\n",
            port);
    exit(1);
  }

  int cli_len = sizeof(client_desc);
  static int new_socket;
  if ((new_socket = accept(client_desc, (struct sockaddr *)&client_desc,
                           (socklen_t *)&cli_len)) < 0) {
    fprintf(stderr, "serverProxy ERROR: failed to accept connection\n");
    close(client_desc);
    exit(1);
  }
  close(client_desc);

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
  if (address == -1) {
    fprintf(
        stderr,
        "serverProxy ERROR: inet_addr failed to convert telnetAddress '%s'\n",
        telnetAddress);
    close(new_socket);
    close(tel_desc);
    exit(1);
  }
  memcpy(&telnet_sin.sin_addr, &address, sizeof(address));

  if (connect(tel_desc, (struct sockaddr *)&telnet_sin, sizeof(telnet_sin)) <
      0) {
    fprintf(stderr,
            "serverProxy ERROR: can't connect to %s.%d - Connection refused\n",
            telnetAddress, telnetPort);
    close(new_socket);
    close(tel_desc);
    exit(1);
  }

  // End of socket, and connect of telnet daemon on server

  // This function will run select on both sockets of telnet and clientProxy
  relayLoop(tel_desc, new_socket);

  close(tel_desc);
  close(new_socket);
}

/*
    relayLoop

    params: int tel_desc: telnet socket file descriptor
            int client_desc: clientProxy socket file descriptor

    return: None

    This function selects between the telnet socket and clientProxy socket. It
   will read from clientProxy to then write the telnet daemon. It will read from
   telnet daemon to write the clientProxy.
*/
void relayLoop(int tel_desc, int client_desc) {
  int MAXFD = 0;

  if (tel_desc > client_desc) {
    MAXFD = tel_desc;
  } else {
    MAXFD = client_desc;
  }

  fd_set listen;
  struct timeval timeout; /* timeout for select call */
  int nfound;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  int BUFLEN = 1024;
  char buf[BUFLEN];
  char clientBuf[BUFLEN];
  int n;
  int missingHeartBeat = 0;
  int size;
  char type;
  int clientID = -1;
  int newID;

  while (1) {
    if (missingHeartBeat == 3) {
      // Need to reconnect
      // TODO: We need to close disconnected socket(s) here
      // TODO: Add a one-second timeout to the call to select in each proxy
      printf("Need to open a new connection!\n");
      exit(1); // TODO: We do not exit. This code is temporarily.
    }

    if (clientID != -1) {
      sendHeartBeat(clientID, client_desc);
    }

    FD_ZERO(&listen);
    FD_SET(tel_desc, &listen);
    FD_SET(client_desc, &listen);
    nfound = select(MAXFD + 1, &listen, (fd_set *)0, (fd_set *)0, &timeout);
    printf("nfound = %d\n", nfound);
    if (nfound == 0) {
      printf("timed out\n");
      missingHeartBeat++;
      continue;
    } else if (nfound < 0) {
      fprintf(stderr, "ERROR: failed on select call\n.");
      close(tel_desc);
      close(client_desc);
      exit(1);
    }

    if (FD_ISSET(tel_desc, &listen)) {
      printf("tel_desc set\n");
      /*
      n = read(tel_desc, &buf, BUFLEN);

      if (n <= 0)
      {
          fprintf(stderr, "telnet daemon closed connection.\n");
          close(tel_desc);
          close(client_desc);
          exit(0);
      }

      // write the user text to the server
      n = write(client_desc, buf, n);

      if (n < 0)
      {
          fprintf(stderr, "serverProxy ERROR: Failed to write to the
      clientProxy...\n"); close(tel_desc); close(client_desc); exit(1);
      }
      */
    }

    if (FD_ISSET(client_desc, &listen)) {
      n = read(client_desc, &type, 1);
      printf("type = %c\n", type);

      if (n <= 0) {
        fprintf(stderr, "clientProxy closed connection.\n");
        close(tel_desc);
        close(client_desc);
        exit(0);
      }

      if (type == 'h') {
        missingHeartBeat = 0;
        if ((newID = readHeartBeat(client_desc)) != clientID) {
          if (clientID == -1) {
            clientID = newID;
          }
          // Else it is a new connection
          else {
              // We have already closed the now-disconnected sockets in a previous if check
          }
        }
      }

      /*
      n = read(client_desc, &clientBuf, BUFLEN);

      if (n <= 0)
      {
          fprintf(stderr, "clientProxy closed connection.\n");
          close(tel_desc);
          close(client_desc);
          exit(0);
      }

      int result = write(tel_desc, &clientBuf, n);
      if (result < 0)
      {
          fprintf(stderr, "serverProxy ERROR: Failed to write the clientProxy
      text to the serverProxy...\n"); close(tel_desc); close(client_desc);
          exit(1);
      }
      */
    }
  }
}

void sendHeartBeat(int clientID, int client_desc) {
  int n;
  char type = 'h';
  // write the message type
  n = write(client_desc, &type, 1);
  if (n < 0) {
    fprintf(stderr, "ERROR: Failed to write to server\n");
    close(client_desc); // TODO change to close all sockets
    exit(1);
  }

  int size = htonl(sizeof(clientID));

  // write the size
  n = write(client_desc, &size, sizeof(size));
  if (n < 0) {
    fprintf(stderr, "ERROR: Failed to write to server\n");
    close(client_desc); // TODO change to close all sockets
    exit(1);
  }

  clientID = htonl(clientID);
  n = write(client_desc, &clientID, sizeof(clientID));
  if (n < 0) {
    fprintf(stderr, "ERROR: Failed to write to server\n");
    close(client_desc); // TODO change to close all sockets
    exit(1);
  }

  printf("sent heart beat\n");
}

int readHeartBeat(int client_desc) {

  int n;
  int size;
  int newID;

  n = read(client_desc, &size, sizeof(size));

  printf("n = %d\n", n);
  if (n <= 0) {
    fprintf(stderr, "clientProxy closed connection.\n");
    close(client_desc);
    exit(0);
  }
  size = ntohl(size);
  printf("size = %d\n", size);

  n = read(client_desc, &newID, size);

  if (n <= 0) {
    fprintf(stderr, "clientProxy closed connection.\n");
    close(client_desc);
    exit(0);
  }

  newID = ntohl(newID);
  printf("ID = %d\n", newID);

  printf("recieved heart beat\n");

  return newID;
}

int main(int argc, const char *argv[]) {
  int port = atoi(argv[1]);
  connectToSockets(port);

  return 0;
}