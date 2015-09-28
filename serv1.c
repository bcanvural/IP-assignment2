#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT_NUM 4444

ssize_t writen(int fd, const void *vptr, size_t n) {
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;
    ptr = vptr;
    nleft = n;
    
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (errno == EINTR) {
                nwritten = 0; /* and call write() again */
            }
            else {
                return -1; /* error */
            }
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}


// The TCP-server with an iterative structure.
int main(int argc, char **argv) {
    int socketfd, newsockfd, err, res;
    struct sockaddr_in addr, client_addr;
    socklen_t addrlen;
    uint32_t client_count = 0;
    int reuseaddr = 1;
    ssize_t bytes_written;


    socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd < 0) {
        perror("Error creating socket!");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT_NUM);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

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
            uint32_t network_order = htonl(client_count);
            printf("Connection from %s \n", inet_ntoa(client_addr.sin_addr));
            bytes_written = writen(newsockfd, &network_order, sizeof(network_order));

            if (bytes_written < sizeof(network_order)) {
                perror("Not enough bytes are written!");
            }

            close(newsockfd);

        }
        
    }

    close(socketfd);
}