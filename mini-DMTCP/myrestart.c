//
// Created on 1/19/16.
//

#include "myrestart.h"
#include "ckpt.c"

/*
 * The myrestart.c program should take one argument, the name
        of the checkpoint image file from which to restore the process.
Now copy that filename from the stack (probably from 'argv[1]')
into your data segment (probably into a global variable).
(We will soon be unmapping the initial stack.  So, we better
first save any important data.)  When you copy the filename
        into allocated storage in your data segment, an easy way
is to declare a global variable (e.g., 'char ckpt_image[1000];')
 */
char ckpt_image[1000];

ucontext_t reg;
struct header_struct header_structs[MAX_HEADER_NUM];

void freeCurrentStackAndRestoreMemory(char *ckpt_image)
{
    // Now remove the original stack of myrestart, in case the checkpoint
    // image will also be mapping some memory into that location.
    printf("Before freeing the current stack\n");
    freeCurrentStack();
    printf("Before restoring the memory\n");
    restore_memory(ckpt_image);
}

void freeCurrentStack()
{
    printf("Start reading headers from maps in myrestart.c\n");

    int i = getHeaders();

    printf("Finish reading headers from maps in myrestart.c\n\n");

    printf("Start initializing structs from headers\n");

    int counter = 0;

    int procs [] = {PROT_READ, PROT_WRITE, PROT_EXEC};
    char temp[MAX_HEADER_LENGTH];
    for (int j = 0; j < i; j++) {
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
                unsigned long long start = readhex(strtok(address_range, "-"));
                unsigned long long len = readhex(strtok(NULL, "-")) - header_structs[j].mem_start;
                // release memory
                printf("Release memory\n");
                // Remove the original stack of myrestart
                munmap((void *)start, (size_t)len);
                printf("Finishing releasing memory\n");
                break;
            }
        }

        printf("Length: %llu\n", header_structs[j].mem_len);

        printf("Counter: %d\n", counter);
        counter++;
    }
}

/*
 * Copy the data from the memory sections of your checkpoint image
 * into the corresponding memory sections.
 */
void restore_memory(char * path)
{
    printf("Start loading %s in myrestart.c\n", path);

    FILE * ckpt = fopen(path, "r");
    FILE * ckpt_reg = fopen("ckpt_reg", "r");

    if(fread(&reg, sizeof(reg), 1, ckpt_reg) < 1)
    {
        printf("Unexpected exit 1\n");
        exit(1);
    }

    int i = 0;
    while(1){
        printf("In the while loop in myrestart.c: %d\n", i);
        // Copy the header of your checkpoint image file into the struct
        size_t ret_code =fread((header_structs+i), sizeof(struct header_struct), 1, ckpt);
        if( (ferror(ckpt)) || (feof(ckpt)) ){
            printf("Exit 2\n");
            break;
        }

        if ( ret_code == sizeof(struct header_struct))
        {
            if (mprotect((header_structs+i), sizeof(struct header_struct), PROT_READ) == -1)
                perror("mprotect error\n");
        }

        void* addr;
        //Call mmap to map into memory a corresponding memory section for a new process.
        addr = mmap((void*)(header_structs+i)->mem_start, (header_structs+i)->mem_len, PROT_READ|PROT_EXEC|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED , -1, 0);
        printf("start: %llu, length: %llu, permission: %d\n", (header_structs+i)->mem_start, (header_structs+i)->mem_len, (header_structs+i)->permission);

        if ( addr == MAP_FAILED ) {
            printf("Unexpected exit 3\n");
            perror("mmap error. " );
            exit(1);
        }
        // Copy the data from the memory sections of checkpoint image into the corresponding memory sections
        fread((void*)(header_structs+i)->mem_start, (header_structs+i)->mem_len, 1, ckpt);
        i++;
        printf("counter: %d\n", i);
    }

    printf("Leave loop in myrestart.c\n");

    // Jump into the old program and restore the old registers.
    setcontext(&reg);
    fclose(ckpt);
    fclose(ckpt_reg);
}

int main(int argc, char* argv[])
{
    printf("MYRESTART\n");

    // First read the current process's maps (process of restart) because
    // we are going to change the stack pointer to a new address
    readSelfMaps();

    // Map in some new memory for a new stack, in the range 0x5300000 - 0x5301000
    mmap((void *)0x5300000, 0x100000, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);

    strcpy(ckpt_image, argv[1]);
    printf("argv[1]: %s\n", ckpt_image);

    printf("Before mapping a new memory\n");
    void * stack_ptr = (void *)(0x5300000 + 0x90000);

    // Use the inline assembly syntax of gcc to switch the stack pointer
    // (syntax: $sp or %sp) from the initial stack to the new stack
    printf("Before running the assembly code to switch the stack pointer\n");
    asm volatile ("mov %0,%%rsp;" : : "g" (stack_ptr) : "memory");

    printf("After switching the stack pointer\n");
    freeCurrentStackAndRestoreMemory(ckpt_image);
}