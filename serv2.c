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

// Shared memory id and semaphore id. Made global for use in the custom SIGINT 
// handler to destroy the ID when the server is killed. 
int sem;
int shmid;


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
    signal(sig, sig_chld);

}


// The TCP-server with one-proces-per-client structure.
int main(int argc, char **argv) {
    int socketfd, newsockfd, err, res, pid;
    struct sockaddr_in addr, client_addr;
    // struct hostent *resolv; 
    socklen_t addrlen;
    int reuseaddr = 1;
    ssize_t bytes_written;


    socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd < 0) {
        perror("Error creating socket!");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(4444);
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
    
    uint32_t *client_counter = (uint32_t *) shmat(shmid, 0, 0);
    if (client_counter == NULL) {
        perror("shmat error");
        close(socketfd);
        shmctl(shmid, IPC_RMID, 0);
        exit(1);
    }

    *client_counter = 0;

    // Set up the semaphore that is used to prevent race conditions in acces to
    // the shared memory segment.
    struct sembuf up =   {0, 1, 0};
    struct sembuf down = {0, -1, 0};

    sem = semget(IPC_PRIVATE, 1, 0600); // Create semaphore

    semop(sem, &up, 1); // Start at value 1



    signal(SIGCHLD, sig_chld);
    while (1) {
        newsockfd = accept(socketfd, (struct sockaddr *) &client_addr, &addrlen);

        if (newsockfd < 0) {
            perror("newsockfd accept error");
            continue;
        }

        pid = fork();
        if (pid == 0) { // Child
            semop(sem, &down, 1);

            *client_counter = *client_counter + 1;
            bytes_written = write(newsockfd, client_counter, sizeof(uint32_t));
            semop(sem, &up, 1);

            if (bytes_written < sizeof(uint32_t)) {
                perror("Not enough bytes are written!");
            }

            shmdt((void *) client_counter);
            
            exit(0);
        }
        else { // Parent

            close(newsockfd);

        }
    }


    close(socketfd);
    shmdt((void *) client_counter);
    shmctl(shmid, IPC_RMID, 0);
    semctl(sem, 0, IPC_RMID);
    return 0;
}