#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include "malloc.h"
#include <unistd.h>
#include <pthread.h>

#define HIGH_EXPONENT 14  //2^10 = 1024 Gigabytes, because the largest block is 512
#define LOW_EXPONENT 6    // 2^5 = 32 bytes
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

pthread_mutex_t malloc_mutex = PTHREAD_MUTEX_INITIALIZER;
# define MALLOC_LOCK pthread_mutex_lock(&malloc_mutex);
# define MALLOC_UNLOCK pthread_mutex_unlock(&malloc_mutex);

typedef long long Align; // for alignment to long long (8 bytes) boundary

union ublock {
    struct {
        size_t isFree;
        size_t kval;
        union ublock *prev;
        union ublock *next;
        union ublock *fourPageBase;
        size_t size;
        pthread_t tid;
        int magic;
    } meta;
    Align x;
};

typedef union ublock block;

static block *BUDDY_HEAD = NULL;

static block *AVAIL[HIGH_EXPONENT + 1];

static block *LARGE_BLOCK_HEAD = NULL;

static unsigned MAX;

static int COUNT = 0;

static unsigned long long sbrkSize = 0;
static unsigned long long mmapSize = 0;

static unsigned long long numOfFourPage = 0;
static unsigned long long numOfMmap = 0;

static unsigned long long totalAllocationRequests = 0;
static unsigned long long totalFreeRequests = 0;

static unsigned long long totalNumberOfBlocks = 0;

void *malloc(size_t size)
{
    MALLOC_LOCK;
    totalAllocationRequests++;
    COUNT++;
    //printf("COUNT: %d\n", COUNT);

    void * ret = buddy_malloc(size);
    MALLOC_UNLOCK;
    return ret;
}

void free(void *ptr)
{
    MALLOC_LOCK;
    totalFreeRequests++;
    if (ptr != NULL) {
        buddy_free(ptr);
    }
    MALLOC_UNLOCK;
    return;
}

void *calloc(size_t nmemb, size_t size)
{
    COUNT++;
    //printf("COUNT: %d\n", COUNT);

    return buddy_calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size)
{
    COUNT++;
    //printf("COUNT: %d\n", COUNT);
    return buddy_realloc(ptr, size);

}

void buddy_init()
{
    unsigned order = HIGH_EXPONENT;
    sbrkSize += (unsigned long long)(1 << order);
    numOfFourPage++;
    //void * base = sbrk(1 << order);
    long onPpageSize = sysconf(_SC_PAGESIZE);
    void * base = sbrk(4 * onPpageSize);

    if (base < 0 || errno == ENOMEM) {
        errno = ENOMEM;
        exit(1);
    }

    // Now have order value for block size.
    MAX = order;
    BUDDY_HEAD = (block *) base;

    // Add block of max size to the mth list
    AVAIL[order] = (block *) base;
    AVAIL[order]->meta.isFree = 1;
    AVAIL[order]->meta.kval = order;
    AVAIL[order]->meta.next = NULL;
    AVAIL[order]->meta.prev = NULL;
    AVAIL[order]->meta.size = (size_t)(1 << order);
    AVAIL[order]->meta.tid = pthread_self();
    AVAIL[order]->meta.magic=123456;
    AVAIL[order]->meta.fourPageBase = BUDDY_HEAD;
}

void *buddy_calloc(size_t num, size_t size)
{
    MALLOC_LOCK;
    void *rval = buddy_malloc(num * size);

    // MUST INITIALIZE NEW DATA TO ZERO
    if (rval != NULL) {
        memset(rval, 0, num * size);
    }

    MALLOC_UNLOCK;
    return rval;
}

