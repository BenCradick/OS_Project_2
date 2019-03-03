#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/time.h>
#include <signal.h>
#include <memory.h>

void checkErrorFlag(char* str);
void sigHandle(int sig);
void programAlarm(void);
void freeSharedMemory();
void incrementTime(long *nano, long *sec, long timeIncrement, long billion);
void printSharedClock();


FILE *outputStream; // needed to be global for requirement about outputing shared clock data.

int main(int argc, char **argv) {


    struct itimerval it_val;

    if(signal(SIGALRM, (void(*)(int)) programAlarm) == SIG_ERR){
        perror("Failed to catch SIGALRM");
        exit(1);
    }
    if(signal(SIGABRT,sigHandle) == SIG_ERR){
        perror("Failed to catch SIGABRT\n");
    }
    if(signal(SIGINT, sigHandle) == SIG_ERR){
        perror("Failed to catch SIGINT\n");
    }
    it_val.it_value.tv_sec = 10;
    it_val.it_value.tv_usec = 0;

    it_val.it_interval = it_val.it_value;

    if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
        perror("Failed to call setitimer: ");
        exit(1);
    }
    /*
     * Variables used for handling shared memory -Start
     */
    key_t key = ftok("oss.c", 42); //42 is arbitrary, but also the answer to life the universe and everything.
    checkErrorFlag("Error generating shared memory keys: ");
    key_t key2 = ftok("child.c", 76); // again arbitrary
    checkErrorFlag("Error generating shared memory keys: ");

    int shmid = shmget(key, sizeof(int),  0666 | IPC_CREAT);
    int shmid2  = shmget(key2, sizeof(int), 0666 | IPC_CREAT);
    checkErrorFlag("Error assigning shared memory id: ");

    long* sec = (long*) shmat(shmid, NULL,0);
    long* nano = (long*) shmat(shmid2, NULL, 0);

    FILE *inputStream;

    checkErrorFlag("Error assigning shared memory to variable: ");
    /*
     * Variables used for shared memory -End
     *
     * Variables used for argument handling -Start
     */

    int c; // used to hold return value of getopt
    long timeIncrement; // specified by first line of input file

    long nextSec;
    long nextNano;

    long maxChildren = 4;
    long maxLiveChildren = 2;
    long liveChildren = 0;
    long children = 0;
    const long billion = 1000000000;

    char* inputFile;
    char* outputFile;

    char line[LINE_MAX];
    char* tokens[3];
    char* token;

    int i = 0; // iterator
    int status;

    pid_t pid;



    //Default values for input/output files
    inputFile = "input.txt";
    outputFile = "output.txt";
    // End argument handling variables.

    checkErrorFlag("Error fetching the time: ");


    while((c = getopt(argc, argv, "h:n:s:i:o:")) != -1){
        switch(c){
            case 'h':
                fprintf(stdout, "Arguments that this program takes include: \n "
                                "-h : Displays this page\n"
                                "-n : Specifies the max number of child processes default is 4, max 20\n"
                                "-s : Specifies max number of concurrent child processes default value is 2\n"
                                "-i : Specifies the input file to read default is input.txt\n"
                                "-o : Specifies the output file to write to default is output.txt\n");
                fprintf(stdout, "Program will now exit without running the programs core functionality.\n");
                return EXIT_SUCCESS;

            case 'n':
                maxChildren = strtol(optarg, NULL, 10);
                break;
            case 's':
                maxLiveChildren = strtol(optarg, NULL, 10);
                if(maxLiveChildren >=20){
                    maxLiveChildren = 19;
                }
                break;
            case 'i':
                inputFile = optarg;
                break;
            case 'o':
                outputFile = optarg;
                break;
            case '?':
                if(optopt == 'h' || optopt == 'n' || optopt == 's' || optopt == 'i' || optopt == 'o'){
                    fprintf(stderr, "-%c requires and additional argument\n", optopt);
                }
                else if(isprint(optopt)){
                    fprintf(stderr, "Unknown option character '-%c' \n", optopt);
                }
                else{
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                }
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "Unknown command line argument occurred.\n");
                fprintf(stdout, "Use commandline argument -h for help\n");
                exit(EXIT_FAILURE);
        }
    }

    //Readying files.
    inputStream = fopen(inputFile, "r");
    checkErrorFlag("Error Opening input file: ");
    outputStream = fopen(outputFile, "a+");
    checkErrorFlag("Error opening output file: ");

    //get incrementation value for file
    if(fgets(line, LINE_MAX, inputStream) == NULL){
        fprintf(stderr, "File %s contains no text", inputFile);
        fclose(inputStream);
        return EXIT_FAILURE;
    }

    timeIncrement = strtol(line, NULL, 10);
    fgets(line, LINE_MAX, inputStream);
    token = strtok(line, " ");
    while(token != NULL){
        tokens[i] = token;
        token = strtok(NULL, " ");
        i++;
    }

    i = 0; //reset for later use.

    nextSec = strtol(tokens[0], NULL, 10);
    nextNano = strtol(tokens[1], NULL, 10);


    while(children < maxChildren){
        incrementTime(nano, sec, timeIncrement, billion);
        if(liveChildren < maxLiveChildren && *sec >= nextSec && *nano >= nextNano){
            liveChildren++;
            children++;
            fprintf(outputStream, "seconds: %ld nanoseconds: %ld pid: %d duration: %s", *sec, *nano, getpid(), tokens[2]);
            fflush(outputStream); // without flush child prints as well

            pid = fork();
            if(pid == 0){


                fclose(outputStream);
                char* args[3] = {"./child", tokens[2], (char*)0}; // needed null terminator
                if(execv("./child", args) == -1){
                    perror("Error executing file");
                }
            }
            else{

                if(fgets(line,LINE_MAX, inputStream) != NULL){
                    token = strtok(line, " ");
                    while( token != NULL){
                        tokens[i] = token;
                        token = strtok(NULL, " ");
                        i++;
                    }
                    nextSec = strtol(tokens[0],NULL,10);
                    nextNano = strtol(tokens[1], NULL, 10);
                    i = 0; // reset again
                }
                else{
                    children = maxChildren;

                }
            }
        }

        if((pid = waitpid(0, &status, WNOHANG)) > 0){
            if(WIFEXITED(status)) {
                liveChildren--;
                fprintf(outputStream, "pid: %d seconds: %ld nanoseconds: %ld\n", pid, *sec, *nano);
            }


        }
            if(errno > 0)
            {
                if(errno == ECHILD)
                {
                    errno = 0;
                }
                else
                {
                    perror("wait error");
                }
            }

    }
    while(liveChildren > 0){
        incrementTime(nano, sec, timeIncrement, billion);

        if((pid = waitpid(0, &status, WNOHANG)) > 0){
            if(WIFEXITED(status)) {
                fprintf(outputStream, "pid: %d seconds: %ld nanoseconds: %ld\n", pid, *sec, *nano);
                liveChildren--;

            }

        }
        else{
            if(errno == ECHILD){
                liveChildren = 0;
            }
        }
    }
    shmdt(sec);
    shmdt(nano);

    printSharedClock();
    freeSharedMemory();
    fclose(outputStream);
    return EXIT_SUCCESS;
}


