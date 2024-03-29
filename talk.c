#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#define PORT_NUM 5555

int pid;

char my_getch() {
    char buf = 0;
    struct termios oldt, newt;
    if (tcgetattr(0, &oldt) < 0) {
        perror("tcgetattr()");
    }

    newt = oldt;
    newt.c_lflag &= ~ICANON;
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) < 0) {
        perror("tcsetattr ICANON");
    }
    if (read(0, &buf, 1) < 0) {
        perror ("read()");
    }
    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) < 0) {
        perror ("tcsetattr ~ICANON");
    }
    return buf;
}

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

ssize_t readn(int fd, void *vptr, size_t n) { /* Read "n" bytes from a descriptor. */
    size_t  nleft;
    ssize_t nread;
    char   *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR) {
                nread = 0;      /* and call read() again */
            }
            else {
                return -1;
            }
        }
        else if (nread == 0) {
            break;              /* EOF */
        }
        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft);         /* return >= 0 */
}


void sig_int(int sig) {
    if (sig == SIGINT) {
        kill(pid, SIGTERM);
        exit(0);
    }
}

int server() {
    int socketfd, newsockfd, err, res, bytes_read;
    struct sockaddr_in addr, client_addr;
    socklen_t addrlen;
    int reuseaddr = 1;

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

    newsockfd = accept(socketfd, (struct sockaddr *) &client_addr, &addrlen);
    if (newsockfd < 0) {
        perror("newsockfd accept error");
    }
    else {
        printf("Client connection accepted!\n");
    }

    // int pid;
    pid = fork();

    if (pid == 0) { // The child sends messages to the client

        // Start reading keyboard input and send it to the client
        while (1) {
            char c = my_getch();
            uint32_t bytes = htonl(c);
            writen(newsockfd, &bytes, sizeof(uint32_t));
        }
    }
    else { // The parent reads incoming messages
        signal(SIGINT, sig_int);
        char c;
        uint32_t bytes;
        while (1) {
            bytes_read = readn(newsockfd, &bytes, sizeof(bytes));
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
                c = (char)(ntohl(bytes));
                putchar(c);
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

    // int pid;

    pid = fork();

    if (pid == 0) { // Child sends messages to the server
        char c;
        while (1) {
            c = my_getch();
            uint32_t bytes = htonl(c);
            writen(socketfd, &bytes, sizeof(uint32_t));
        }
    }
    else { // Parent reads incoming messages
        signal(SIGINT, sig_int);
        char c;
        uint32_t bytes;
        while (1) {
            bytes_read = readn(socketfd, &bytes, sizeof(bytes));
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
                c = (char) (ntohl(bytes));
                putchar(c);
            }
        }
    }
    close(socketfd);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

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
