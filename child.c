//
// Created by Ben Cradick on 2019-02-26.
//

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdlib.h>

int main(){

    key_t key = ftok("oss.c", 42);

    int shmid = shmget(key, 1024, 0666 | IPC_CREAT);

    char* str = (char*) shmat(shmid, (void*)0,0);

    printf("%s\n", str);

    shmctl(shmid, IPC_RMID, NULL);
    if(errno != 0){
        perror("Error reading shared memory: ");
        return EXIT_FAILURE;
    }
    return 0;
}