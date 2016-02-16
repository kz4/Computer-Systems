#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <string.h>

#define READ_END 0
#define WRITE_END 1

void forkChildren (char * workerPath, char * mechanism, char * value, char * n) {
    pid_t pid;

    double sum = 0;
    for (int i = 0; i < atoi(n); i++) {
        int fd[2];
        pipe(fd);
        pid = fork();
        if (pid == -1) {
            /* error handling here, if needed */
            fprintf(stderr, "ERROR: pid = -1\n");
            return;
        }
        else if (pid == 0) {
//            fprintf(stderr, "I am a child: %d PID: %d\n",i, getpid());
            close(fd[READ_END]);   // close the read end of the pipe
            dup2(fd[WRITE_END], STDOUT_FILENO);
//            fprintf(stderr, "worker path: %s\n", workerPath);
            char * index = malloc(16);
            snprintf(index, 16, "%d", i);
//            fprintf(stderr, "Current i: %s\n", str);
            execl(workerPath, workerPath, "-x", value, "-n", index, "From master", NULL);
            close(fd[WRITE_END]);   // close the write end of the pipe

        }
        else {
            wait(NULL);
            fprintf(stderr, "I am a parent: %d PID: %d\n",i, getpid());
            close(fd[WRITE_END]);   // close the write end
//            dup2(fd[READ_END], 0);
            double buf;
            read(fd[READ_END], &buf, sizeof(double));
//            fprintf(stderr, "buf: %f\n", buf);
            fprintf(stderr, "buf: %.10e\n", buf);
            sum += buf;
            close(fd[READ_END]);   // close the read end
        }
    }
    fprintf(stderr, "sum: %f\n", sum);
}

void selectProcess(char * workerPath, char * mechanism, char * value, char * n) {
    pid_t pid;

    int num = atoi(n);
    int pipeFds[num];
    int isPipeFdsSet[num];

    double sum = 0;
    for (int i = 0; i < num; i++) {
        int fd[2];
        pipe(fd);
        pipeFds[i] = fd[0];   // Record read end
        isPipeFdsSet[i] = 1;
        pid = fork();
        if (pid == -1) {
            /* error handling here, if needed */
            fprintf(stderr, "ERROR: pid = -1\n");
            return;
        }
        else if (pid == 0) {
            close(fd[READ_END]);   // close the read end of the pipe
            dup2(fd[WRITE_END], STDOUT_FILENO);
            char * index = malloc(16);
            snprintf(index, 16, "%d", i);
            execl(workerPath, workerPath, "-x", value, "-n", index, "From master", NULL);
            close(fd[WRITE_END]);   // close the write end of the pipe

        }
        else {
            // Do nothing
        }
    }

    int counter = 0;

    // -1 indicates not exit for the while loop, 1 means to exit
    int exit = -1;

    while(1)
    {
        fd_set readset;
        // Initialize the set
        FD_ZERO(&readset);
        int maxfd = 0;
        int j, result;

        for (j=0; j<num; j++) {
            if (isPipeFdsSet[j] == 1)
            {
                FD_SET(pipeFds[j], &readset);
                maxfd = (maxfd>pipeFds[j])?maxfd:pipeFds[j];
            }
        }

        // Now, check for readability
        result = select(maxfd+1, &readset, NULL, NULL, NULL);
        if (result == -1) {
            // Some error...
        }
        else {
            for (j=0; j<num; j++) {
                if (FD_ISSET(pipeFds[j], &readset)) {
                    // myfds[j] is readable
                    isPipeFdsSet[j] = -1;
                    double buf;
                    read(pipeFds[j], &buf, sizeof(double));
                    fprintf(stderr, "buf: %f\n", buf);
                    sum += buf;
                    close(pipeFds[j]);   // close the read end
                    counter++;
                    fprintf(stderr, "counter: %d num: %d\n", counter, num);
                    if (counter == num) {
                        exit = 1;
                        break;
                    }
                }
            }
        }
        if (exit == 1)
        {
            break;
        }
    }
    fprintf(stderr, "sum: %f\n", sum);
}

void pollProcess(char * workerPath, char * mechanism, char * value, char * n) {
    pid_t pid;

    int num = atoi(n);
    int pipeFds[num];

    double sum = 0;
    for (int i = 0; i < num; i++) {
        int fd[2];
        pipe(fd);
        pipeFds[i] = fd[READ_END];   // Record read end
        pid = fork();
        if (pid == -1) {
            /* error handling here, if needed */
            fprintf(stderr, "ERROR: pid = -1\n");
            return;
        }
        else if (pid == 0) {
            close(fd[0]);   // close the read end of the pipe
            dup2(fd[WRITE_END], STDOUT_FILENO);
            char * index = malloc(16);
            snprintf(index, 16, "%d", i);
            execl(workerPath, workerPath, "-x", value, "-n", index, "From master", NULL);
            close(fd[WRITE_END]);   // close the write end of the pipe

        }
        else {
            // Do nothing
        }
    }

    int counter = 0;

    // -1 indicates not exit for the while loop, 1 means to exit
    int exit = -1;
    struct pollfd fds[num];

    int j;
    for (j=0; j<num; j++) {
        /* Open STREAMS device. */
        fds[j].fd = pipeFds[j];
        fds[j].events = POLLIN;
    }

    while(1)
    {
        int result;

        // Now, check for readability
        result = poll(fds, num, -1);
        if (result == -1) {
            // Some error...
        }
        else {
            for (j=0; j<num; j++) {
                if (fds[j].revents & POLL_IN) {
                    double buf;
                    read(pipeFds[j], &buf, sizeof(double));
                    fprintf(stderr, "buf: %f\n", buf);
                    sum += buf;
                    close(pipeFds[j]);   // close the read end
                    counter++;
                    fprintf(stderr, "counter: %d num: %d\n", counter, num);
                    if (counter == num) {
                        exit = 1;
                        break;
                    }
                }
            }
        }
        if (exit == 1)
        {
            break;
        }
    }
    fprintf(stderr, "sum: %f\n", sum);
}

