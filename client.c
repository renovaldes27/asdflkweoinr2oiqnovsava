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


// connectToSock
// params: int port : the port to connect to
//         string hostName : the host to connect to
// return: int sock_desc
// This function takes a port number and host name
// and connects to a socket at that location.
int connectToSock(int port, const char* ipString){
    int sock_desc;
    sock_desc = socket(PF_INET, SOCK_STREAM, 6);

    struct sockaddr_in sin;

    sin.sin_family = PF_INET;
    sin.sin_port = htons(port);
    in_addr_t address;
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

void queryServer(int sock_desc){  
    while(1){
        size_t BUFLEN = 1024;
        char* buf = NULL;
        int n;
        n = getline(&buf,&BUFLEN,stdin);
        
        if (n <=0){
            return;
        }

        if (n > 1025){ // 1025 to account for newline character, so max char sent is 1024
            n = 1025;
        }

        int num = n-1;  // Don't write the newline character

        num = htonl(num);
        int result = write(sock_desc, &num, sizeof(num));
        if (result < 0){
            fprintf(stderr, "ERROR: Failed to write to server\n");
        }
        
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