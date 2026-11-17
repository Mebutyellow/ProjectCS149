
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define NUM_SECTORS 100     // 100 blocks total the first block is the subdirectory
#define SECTOR_SIZE 512     // 512 bytes, integer = 4 bytes
#define DIR_ENTRIES 25      // if more files come, then need to create another directory sector
#define DATA_SIZE   504     // 504 since forward and backward take 8 bytes total
#define FILE_NAME_LENGTH 9  // max name lenght is 8 characters + '\0\' for the end of character

union block {

    struct directory{

        int back;
        int frwd;
        int free;

        struct {
            char type;
            char name[9];
            int link;
            short size;
        } dir[25];

    } directory;


    struct{

        int backf;
        int frwdf;
        char userdata[504];

    } data;

};

struct nodeType {
    int info;
    struct nodeType *link;
};

union block sector[100];

static struct nodeType *free_list;
static int current_directory;

void createSector(int index, int b, int f){
    sector[index].directory.back = b;
    sector[index].directory.frwd = f;
    sector[index].directory.free = 25;

    for (int i = 0; i < 25; i++) {
        sector[index].directory.dir[i].type = 'F';
        sector[index].directory.dir[i].name[0] = '\0';
        sector[index].directory.dir[i].link = 0;
        sector[index].directory.dir[i].size = 0;
    }
}

static void freeList(void)
{
    struct nodeType *node;
    while (free_list != NULL) {
        node = free_list;
        free_list = node->link;
        free(node);
    }
}

static int buildList(void)
{
    int i;
    free_list = NULL;
    for (i = NUM_SECTORS - 1; i >= 0; --i) {
        struct nodeType *node = malloc(sizeof(struct nodeType));
        if (node == NULL)
            return 0;
        node->info = i;
        node->link = free_list;
        free_list = node;
    }
    return 1;
}

static int Remove(void)
{
    struct nodeType *node;
    int result;
    if (free_list == NULL)
        return -1;
    node = free_list;
    free_list = node->link;
    result = node->info;
    free(node);
    return result;
}

static void Insert(int sector_number)
{
    struct nodeType *node = malloc(sizeof(struct nodeType));
    if (node == NULL) {
        fprintf(stderr, "Unable to return sector %d to the free list.\n",
                sector_number);
        return;
    }
    node->info = sector_number;
    node->link = free_list;
    free_list = node;
}


int main(void){

    createSector(0, -1, -1);

    printf("Welcome to the file management system project:\n");
    printf("This file system has %d sectors with %d bytes each.\n", NUM_SECTORS, SECTOR_SIZE);



    char input[256];
    char cmd[16];

    while (1) {

        printf(">> ");

        if (fgets(input, sizeof(input), stdin) == NULL) break;
        input[strcspn(input, "\n")] = '\0';
        if (sscanf(input, "%15s", cmd) != 1) continue;   // blank line, ask again


        if (strcmp(cmd, "EXIT") == 0) {
            break;
        } else if (strcmp(cmd, "CREATE") == 0) {

            char type, name[16];
            if (sscanf(input, "%*s %c %15s", &type, name) != 2)
                printf("Usage: CREATE <U|D> <name>\n");
            else
                printf("create %c %s\n", type, name);    // call your real function here


        } else if (strcmp(cmd, "DIR") == 0) {
            
            printf("dir\n");                              // call your real function here

        } else {
            printf("Invalid command.\n");
        }



    }

    return 0;

}