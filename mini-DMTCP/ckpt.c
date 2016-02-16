//
// Created on 1/19/16.
//

//#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <errno.h>
#include "ckpt.h"

// MAX_HEADER_NUM is usually 20, but could go over
#define MAX_HEADER_NUM 30
#define MAX_HEADER_LENGTH 100
#define MAPS_MAX MAX_HEADER_NUM * MAX_HEADER_LENGTH

typedef enum {stack = 10, vsyscall = 20, context = 50, others = 100} header_type;

char maps[MAPS_MAX];

const char SPACE[2] = " ";

// Array of headers in the map
char* headers[MAX_HEADER_NUM];

// Create a struct to store the register
ucontext_t reg;

int saveckpt = 0;

struct header_struct
{
    header_type h_type;
    unsigned long long mem_start;
    unsigned long long mem_len;
    int permission;
};

//
struct header_struct header_structs[MAX_HEADER_NUM];



/*
 * Read the current process's maps (which is the memory layout
 *   of the current process)
 */
char * readSelfMaps()
{
    printf("Start reading current process's maps\n");

    FILE * fp = fopen("/proc/self/maps", "r");
    char header[MAX_HEADER_LENGTH];

    if (fp)
    {
        while (fgets(header, MAX_HEADER_LENGTH, fp) != NULL)
        {
            // when copying the header, since there is a \n in the end of the header
            // so every header will be separted by the end character
            // maps + strlen(maps) is the end address of maps
            strcpy(maps + strlen(maps), header);
        }
        printf("maps: \n%s", maps);
        fclose(fp);
    }

    printf("Finish reading current process's maps\n\n");
    return maps;
}


/*
 * Get the headers of the maps and save it to headers
 * Normally, the max number of headers is 20, but could go over
 */
int getHeaders()
{
    printf("Start reading headers from maps\n");

    int i = 1;
    char * maps_copy = (char *)malloc(strlen(maps) + 1);
    maps_copy = strncpy(maps_copy, maps, strlen(maps));
    maps_copy[strlen(maps)] = '\0';
    headers[0] = strtok(maps_copy, "\n");
    char * header;
    while((header = strtok(NULL, "\n")) != NULL)
    {
        headers[i] = header;
        i++;
    }
    headers[i] = NULL;

    printf("Print out the headers:\n");
    for (int j = 0; j < i; j++) {
        printf("%d, %s\n", j, headers[j]);
    }

    free(maps_copy);

    printf("Finish reading headers from maps\n\n");
    return i;
}

int initStructs(int header_num)
{
    printf("Start initializing structs from headers\n");

    int counter = 0;

    int procs [] = {PROT_READ, PROT_WRITE, PROT_EXEC};
    char temp[MAX_HEADER_LENGTH];
    for (int j = 0; j < header_num; j++) {
//        printf("Before copying the header to temp\n");
        memset(temp, 0, MAX_HEADER_LENGTH);
        strncpy(temp, headers[j], MAX_HEADER_LENGTH);

        // Address range
        char * address_range = strtok(temp, SPACE);

        // Permissions
        char * permissions = strtok(NULL, SPACE);
        int protection = 0;
        for (int k = 0; k < 3; ++k) {
            if (permissions[k] != '-') {
                protection |= procs[k];
            }
        }
        (header_structs+j)->permission = protection;

        // Skip offset
        strtok(NULL, SPACE);
        // Skip dev
        strtok(NULL, SPACE);
        // Skip inode
        strtok(NULL, SPACE);
        // Path
        char * path = strtok(NULL, SPACE);
        if (path != NULL)
        {
            if (strcmp(path, "[stack]") == 0) {
                (header_structs+j)->h_type = stack;
            }
            else if (strcmp(path, "[vsyscall]") == 0) {
//                (header_structs+j)->h_type = vsyscall;
                continue;
            }
            else {
                (header_structs+j)->h_type = others;
            }
        }
        else{
            (header_structs+j)->h_type = others;
        }

        (header_structs+j)->mem_start = readhex(strtok(address_range, "-"));
        (header_structs+j)->mem_len = readhex(strtok(NULL, "-")) - header_structs[j].mem_start;
        printf("Length: %llu\n", header_structs[j].mem_len);

        printf("Counter: %d\n", counter);
        counter++;
    }

//    Set vsyscall to NULL
//    (header_structs+counter - 1) = NULL;

    for (int l = 0; l < counter; l++) {
        printf("%d %llu %llu %d %d\n",l, (header_structs + l)->mem_start, (header_structs + l)->mem_len,
               (header_structs + l)->permission, (header_structs + l)->h_type);
    }

    printf("counter: %d\n", counter);
    printf("Finish initializing structs from headers\n\n");

    return counter;
}

void saveToDisk(char * file_name, int struct_num)
{
    printf("Start saving structs to disk\n");

    FILE *file = fopen(file_name, "w");
    for (int i = 0; i < struct_num; ++i) {
        if ((header_structs+i)->permission & PROT_READ){
            fwrite((header_structs+i), sizeof(struct header_struct), 1, file);
            fwrite((void *)(header_structs+i)->mem_start, (header_structs+i)->mem_len, 1, file);
        }
    }

    printf("%s saved to disk\n", file_name);
    fclose(file);
    saveckpt = 1;
    getcontext(&reg);
    printf("go to getcontext\n");
    if(saveckpt==1) {
        FILE *fp = fopen("ckpt_reg", "w");
        fwrite(&reg, sizeof(reg), 1, fp);
        printf("register saved\n");
        fclose(fp);
    }

    printf("Finish saving structs to disk\n\n");
}

//int main()
void checkpoint()
{
    readSelfMaps();
    int header_num = getHeaders();
    int structs_num = initStructs(header_num);
    saveToDisk("myckpt", structs_num);

//    return 0;
}

/*
 * This is a simple way to trigger the checkpoint.  We wrote a signal
     handler for SIGUSR2, that will initiate all of the previous steps.
     [ To trigger a checkpoint, you now just need to do from a command line:
         kill -12 PID
       where PID is the process id of the target process that you wish
       to checkpoint.  (Signal 12 is SIGUSR2, reserved for end users.]
 */
__attribute__ ((constructor)) void init_signal() {
    signal(SIGUSR2, checkpoint);
}

unsigned long long readhex (char *value)
{
    unsigned long long v;
    v = 0;
    int counter = 0;
    char c;
    while (1) {
        c =  *(value + counter);
        if ((c >= '0') && (c <= '9')) c -= '0';
        else if ((c >= 'a') && (c <= 'f')) c -= 'a' - 10;
        else if ((c >= 'A') && (c <= 'F')) c -= 'A' - 10;
        else break;
        v = v * 16 + c;
        counter += 1;
    }
    return v;
}