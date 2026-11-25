
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define NUM_SECTORS 100     // 100 blocks total the first block is the subdirectory
#define SECTOR_SIZE 512     // 512 bytes, integer = 4 bytes
#define DIR_ENTRIES 25      // if more files come, then need to create another directory sector
#define DATA_SIZE   504     // 504 since forward and backward take 8 bytes total
#define FILE_NAME_LENGTH 9  // max name length is 8 characters + '\0\' for the end of character

union block {

    struct directory{

        int back;
        int frwd;
        int free;

        struct dirEntry{
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




struct openFileInfo{
    int isOpen;
    int dirSector;
    int dirIndex;
    char mode;
    int position;
};


static struct openFileInfo openInformation;



union block sector[100];

static struct nodeType *free_list;
static int current_directory;

static char current_path[128] = "~";




static void cdRoot(void) {
    current_directory = 0;
    strcpy(current_path, "~");
}



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
    for (i = NUM_SECTORS - 1; i >= 1; --i) {
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




static int findFile(int dir_sector, const char *name, int *found_sector){

    int s = dir_sector;

    while (s > 0 || s == dir_sector) {
        for (int i = 0; i < DIR_ENTRIES; ++i) {
            if (sector[s].directory.dir[i].type != 'F' &&
                strcmp(sector[s].directory.dir[i].name, name) == 0) {
                *found_sector = s;
                return i;
            }
        }
        s = sector[s].directory.frwd;
    }
    return -1;
}


static int findFreeEntry(int dir_sector, int *found_sector){

    int s = dir_sector;
    int last = dir_sector;

    while (s > 0 || s == dir_sector) {
        if (sector[s].directory.free > 0) {
            for (int i = 0; i < DIR_ENTRIES; ++i) {
                if (sector[s].directory.dir[i].type == 'F') {
                    *found_sector = s;
                    return i;
                }
            }
        }
        last = s;
        s = sector[s].directory.frwd;
    }

    int n = Remove();
    if (n == -1)
        return -1;

    createSector(n, last, -1);
    sector[last].directory.frwd = n;

    *found_sector = n;
    return 0;
}


static void create(char type, const char *name){

    if (type != 'U' && type != 'D') {
        printf("Type must be U or D.\n");
        return;
    }

    if (strlen(name) > 8) {
        printf("Name must be 8 characters or fewer.\n");
        return;
    }

    int dir_sector;
    int entry_index;

    if (findFile(current_directory, name, &dir_sector) != -1) {
        printf("Error: file or directory '%s' already exists.\n", name);
        return;
    }

    entry_index = findFreeEntry(current_directory, &dir_sector);
    if (entry_index == -1) {
        printf("Error: no free directory entries available.\n");
        return;
    }

    int new_sector = 0;

    if (type == 'D') {
        new_sector = Remove();
        if (new_sector == -1) {
            printf("Error: no free sectors available.\n");
            return;
        }
        createSector(new_sector, -1, -1);
    }

    sector[dir_sector].directory.dir[entry_index].type = type;
    strncpy(sector[dir_sector].directory.dir[entry_index].name, name, FILE_NAME_LENGTH - 1);
    sector[dir_sector].directory.dir[entry_index].name[FILE_NAME_LENGTH - 1] = '\0';
    sector[dir_sector].directory.dir[entry_index].link = new_sector;
    sector[dir_sector].directory.dir[entry_index].size = 0;
    sector[dir_sector].directory.free--;
}


static void printDirectory(int dir_sector){

    int s = dir_sector;

    printf("Directory contents:\n");
    while (s > 0 || s == dir_sector) {
        for (int i = 0; i < DIR_ENTRIES; ++i) {
            if (sector[s].directory.dir[i].type == 'F')
                continue;
            if (sector[s].directory.dir[i].type == 'D')
                printf("D  %s\n", sector[s].directory.dir[i].name);
            else
                printf("U  %s  %d bytes\n", sector[s].directory.dir[i].name, sector[s].directory.dir[i].size);
        }
        s = sector[s].directory.frwd;
    }

}



static void changeDirectory(const char *name){

    int dir_sector;
    int entry_index = findFile(current_directory, name, &dir_sector);

    if (entry_index == -1) {
        printf("Error: directory '%s' not found.\n", name);
        return;
    }

    if (sector[dir_sector].directory.dir[entry_index].type != 'D') {
        printf("Error: '%s' is not a directory.\n", name);
        return;
    }

    current_directory = sector[dir_sector].directory.dir[entry_index].link;

    if (strlen(current_path) + strlen(name) + 2 < sizeof(current_path)) {
        strcat(current_path, "/");
        strcat(current_path, name);
    }

}



static void openFile(char mode, const char *name){

    if (mode != 'I' && mode != 'O' && mode != 'U') {
        printf("Error: mode must be I, O, or U.\n");
        return;
    }

    if (openInformation.isOpen) {
        printf("Error: a file is already open.\n");
        return;
    }

    int dir_sector;
    int entry_index = findFile(current_directory, name, &dir_sector);

    if (entry_index == -1) {
        printf("Error: file '%s' not found.\n", name);
        return;
    }

    if (sector[dir_sector].directory.dir[entry_index].type != 'U') {
        printf("Error: '%s' is not a user file.\n", name);
        return;
    }

    openInformation.isOpen = 1;
    openInformation.dirSector = dir_sector;
    openInformation.dirIndex = entry_index;
    openInformation.mode = mode;
    if (mode == 'O'){
        openInformation.position = sector[dir_sector].directory.dir[entry_index].size;
    } else{
        openInformation.position = 0;
    }

    printf("File '%s' opened in mode '%c'.\n", name, mode);
}


static void closeFile(void){

    if (!openInformation.isOpen) {
        printf("Error: no file is currently open.\n");
        return;
    }

    openInformation.isOpen = 0;
    openInformation.dirSector = 0;
    openInformation.dirIndex = 0;
    openInformation.mode = '\0';
    openInformation.position = 0;


    printf("File closed.\n");
}


static void seek(int position){

    if (!openInformation.isOpen) {
        printf("Error: no file is currently open.\n");
        return;
    }

    int file_size = sector[openInformation.dirSector].directory.dir[openInformation.dirIndex].size;

    if (position < 0 || position > file_size) {
        printf("Error: position %d is out of bounds for file size %d.\n", position, file_size);
        return;
    }

    openInformation.position = position;
    printf("Seeked to position %d in the file.\n", position);
}




static void writeFile(int count, const char *data) {
    if (!openInformation.isOpen) {
        printf("Error: no file is open.\n");
        return;
    }

    if (openInformation.mode == 'I') {
        printf("Error: file is open for input only.\n");
        return;
    }

    if (count <= 0) {
        printf("Error: byte count must be positive.\n");
        return;
    }

    if ((int)strlen(data) < count) {
        printf("Error: fewer than %d data bytes were provided.\n", count);
        return;
    }

    struct dirEntry *e = &sector[openInformation.dirSector].directory.dir[openInformation.dirIndex];
    int pos = openInformation.position;

    if (pos + count > 32767) {
        printf("Error: file size cannot exceed 32767 bytes.\n");
        return;
    }

    int cur = e->link;
    int prev = 0;
    int skip = pos / DATA_SIZE;
    for (int i = 0; i < skip; i++) {
        prev = cur;
        cur = sector[cur].data.frwdf;
    }

    int offset = pos % DATA_SIZE;
    int written = 0;

    while (written < count) {

        if (cur == 0) {                       
            int n = Remove();
            if (n == -1) {
                printf("Error: disk full. Only %d bytes written.\n", written);
                break;
            }
            memset(sector[n].data.userdata, 0, DATA_SIZE);
            sector[n].data.backf = prev;
            sector[n].data.frwdf = 0;

            if (prev == 0)
                e->link = n;                  
            else
                sector[prev].data.frwdf = n;

            cur = n;
        }

        int room = DATA_SIZE - offset;
        int chunk = (count - written < room) ? (count - written) : room;

        memcpy(sector[cur].data.userdata + offset, data + written, chunk);
        written += chunk;
        offset += chunk;

        if (written < count) {                
            prev = cur;
            cur = sector[cur].data.frwdf;
            offset = 0;
        }
    }

    pos += written;
    openInformation.position = pos;
    if (pos > e->size)
        e->size = (short)pos;

    printf("%d bytes written.\n", written);
}




static void readFile(int count) {
    if (!openInformation.isOpen) {
        printf("Error: no file is open.\n");
        return;
    }

    if (openInformation.mode == 'O') {
        printf("Error: file is open for output only.\n");
        return;
    }

    if (count <= 0) {
        printf("Error: byte count must be positive.\n");
        return;
    }

    struct dirEntry *e = &sector[openInformation.dirSector].directory.dir[openInformation.dirIndex];
    int pos = openInformation.position;
    int size = e->size;

    if (pos >= size || e->link == 0) {
        printf("0 bytes read (end of file).\n");
        return;
    }

    if (count > size - pos)
        count = size - pos;

    int cur = e->link;
    for (int i = 0; i < pos / DATA_SIZE; i++)
        cur = sector[cur].data.frwdf;

    int offset = pos % DATA_SIZE;
    int done = 0;

    printf("Data: ");
    while (done < count && cur != 0) {
        int room = DATA_SIZE - offset;
        int chunk = (count - done < room) ? (count - done) : room;

        fwrite(sector[cur].data.userdata + offset, 1, chunk, stdout);
        done += chunk;
        offset = 0;
        cur = sector[cur].data.frwdf;
    }
    printf("\n");

    openInformation.position = pos + done;
    printf("%d bytes read.\n", done);
}



static void deleteEntry(const char *name) {
    int dir_sector;
    int idx = findFile(current_directory, name, &dir_sector);

    if (idx == -1) {
        printf("Error: '%s' not found.\n", name);
        return;
    }

    struct dirEntry *e = &sector[dir_sector].directory.dir[idx];

    if (openInformation.isOpen &&
        openInformation.dirSector == dir_sector &&
        openInformation.dirIndex == idx) {
        printf("Error: '%s' is currently open. CLOSE it first.\n", name);
        return;
    }

    if (e->type == 'U') {
        int s = e->link;
        while (s != 0) {
            int next = sector[s].data.frwdf;
            Insert(s);
            s = next;
        }
    } else if (e->type == 'D') {
        // check that every sector of the subdirectory is empty
        int s = e->link;
        while (s > 0) {
            if (sector[s].directory.free != DIR_ENTRIES) {
                printf("Error: directory '%s' is not empty.\n", name);
                return;
            }
            s = sector[s].directory.frwd;
        }

        // release every sector of the subdirectory's chain
        s = e->link;
        while (s > 0) {
            int next = sector[s].directory.frwd;
            Insert(s);
            s = next;
        }
    }

    e->type = 'F';
    e->name[0] = '\0';
    e->link = 0;
    e->size = 0;
    sector[dir_sector].directory.free++;

    printf("'%s' deleted.\n", name);
}




int main(void){


    if (sizeof(struct dirEntry) != 20 || sizeof(union block) != SECTOR_SIZE) {
         printf("Error: structure sizes are wrong (entry = %zu, block = %zu).\n", sizeof(struct dirEntry), sizeof(union block));
        return 1;
    }


    createSector(0, -1, -1);
    openInformation.isOpen = 0;

    if (!buildList()) {
        printf("Error: could not build the free-sector list.\n");
        freeList();
        return 1;
    }

    current_directory = 0;



    printf("File management system project:\n");
    printf("This file system has %d sectors with %d bytes each.\n", NUM_SECTORS, SECTOR_SIZE);



    char input[2048];
    char cmd[16];

    while (1) {


        printf("program@User:%s$ ", current_path);


        if (fgets(input, sizeof(input), stdin) == NULL) break;
        input[strcspn(input, "\n")] = '\0';
        if (sscanf(input, "%15s", cmd) != 1) continue;   // blank line, ask again

        for (int i = 0; cmd[i]; i++){
            cmd[i] = toupper((unsigned char)cmd[i]);
        }


        if (strcmp(cmd, "EXIT") == 0) {
            break;
        } else if (strcmp(cmd, "CREATE") == 0) {

            char type, name[16];
            if (sscanf(input, "%*s %c %15s", &type, name) != 2)
                printf("Usage: CREATE <U|D> <name>\n");
            else
                create(toupper((unsigned char)type), name);   


        } else if (strcmp(cmd, "DIR") == 0) {
            
            printDirectory(current_directory);     
            
        } else if (strcmp(cmd, "CD") == 0) {
            char name[16];
            if (sscanf(input, "%*s %15s", name) != 1)
                printf("Usage: CD <directory>\n");
            else{
                changeDirectory(name);
            }

        } else if (strcmp(cmd, "OPEN") == 0) {

            char mode, name[16];
            if (sscanf(input, "%*s %c %15s", &mode, name) != 2){
                printf("Usage: OPEN <I|O|U> <name>\n");
        
            } else{
                openFile(toupper((unsigned char)mode), name);
            }

        } else if (strcmp(cmd, "CLOSE") == 0) {
            closeFile();

        } else if (strcmp(cmd, "SEEK") == 0) {

            int position;
            if (sscanf(input, "%*s %d", &position) != 1) {
                printf("Usage: SEEK <position>\n");
            } else {
                seek(position);
            }

        } else if (strcmp(cmd, "WRITE") == 0) {
            int count, pos = 0;
            if (sscanf(input, "%*s %d%n", &count, &pos) != 1 || pos == 0) {
                printf("Usage: WRITE <bytes> <data>\n");
            } else {
                const char *data = input + pos;
                if (*data == ' ') data++;     
                writeFile(count, data);
            }

        } else if (strcmp(cmd, "ROOT") == 0) {

            cdRoot();

        } else if (strcmp(cmd, "READ") == 0) {

            int count;
            if (sscanf(input, "%*s %d", &count) != 1)
                printf("Usage: READ <bytes>\n");
            else
                readFile(count);

        } else if (strcmp(cmd, "DELETE") == 0) {
            
            char name[16];
            if (sscanf(input, "%*s %15s", name) != 1)
                printf("Usage: DELETE <name>\n");
            else
                deleteEntry(name);

        }else {

            printf("Invalid command.\n");
        }



    }


    //Exiting the program releases the dynamically allocated memory for the free list
    freeList();
    printf("Exited the program.\n");

    return 0;

}