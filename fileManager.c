
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define NUM_BLOCKS 100
#define BLOCK_SIZE (1024 * 64)
#define MAX_FILES 100
#define FILE_NAME_LENGTH 32


typedef struct {

    char name[FILE_NAME_LENGTH];
    int fd;
    int size;
    int startBlock;
    int blocksAllocated;
    time_t created;
    time_t modified;

} MetaData;


MetaData metaDataArray[MAX_FILES];
char blockArray[NUM_BLOCKS][BLOCK_SIZE];
int nextArray[NUM_BLOCKS];
int bitmapArray[NUM_BLOCKS];



int createFile();
int deleteFile();
int writeFile();
void readFile();
void listFile();
void printOptions();



int main(){

    printf("Welcome to the file managment system project:\n");
    printf("This file managment system will have %d blocks with %d bytes of data for each block\n", NUM_BLOCKS, BLOCK_SIZE);

    char input[100];

    while (1) {

        printOptions();
        printf("Enter option: ");

        if (fgets(input, sizeof(input), stdin) == NULL) break;
        input[strcspn(input, "\n")] = '\0';
        if (strcmp(input, "-q") == 0) break;



        if (strcmp(input, "-c") == 0) {
            createFile();
        } else if (strcmp(input, "-d") == 0) {
            deleteFile();
        } else if (strcmp(input, "-w") == 0) {
            writeFile();
        } else if (strcmp(input, "-r") == 0) {
            readFile();
        } else if (strcmp(input, "-l") == 0) {
            listFile();
        } else {
            printf("Invalid option.\n");
        }
    }


    printf("Thank you for using our file system!\n");
    return 0;


    
}


void printOptions() {
    printf("\nYou have the options of:\n");
    printf("Create file:    '-c'\n");
    printf("Delete file:    '-d'\n");
    printf("Write to file:  '-w'\n");
    printf("Read file:      '-r'\n");
    printf("List files:     '-l'\n");
    printf("Quit program:   '-q'\n");
}




int createFile(){
    return 0;
}


int deleteFile(){
    return 0;

}


int writeFile(){
    return 0;

}


void readFile(){

}


void listFile(){

}
