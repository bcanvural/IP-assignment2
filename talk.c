#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT_NUM 5555
#define MAX_MESSAGE_LENGTH 1024

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
    int socketfd, newsockfd, err, res, bytes_read;
    struct sockaddr_in addr, client_addr;
    socklen_t addrlen;
    int reuseaddr = 1;

    char recv_message[MAX_MESSAGE_LENGTH];
    char input_message[MAX_MESSAGE_LENGTH];

    socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd < 0) {
        perror("Error creating socket!");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT_NUM);
    // addr.sin_addr.s_addr = inet_addr("130.37.154.76");
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

    newsockfd = accept(socketfd, (struct sockaddr *) &client_addr, &addrlen);
    if (newsockfd < 0) {
        perror("newsockfd accept error");
    }
    else {
        printf("Client connection accepted!\n");
    }


    int pid;

    pid = fork();

    if (pid == 0) { // The child sends messages to the client

        // Start reading keyboard input and send it to the client
        while (1) {
            if (gets(input_message) == NULL) {
                perror("input error");
            }
            else {
                writen(newsockfd, &input_message, sizeof(input_message));
            }
        }
    }
    else { // The parent reads incoming messages

        while (1) {
            
            bytes_read = read(newsockfd, &recv_message, sizeof(recv_message));
            if (bytes_read == 0) {
                printf("Session has ended, closing server...\n");
                kill(pid, SIGTERM);
                close(newsockfd);
                return 0;
            }
            else if (bytes_read < 0) {
                perror("Read error occurred");
                kill(pid, SIGTERM);
                close(newsockfd);
                exit(1);
            }
            else {
                printf("%s\n", recv_message);
            }
        }
    }


    close(newsockfd);
    close(socketfd);


}


int client(char *hostname) {
    int socketfd, bytes_read;

    struct hostent *h;
    struct sockaddr_in serv_addr;
    struct in_addr *addr;

    char recv_message[MAX_MESSAGE_LENGTH];
    char input_message[MAX_MESSAGE_LENGTH];


    h = gethostbyname(hostname);

    if (h == NULL) {
        fprintf(stderr, "Hostname \"%s\" could not be resolved\n", hostname);
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

    printf("Connected to server!\n");

    int pid;

    pid = fork();

    if (pid == 0) { // Child sends messages to the server
        while (1) {
            
            if (gets(input_message) == NULL) {
                perror("input error");
            }
            else {
                writen(socketfd, &input_message, sizeof(input_message));
            }
        }

    }
    else { // Parent reads incoming messages
        while (1) {
            
            bytes_read = read(socketfd, &recv_message, sizeof(recv_message));
            if (bytes_read == 0) {
                printf("Session has ended, closing client...\n");
                kill(pid, SIGTERM);
                close(socketfd);
                return 0;
            }
            else if (bytes_read < 0) {
                perror("Read error occurred");
                kill(pid, SIGTERM);
                close(socketfd);
                exit(1);
            }
            else {
                printf("%s\n", recv_message);
            }
        }
    }


    close(socketfd);




}



int main(int argc, char **argv) {

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