void *buddy_malloc(size_t size)
{
    // if the size > 512, mmap
    if (size + sizeof(block) > 512) {
        size_t allocSize = size + sizeof(block);
        void *a = mmap(0, allocSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

        mmapSize += (unsigned long long)allocSize;
        numOfMmap++;

        //printf("mmaped %d size at %p\n", (int)allocSize, a);
        block * cur = (block *)a;
        cur->meta.size = allocSize;
        cur->meta.prev = NULL;
        cur->meta.tid = pthread_self();
        cur->meta.kval = 1;
        cur->meta.magic = 654321;
        cur->meta.isFree=0;
        if (LARGE_BLOCK_HEAD == NULL){
            cur->meta.next = NULL;
            LARGE_BLOCK_HEAD = cur;
        } else {
            LARGE_BLOCK_HEAD->meta.prev = cur;
            cur->meta.next = LARGE_BLOCK_HEAD;
            LARGE_BLOCK_HEAD = cur;
        }
        void * returnPtr = a + sizeof(block);
        return returnPtr;
    } else{
        // First time BUDDY_HEAD is NULL
        if (BUDDY_HEAD == NULL){
            printf("First time BUDDY_HEAD is NULL, BUDDY_INIT");
            buddy_init();
        }

        // loop to find size of k
        unsigned k = get_k(size);

        void *rval = NULL;

        block * curr = AVAIL[k];
        while (curr != NULL && curr->meta.tid != pthread_self()){
            curr = curr->meta.next;
        }

        // if kth list is null, split on k+1 if its not the max k
        if (curr == NULL) {
            split(k + 1);
            totalNumberOfBlocks++;
        }

        block * curr2 = AVAIL[k];
        while (curr2 != NULL && curr2->meta.tid != pthread_self()) {
            curr2 = curr2->meta.next;
        }

        if (curr2 != NULL) {
            block *tmp = curr2;
            block * prev = curr2->meta.prev;
            block *next = curr2->meta.next;

            if (prev != NULL){
                prev->meta.next = next;
            } else {
                AVAIL[k] = next;
            }

            if (next != NULL) {
                next->meta.prev = prev;
            }
            tmp->meta.prev = NULL;
            tmp->meta.next = NULL;
            tmp->meta.isFree = 0;
            //tmp->meta.size = (size_t)1 << k;
            tmp++;
            rval = (void *) tmp;
        } else {
            errno = ENOMEM;
        }
        return rval;
    }
}

void *buddy_realloc(void *ptr, size_t size)
{
    MALLOC_LOCK;
    if (ptr == NULL) {
        void * ret =buddy_malloc(size);
        MALLOC_UNLOCK;
        return ret;
    }

    if (size == 0) {
        buddy_free(ptr);
        MALLOC_UNLOCK;
        return ptr;
    }
    // this should

    void *rval;

    block *TMP = (block *) ptr;
    TMP--;

//    size_t oldSize = (1 << TMP->meta.kval) - sizeof(block);
    size_t oldSize = TMP->meta.size - sizeof(block);

    if (TMP->meta.magic != 123456 && TMP->meta.magic!=654321){
        rval = buddy_malloc(size);
        MALLOC_UNLOCK;
        return rval;
    }

    if (oldSize != size) {
        // Get the smaller size
        rval = buddy_malloc(size);
        size_t cpySize;
        if (oldSize < size) {
            cpySize = oldSize;
        }
        else {
            cpySize = size;
        }

        memcpy(rval, ptr, cpySize);
        //printf("BEFORE BUDDY_FREE, AFTER MEMCPY\n");
        buddy_free(ptr);
    } else {
        rval = ptr;
    }
    MALLOC_UNLOCK;
    //printf("**************IN REALLOC DONE**************\n");

    return rval;
}

void buddy_free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    // if the size > 512, unmmap
    block *temp = (block *)(ptr - sizeof(block));
    //if (temp->meta.size > 512) {
    if (temp->meta.magic == 654321 && temp->meta.isFree == 0) {
        block *prev = temp->meta.prev;
        block *next = temp->meta.next;

        if (prev == NULL) {
            LARGE_BLOCK_HEAD = next;
        } else {
            prev->meta.next = next;
        }

        if (next != NULL) {
            next->meta.prev = prev;
        }

        temp->meta.isFree = 1;
        temp->meta.prev = NULL;
        temp->meta.next = NULL;
        munmap((void *)temp, temp->meta.size);
        return;
    } else if (temp->meta.magic == 123456 && temp->meta.isFree == 0) {
        if (temp->meta.isFree) {
            perror("Attempting to free already freed pointer\n");
            return;
        }

        size_t buddy = 0;
        block *BUD = NULL;
        size_t k = temp->meta.kval;

        if (k < MAX) {

            BUD = (block *) ((((size_t) temp - (size_t)temp->meta.fourPageBase) ^ ((size_t)(1 << k)))
                             + (size_t)temp->meta.fourPageBase);
            //printf("b_free: Bud->tag=%u, k=%u, &=%p\n", BUD->tag, BUD->kval, BUD);
            //printf("b_free: tmp=%zx bud=%zx k=%u\n", (size_t)TMP - (size_t) BUDDY_HEAD ,(size_t) BUD - (size_t) BUDDY_HEAD, k);

            // MIRACLE WORKER
            if (k == BUD->meta.kval)
                buddy = BUD->meta.isFree;
        }
        if (buddy) {
            block *PREV = BUD->meta.prev;
            block *NEXT = BUD->meta.next;

            if (PREV == NULL) {
                //printf("b_free: BUD PREV was NULL\n");
                AVAIL[k] = BUD->meta.next;
            } else {
                PREV->meta.next = BUD->meta.next;
            }
            if (NEXT != NULL) {
                NEXT->meta.prev = BUD->meta.prev;
            }

            block * head;
            // Find the address that is smallest
            if (BUD < temp)
                head = BUD;
            else
                head = temp;

            k++;
            head->meta.kval = k;
            head->meta.isFree = 0;
            head->meta.size = (size_t)(1<<k);
            head++;

            buddy_free((void *) head);
            totalNumberOfBlocks--;
        } else {
            //printf("b_free: no buddy\n");
            if (AVAIL[k] == NULL){
                temp->meta.prev = NULL;
                AVAIL[k] = temp;
            }
            else {
                block *curr = AVAIL[k];
                while (curr->meta.next != NULL) {
                    curr = curr->meta.next;
                }
                temp->meta.prev = curr;
                curr->meta.next = temp;
            }
            temp->meta.kval = k;
            temp->meta.next = NULL;
            temp->meta.isFree = 1;
            temp->meta.size = (size_t)(1 << k);
        }

        return;
    }
}

