#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// connectToSock
// params: int port : the port to connect to
//         string hostName : the host to connect to
// return: int sock_desc
// This function takes a port number and host name
// and connects to a socket at that location.
int connectToSock(int port, char* ipString){
    int sock_desc;
    sock_desc = socket(PF_INET, SOCK_STREAM, 6);

    struct sockaddr_in sin;

    sin.sin_family = PF_INET;
    sin.sin_port = htons(port);
    inet_pton(AF_INET, &ipString, &(sin.sin_addr));

    if (connect(sock_desc, (struct sockaddr *)&sin, sizeof(sin)) < 0){
        fprintf(stderr,"can't connect to %s.%d : Connection refused\n", ipString, port);
        exit(1);
    } 

    return sock_desc;
}

void queryServer(int sock_desc){  
    while(1){
        int BUFLEN = 1024;
        char* buf;
        int n;
        buf = (char *) malloc (BUFLEN + 1);
        n = getline(&buf,&BUFLEN,stdin);
        if (n <=0){
            return;
        }
        buf[n] = "\0";


        printf(buf);
        int32_t num = htonl(n);
        char *data = (char*)&num;
        int result = write(sock_desc, data, sizeof(num));
        if (result < 0){
            fprintf(stderr, "ERROR: Failed to write to server\n");
        }

        result = write(sock_desc, &buf, n);
        if (result < 0){
            fprintf(stderr, "ERROR: Failed to write to server\n");
        }
        free(buf);
    }
}

int main( int argc, const char* argv[] )
{
    if (argc > 3){
        fprintf(stderr,"Too many cmd-line arguments\n");
    }

    // Connect to socket
    int address = argv[1];
    int port = atoi(argv[2]);
    int sock_desc = connectToSock(port,address);

    queryServer(sock_desc);
    close(sock_desc);
}