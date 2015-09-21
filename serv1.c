#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


// The TCP-server with an iterative structure.
int main(int argc, char **argv) {
    int socketfd, newsockfd, err, res;
    struct sockaddr_in addr, client_addr;
    // struct hostent *resolv; 
    socklen_t addrlen;
    int client_count = 0;


    socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd < 0) {
        perror("Error creating socket!");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(4444);
    // addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    err = bind(socketfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
    if (err < 0) {
        perror("Error when binding socket!");
        close(socketfd);
        exit(1);
    }

    res = listen(socketfd, 5); // Backlog often set to 5, hence the magic number
    if (res < 0) {
        perror("Error in listen!");
        close(socketfd);
        exit(1);

    }
    addrlen = sizeof(struct sockaddr_in);

    while (1) {
        newsockfd = accept(socketfd, (struct sockaddr *) &client_addr, &addrlen);

        if (newsockfd < 0) {
            perror("newsockfd accept error");
        }
        else {
            client_count++;
            printf("Connection from %s \n", inet_ntoa(client_addr.sin_addr));
            write(newsockfd, &client_count, sizeof(client_count));
            close(newsockfd);

        }
        // break;
    }

    close(socketfd);
}