unsigned get_k(size_t size)
{
    unsigned k = LOW_EXPONENT;

    size_t calcSize = (1 << k) - sizeof(block);
    while (size > calcSize) {
//    while (size > calcSize && k < HIGH_EXPONENT) {
        calcSize = ((size_t) 1 << ++k) - sizeof(block);
    }
    return k;
}

/*
 *
 * Split a node.
 *
 */
void split(unsigned k)
{
    block * curr = AVAIL[k];
    while (curr != NULL && curr->meta.tid != pthread_self()) {
        curr = curr->meta.next;
    }

    if (curr == NULL && k + 1 <= HIGH_EXPONENT) {
        split(k + 1);
        totalNumberOfBlocks++;
    } else if (curr == NULL && k + 1 > HIGH_EXPONENT) {

        // Allocate 4 more pages
        sbrkSize+= (unsigned long long)(1 << HIGH_EXPONENT);
        numOfFourPage++;

        unsigned m = HIGH_EXPONENT;
//        void * ptr = sbrk(1 << m);
        long onePageSize = sysconf(_SC_PAGESIZE);
        void * ptr = sbrk(4 * onePageSize);

        if (ptr < 0 || errno == ENOMEM) {
            errno = ENOMEM;
            exit(1);
        }

        block * cur = ptr;

        // Put the new 4 pages in the front of AVAIL[m]
        if (AVAIL[m] == NULL){
            cur->meta.next = NULL;
            AVAIL[m] = cur;
        } else {
            AVAIL[m]->meta.prev = cur;
            cur->meta.next = AVAIL[m];
            AVAIL[m] = cur;
        }

        //AVAIL[m] = (block *)ptr;
        AVAIL[m]->meta.isFree = 1;
        AVAIL[m]->meta.kval = m;
        AVAIL[m]->meta.next = NULL;
        AVAIL[m]->meta.prev = NULL;
        AVAIL[m]->meta.size = (size_t)(1 << m);
        AVAIL[m]->meta.tid = pthread_self();
        AVAIL[m]->meta.magic=123456;
        AVAIL[m]->meta.fourPageBase = (block *)ptr;
    }

    block * curr2 = AVAIL[k];
    while (curr2 != NULL && curr2->meta.tid != pthread_self()) {
        curr2 = curr2->meta.next;
    }

    // we can split now
    if (curr2 != NULL) {
        //      printf("split: AVAIL[k] isn't NULL\n");
        // ???? tmp, curr2 why create two?
        block *tmp = curr2;
        block *PREV = curr2->meta.prev;
        block *NEXT = curr2->meta.next;

        if (PREV == NULL) {
            // Remove current by moving the pointer to the next node
            AVAIL[k] = NEXT;
        } else {
            PREV->meta.next = NEXT;

            //?????
//            NEXT->meta.prev = PREV;
        }

//        if (NEXT == NULL) {
//            PREV->meta.next = AVAIL[k];
//        }
        if (NEXT != NULL) {
            NEXT->meta.prev = PREV;
        }

        tmp->meta.prev = NULL;
        tmp->meta.next = NULL;

        block *list = AVAIL[k-1];
        if (list != NULL) {
            while (list->meta.next != NULL) {
                list = list->meta.next;
            }
            list->meta.next = tmp;
            tmp->meta.prev = list;
        } else {
            AVAIL[k-1] = tmp;
            tmp->meta.prev = NULL;
        }

        tmp->meta.isFree = 1;
        tmp->meta.kval = k - 1;
        tmp->meta.size = (size_t)(1 << (k - 1));
        //      (block*) ((((void*)TMP - BUDDY_HEAD) + ((size_t)1<<k)) + BUDDY_HEAD);

        // ????
        block *SEC = (block *) ((size_t) tmp + ((size_t)(1 << (k - 1))));
        tmp->meta.next = SEC;
        SEC->meta.isFree = 1;
        SEC->meta.kval = k - 1;
        SEC->meta.next = NULL;
        SEC->meta.prev = tmp;
        SEC->meta.size = (size_t)(1 << (k - 1));
        SEC->meta.tid = tmp->meta.tid;
        SEC->meta.magic=123456;
        SEC->meta.fourPageBase=tmp->meta.fourPageBase;
    }
    return;
}

