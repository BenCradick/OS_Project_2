#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <getopt.h>
#include <ctype.h>
#include <time.h>

void checkErrorFlag(char* str);

int main(int argc, char **argv) {
    /*
     * Variables used for handling shared memory -Start
     */
    key_t key = ftok("oss.c",42); //42 is arbitrary, but also the answer to life the universe and everything.
    key_t key2 = ftok("oss.c", 76); // again arbitrary

    int shmid = shmget(key, sizeof(int),0666 |IPC_CREAT);
    int shmid2  = shmget(key2, sizeof(int), 0666 | IPC_CREAT);

    int* sec = (int*) shmat(shmid, NULL,0);
    int* nano = (int*) shmat(shmid2, NULL, 0);
    /*
     * Variables used for shared memory -End
     *
     * Variables used for argument handling -Start
     */

    int c; // used to hold return value of getopt
    int timeIncrament; // specified by first line of input file

    long maxChildren;
    long maxLiveChildren;

    char* inputFile;
    char* outputFile;

    char line[LINE_MAX];


    FILE *inputStream;
    FILE *outputStream;

    //Default values for input/output files
    inputFile = "input.txt";
    outputFile = "output.txt";
    // End argument handling variables.

    time_t start;
    time(&start);


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
                abort();
            default:
                fprintf(stderr, "Unknown command line argument occurred.\n");
                fprintf(stdout, "Use commandline argument -h for help\n");
                abort();
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

    timeIncrament = strtol(line, NULL, 10);




    shmdt(sec);

    if(errno != 0){
        perror("Unable to partition shared memory: ");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


//helper for streamlining the checking for the error flag and then writing an error message.
void checkErrorFlag(char* str){
    if(!errno){
        perror(str);
        abort();
    }
    else{
        return;
    }
}