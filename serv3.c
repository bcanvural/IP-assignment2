#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

//number of processes to be pre-forked
#define NB_PROC 3
#define PORT_NUM 4444

// Set up the semaphore that is used to prevent race conditions in acces to
// the shared memory segment.
struct sembuf up = {0, 1, 0};
struct sembuf down = {0, -1, 0};

// Shared memory id and semaphore id. Made global for use in the custom SIGINT
// handler to destroy the ID when the server is killed.
int sem;
int shmid;

// client_counter shared variable, declared globally
uint32_t *client_counter;

//this struct carries the socket settings and client address
//this is used when creating another child upon a child's termination
struct socket_vars {
    int socketfd;
    struct sockaddr_in client_addr;
    socklen_t addrlen;
};
//global variable that carries the above variables
struct socket_vars s_vars;

void treat_request(int fd) {

    *client_counter = *client_counter + 1;
    ssize_t bytes_written;
    bytes_written = write(fd, client_counter, sizeof(uint32_t));

    if (bytes_written < sizeof(uint32_t)) {
        perror("Not enough bytes are written!");
    }

}

void recv_requests(int fd, struct sockaddr_in client_addr, socklen_t*  addrlen) { /* An iterative server */
    int newfd;

    /* --- ACQUIRE MUTEX --- */
    semop(sem, &down, 1);
    newfd = accept(fd, (struct sockaddr *) &client_addr, addrlen);
    if (newfd < 0) {
        close(fd);
        close(newfd);
        exit(1);
    }
    treat_request(newfd);
    /* --- RELEASE MUTEX --- */
    semop(sem, &up, 1);
    close(fd);
    close(newfd);
    exit(0);
}

// When the server is killed, make sure to mark the shared memory segment and
// the semaphore array for destruction.
void sig_int(int sig) {
    if (sig == SIGINT) {
        shmctl(shmid, IPC_RMID, 0);
        semctl(sem, 0, IPC_RMID);
        exit(0);
    }
}

void sig_chld(int sig) {
    while (waitpid(0, NULL, WNOHANG) > 0) { }
    //create another child upon a child's termination
    int pid = fork();
    if (pid == 0){ //child
        recv_requests(s_vars.socketfd,
            s_vars.client_addr,
            &s_vars.addrlen);
    }
    signal(sig, sig_chld);
}

// The TCP-server with pre-forking structure.
int main(int argc, char **argv) {
    int socketfd, newsockfd, err, res, pid;
    struct sockaddr_in addr, client_addr;
    socklen_t addrlen;
    int reuseaddr = 1;
    ssize_t bytes_written;

    socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd < 0) {
        perror("Error creating socket!");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT_NUM);
    // addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

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

    signal(SIGINT, sig_int); // Use custom SIGINT handler to destroy semaphore
    // and shared memory segment when the server is
    // killed.

    // Set up the shared memory segment to store the client counter
    shmid = shmget(IPC_PRIVATE, sizeof(uint32_t), 0600);
    if (shmid < 0) {
        perror("shmget failed");
        close(socketfd);
        exit(1);
    }

    //client_counter is declared globally
    client_counter = (uint32_t *) shmat(shmid, 0, 0);
    if (client_counter == NULL) {
        perror("shmat error");
        close(socketfd);
        shmctl(shmid, IPC_RMID, 0);
        exit(1);
    }

    *client_counter = 0;

    sem = semget(IPC_PRIVATE, 1, 0600); // Create semaphore

    semop(sem, &up, 1); // Start at value 1
    signal(SIGCHLD, sig_chld);

    //setting socket_vars struct
    s_vars.socketfd = socketfd;
    s_vars.client_addr = client_addr;
    s_vars.addrlen = addrlen;

    /* Create NB_PROC children. */
    int i;
    for (i = 0; i < NB_PROC; i++) {
        int pid = fork();
        if (pid == 0) { //child
            recv_requests(socketfd, client_addr, &addrlen);
        }
    }

    while ((pid = waitpid(-1, NULL, 0))) {
        if (errno == ECHILD) {
            break;
        }
    }

    close(socketfd);
    shmdt((void *) client_counter);
    shmctl(shmid, IPC_RMID, 0);
    semctl(sem, 0, IPC_RMID);
    return 0;
}