//helper for streamlining the checking for the error flag and then writing an error message.
void checkErrorFlag(char* str){
    if(errno != 0){
        perror(str);
        exit(EXIT_FAILURE);
    }
    else{
        return;
    }
}
void sigHandle(int sig) {


    if (errno == ECHILD) {
        errno = 0;
    }
    printSharedClock();
    freeSharedMemory();
    exit(EXIT_FAILURE);
}
void programAlarm(){
    fprintf(stdout,"Program time out\n");
    signal(SIGINT, SIG_IGN);
    kill(0, SIGINT);
    printSharedClock();
    freeSharedMemory();
    exit(EXIT_FAILURE);


}
void freeSharedMemory(){

    key_t key = ftok("oss.c",42); //42 is arbitrary, but also the answer to life the universe and everything.
    key_t key2 = ftok("child.c", 76); // again arbitrary
    checkErrorFlag("Error generating shared memory keys: ");

    int shmid = shmget(key, sizeof(int), 0666 | IPC_CREAT);
    int shmid2  = shmget(key2, sizeof(int), 0666 | IPC_CREAT);
    checkErrorFlag("Error assigning shared memory id: ");

    shmctl(shmid,IPC_RMID,NULL);
    shmctl(shmid2, IPC_RMID,NULL);
}
void incrementTime(long *nano, long *sec, long timeIncrement, const long billion){
    *nano = timeIncrement + *nano;
    if(*nano > billion){
        long temp = *nano / billion;
        *sec += temp;
        *nano = *nano % billion;
    }
}
void printSharedClock(){
    key_t key = ftok("oss.c",42); //42 is arbitrary, but also the answer to life the universe and everything.
    key_t key2 = ftok("child.c", 76); // again arbitrary
    checkErrorFlag("Error generating shared memory keys: ");

    int shmid = shmget(key, sizeof(int), 0666 | IPC_CREAT);
    int shmid2  = shmget(key2, sizeof(int), 0666 | IPC_CREAT);

    long* sec = (long*) shmat(shmid, NULL,0);
    long* nano = (long*) shmat(shmid2, NULL, 0);

    fprintf(outputStream, "seconds: %ld nanoseconds: %ld\n", *sec, *nano);

    shmdt(sec);
    shmdt(nano);

}