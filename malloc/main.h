#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <math.h>

#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#define PAGE_K log2(PAGE_SIZE) // 2^12 = 4096 bytes = 1 Page size
#define LOW_EXPONENT 6		// 2^6 = 64 bytes > block size + 8 bytes
#define PAGE_NUMBER 4

typedef struct bucket {    // size of is 168 bytes
    struct block * freelist[12 + PAGE_NUMBER + 1]; // ????????? to change???????????
    long kval;
    long max_avail_k;
    struct bucket * prev;
    struct bucket * next;
} bucket;

typedef struct block {		// size of is 32 bytes
    long unused;
    long kval;
    struct block * prev;
    struct block * next;
} block;

static bucket * BASE = NULL;

static unsigned MAX;

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);