//
// Created by Ben Cradick on 2019-02-26.
//

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

void checkErrorFlag(char* string);

int main(int argc, char **argv){
    char cwd[PATH_MAX];

    getcwd(cwd, sizeof(cwd));
   // printf("%s\n",cwd);
    key_t key = ftok("oss.c", 42);
    key_t key2 = ftok("child.c", 76); // again arbitrary
    checkErrorFlag("ftok() failed");

    int shmid = shmget(key, sizeof(int),  0666 | IPC_CREAT);
    int shmid2  = shmget(key2, sizeof(int), 0666 | IPC_CREAT);

    long* sec = (long*) shmat(shmid, NULL,0);
    long* nano = (long*) shmat(shmid2, NULL, 0);

    long duration = strtol(argv[1], NULL, 10);

    long endSec;
    long endNano;

    const long billion = 1000000000;

    long temp = duration % billion;

    endSec = *sec + temp;
    endNano = *nano - (temp * billion);

    while(*sec <= endSec && *nano <= endNano){
//        sec = (long*)shmat(shmid, NULL, 0);
//        nano = (long*)shmat(shmid2, NULL, 0);
    }

    fprintf(stdout, "pid: %d secounds: %ld nanoseconds: %ld %s\n", getpid(), *sec, *nano, "Child process is terminating.");
    shmdt(sec);
    shmdt(nano);
    exit(EXIT_SUCCESS);
}
void checkErrorFlag(char* str){
    if(errno != 0){
        perror(str);
        exit(EXIT_FAILURE);
    }
    else{
        return;
    }
}