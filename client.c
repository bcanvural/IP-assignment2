#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT_NUM 4444

int main(int argc, char **argv) {
    int socketfd;
    struct hostent *h;
    struct in_addr *addr;
    struct sockaddr_in serv_addr;
    uint32_t recv_count;

    if (argc != 2) {
        printf("Syntax: %s <hostname>\n", argv[0]);
        exit(-1);
    }

    h = gethostbyname(argv[1]);

    if (h == NULL) {
        fprintf(stderr, "Hostname \"%s\" could not be resolved\n", argv[1]);
        exit(-2);
    }

    addr = (struct in_addr*) h->h_addr_list[0];

    socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd < 0) {
        perror("Error when creating socket!");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT_NUM);
    serv_addr.sin_addr = *addr;

    if (connect(socketfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to server!");
        close(socketfd);
        exit(1);
    }

    // printf("Connected!!!\n");
    if (read(socketfd, &recv_count, sizeof(recv_count)) < 0) {
        perror("read error");
    }
    else {
        printf("I received: %d\n", ntohl(recv_count));
    }

    close(socketfd);



    return 0;
}