void malloc_stats() {
    block *curr;
    for (int k = 0; k <= HIGH_EXPONENT; k++) {
        printf("Printing AVAIL[%d]\n", k);
        if (AVAIL[k] != NULL) {
            int i=0;
            curr = AVAIL[k];
            while(curr != NULL) {
                printf("the free node %d, free is %zu, the size is %zu, tid: %lu\n",i,curr->meta.isFree,curr->meta.size, curr->meta.tid);
                curr = curr->meta.next;
                i++;
            }
        } else {
            printf("the size %d is empty\n",k);
        }
    }
    printf("from %p to %p, the page total size is %llu\n", BUDDY_HEAD, sbrk(0), (long long unsigned)sbrk(0) - (long long unsigned) BUDDY_HEAD);
    curr = LARGE_BLOCK_HEAD;
    while(curr!=NULL){
        printf("curr size is %zu,free=%zu, tid: %lu\n",curr->meta.size,curr->meta.isFree, curr->meta.tid);
        curr = curr->meta.next;
    }
    printf("Current thread ID: %lu\n", pthread_self());
    //void * currAddr = sbrk(0);
    //printf("Total size of arena allocated with sbrk: %llu\n", (unsigned long long) (currAddr - (void *)BUDDY_HEAD));
    //printf("Total size of arena allocated with mmap: %llu\n", (unsigned long long) (currAddr - (void *)LARGE_BLOCK_HEAD));
    printf("Total number of sbrk bins: %llu\n", numOfFourPage);
    printf("Total number of mmap bins: %llu (double counting the size that gets realloced)\n", numOfMmap);
    printf("Total size of sbrk: %llu\n", sbrkSize);
    printf("Total size of mmap: %llu\n", mmapSize);
    printf("Total allocation requests: %llu\n", totalAllocationRequests);
    printf("Total free requests: %llu\n", totalFreeRequests);
    printf("Total number of blocks: %llu\n", totalNumberOfBlocks);
}