void epollProcess(char * workerPath, char * mechanism, char * value, char * n) {
    int fd[2];
    pid_t pid;

    int num = atoi(n);
    int pipeFds[num];
    int isPipeFdsSet[num];

    double sum = 0;
    for (int i = 0; i < num; i++) {
        pipe(fd);
        pipeFds[i] = fd[0];   // Record read end
        pid = fork();
        isPipeFdsSet[i] = 1;
        if (pid == -1) {
            /* error handling here, if needed */
            fprintf(stderr, "ERROR: pid = -1\n");
            return;
        }
        else if (pid == 0) {
            close(fd[READ_END]);   // close the read end of the pipe
            dup2(fd[WRITE_END], STDOUT_FILENO);
            char * index = malloc(16);
            snprintf(index, 16, "%d", i);
            execl(workerPath, workerPath, "-x", value, "-n", index, "From master", NULL);
            close(fd[WRITE_END]);   // close the write end of the pipe

        }
        else {
            // Do nothing
        }
    }

    int counter = 0;

    // -1 indicates not exit for the while loop, 1 means to exit
    int exit = -1;
    fprintf(stderr, "Before epoll_event declaration\n");
    struct epoll_event ev, events[num];
    int epollfd;

    fprintf(stderr, "Before epoll_create1\n");
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
    }

    fprintf(stderr, "Before loop\n");
    int j;
    for (j=0; j<num; j++) {
        ev.events = EPOLLIN;
        ev.data.fd = pipeFds[j];
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pipeFds[j], &ev) == -1) {
            perror("epoll_ctl: listen_sock!\n");
//            exit(EXIT_FAILURE);
        }
    }

    for (int k = 0; k < num; ++k) {
        fprintf(stderr, "fd: %d\n", pipeFds[k]);
    }

    while(1)
    {
        int result;

        // Now, check for readability
        result = epoll_wait(epollfd, events, num, -1);
//        fprintf(stderr, "result: %d\n", result);

        if (result == -1) {
            // Some error...
        }
        else {
            for (j=0; j<result; j++) {
                for (int i = 0; i < num; ++i) {
                    if (events[j].data.fd == pipeFds[i] && isPipeFdsSet[i] == 1) {
                        fprintf(stderr, "Right after if check\n");

                        double buf;
                        read(pipeFds[i], &buf, sizeof(double));
                        fprintf(stderr, "buf: %f\n", buf);
                        sum += buf;
                        close(pipeFds[i]);   // close the read end
                        counter++;
                        fprintf(stderr, "counter: %d num: %d\n", counter, num);
                        isPipeFdsSet[i] = -1;
                        if (counter == num) {
                            exit = 1;
                            fprintf(stderr, "Exit nested for loop\n");
                            break;
                        }
                    }
                }
            }
        }

        fprintf(stderr, "exit: %d\n", exit);
        if (exit == 1)
        {
            break;
        }
    }
    fprintf(stderr, "sum: %f\n", sum);
}

int main(int argc, char **argv) {
    if (argc != 9) {
        fprintf(stderr, "There should be 9 arguments\n");
    } else {
        char * MECHANISM = argv[4];
        fprintf(stderr, "MECHANISM: %s\n", MECHANISM);
        if (strcmp(MECHANISM, "sequential") == 0) {
            forkChildren (argv[2], argv[4], argv[6], argv[8]);
            fprintf(stderr, "Finished sequential\n");
        } else if (strcmp(MECHANISM, "select") == 0) {
            selectProcess(argv[2], argv[4], argv[6], argv[8]);
            fprintf(stderr, "Finished select\n");
        } else if (strcmp(MECHANISM, "poll") == 0) {
            pollProcess(argv[2], argv[4], argv[6], argv[8]);
            fprintf(stderr, "Finished poll\n");
        } else if (strcmp(MECHANISM, "epoll") == 0) {
            epollProcess(argv[2], argv[4], argv[6], argv[8]);
            fprintf(stderr, "Finished epoll\n");
        }
        else {
            fprintf(stderr, "Error in entering the arguments\n");
        }
    }
    return 0;
}