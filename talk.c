#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

ssize_t writen(int fd, const void *vptr, size_t n) {
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;
    ptr = vptr;
    nleft = n;
    
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0 ) {
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


int server() {
    int socketfd, newsockfd, err, res;
    struct sockaddr_in addr, client_addr;
    socklen_t addrlen;
    int reuseaddr = 1;


    socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd < 0) {
        perror("Error creating socket!");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5555);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

    err = bind(socketfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
    if (err < 0) {
        perror("Error when binding socket!");
        close(socketfd);    int server_mode;

        exit(1);
    }

    res = listen(socketfd, 5); // Backlog often set to 5, hence the magic number
    if (res < 0) {
        perror("Error in listen!");
        close(socketfd);
        exit(1);

    }    int server_mode;

    addrlen = sizeof(struct sockaddr_in);


}


int client(char *hostname) {
    struct hostent *h;
    struct sockaddr_in serv_addr;


    h = gethostbyname(hostname);

    if (h == NULL) {
        fprintf(stderr, "Hostname \"%s\" could not be resolved\n", hostname);
        exit(-2);
    }
    

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5555);
    serv_addr.sin_addr = *addr;

    if (connect(socketfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to server!");
        close(socketfd);
        exit(1);
    }




}



int main(int argc, char const **argv) {
    int server_mode;
    char *hostname;



    if (argc < 2 ) {
        server();

    }
    else if (argc == 2) {
        client(argv[1]);

    }
    else {
        printf("Server Syntax: %s \n", argv[0]);
        printf("Client Syntax: %s <hostname> \n", argv[0]);
        exit(1);
    }






    return 0;
}