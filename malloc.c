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
        size_t size;
        pthread_t tid;
        int magic;
    } meta;
    Align x;
};

typedef union ublock block;

static block *BASE = NULL;

static block *AVAIL[HIGH_EXPONENT + 1];

static block * ublockHead = NULL;

static unsigned MAX;

static int COUNT = 0;

void *malloc(size_t size)
{
    MALLOC_LOCK;
    COUNT++;
    printf("COUNT: %d\n", COUNT);

    void * ret = buddy_malloc(size);
    MALLOC_UNLOCK;
    return ret;
}

void free(void *ptr)
{
    //printf("interposed: buddy_free\n");
    MALLOC_LOCK;
    if (ptr != NULL) {
        buddy_free(ptr);
    }
    MALLOC_UNLOCK;
    return;
}

void *calloc(size_t nmemb, size_t size)
{
    COUNT++;
    printf("COUNT: %d\n", COUNT);
    //printf("interposed: buddy_calloc\n");

    return buddy_calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size)
{
    COUNT++;
    printf("COUNT: %d\n", COUNT);
    //printf("interposed: buddy_realloc\n");
    return buddy_realloc(ptr, size);

}

void buddy_init()
{
    unsigned order = HIGH_EXPONENT;
    void * base = sbrk(1 << order);
    if (base < 0 || errno == ENOMEM) {
        errno = ENOMEM;
        exit(1);
    }

    // Now have k value for block size.
    MAX = order;
    BASE = (block *) base;

    // Add block of max size to the mth list
    AVAIL[order] = (block *) base;
    AVAIL[order]->meta.isFree = 1;
    AVAIL[order]->meta.kval = order;
    AVAIL[order]->meta.next = NULL;
    AVAIL[order]->meta.prev = NULL;
    AVAIL[order]->meta.size = (size_t)(1 << order);
    AVAIL[order]->meta.tid = pthread_self();
    AVAIL[order]->meta.magic=123456;
}

void *buddy_calloc(size_t num, size_t size)
{
    MALLOC_LOCK;
    printf("**************IN CALLOC**************\n");
    if (num == 0 || size == 0) {
        MALLOC_UNLOCK;
        return NULL;
    }

    // allocate block of memory num * size bytes
    //if (BASE == NULL)
    //    buddy_init((size_t) (size * num));

//    MALLOC_UNLOCK;
    void *rval = buddy_malloc(num * size);

//    MALLOC_LOCK;
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
//    MALLOC_LOCK;
    printf("**************IN MALLOC**************\n");
    if (size + sizeof(block) > 512) {
        size_t allocSize = size + sizeof(block);
        void *a = mmap(0, allocSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        printf("mmaped %d size at %p\n", (int)allocSize, a);
        block * cur = (block *)a;
        cur->meta.size = allocSize;
        cur->meta.prev = NULL;
        cur->meta.tid = pthread_self();
        cur->meta.kval = 1;
        cur->meta.magic = 654321;
        if (ublockHead == NULL){
            cur->meta.next = NULL;
            ublockHead = cur;
        }
        else {
            ublockHead->meta.prev = cur;
            cur->meta.next = ublockHead;
            ublockHead = cur;
        }
        void * returnPtr = a + sizeof(block);
//        MALLOC_UNLOCK;
        printf("after mmap!!!!!!!!!!!!!!!!!!!!!****************\n");
        return returnPtr;
    } else{
        // First time BASE is NULL
        if (BASE == NULL){
            printf("First time BASE is NULL, BUDDY_INIT");
            buddy_init();
        }

        // loop to find size of k
        unsigned k = get_k(size);
//    printf("b_malloc:  k= %u\n", k);

        void *rval = NULL;

        block * curr = AVAIL[k];
        while (curr != NULL && curr->meta.tid != pthread_self()){
            curr = curr->meta.next;
        }

        // if kth list is null, split on k+1 if its not the max k
//        if (k < MAX && curr == NULL) {
        if (curr == NULL) {
            printf("b_malloc: Splitting on %u\n", k+1);
            split(k + 1);
        }

        block * curr2 = AVAIL[k];
        while (curr2 != NULL && curr2->meta.tid != pthread_self()) {
            curr2 = curr2->meta.next;
        }

//        if (k <= MAX && AVAIL[k] != NULL) {
        if (curr2 != NULL) {
            //      printf("b_malloc: AVAIL[%u] not null\n", k);
//              printf("ours!!! b_malloc: AVAIL[%d] tag=%d\n", (int)AVAIL[k]->meta.kval , (int)AVAIL[k]->meta.tag);

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
            //      perror("Couldn't allocate memory, in buddy_malloc");
        }
        printf("b_malloc: End of bmalloc\n");
//    MALLOC_UNLOCK;
        return rval;
    }
}

void *buddy_realloc(void *ptr, size_t size)
{
    MALLOC_LOCK;
    printf("**************IN REALLOC************** size: %d\n", (int)size);
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
    // this should never be the case, but we'll take it
//    if (BASE == NULL) {
//        buddy_init(size);
//        printf("this should never be the case, but we'll take it");
//    }

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
        printf("BEFORE BUDDY_FREE, AFTER MEMCPY\n");
        buddy_free(ptr);
    } else {
        rval = ptr;
    }
    MALLOC_UNLOCK;
    printf("**************IN REALLOC DONE**************\n");

    return rval;
}

void buddy_free(void *ptr)
{
//    MALLOC_LOCK;
    if (ptr == NULL)
    {
//        MALLOC_UNLOCK;
        return;
    }
    //printf("b_free: Start of free\n");

    // if the size > 512, unmmap
    block *temp = (block *)(ptr - sizeof(block));
    //if (temp->meta.size > 512) {
    if (temp->meta.magic == 654321 && temp->meta.isFree == 0) {
        block *prev = temp->meta.prev;
        block *next = temp->meta.next;

        if (prev == NULL) {
            ublockHead = next;
        } else {
            prev->meta.next = next;
        }

        if (next != NULL) {
            next->meta.prev = temp->meta.prev;
        }

        munmap(temp, temp->meta.size);
//        MALLOC_UNLOCK;
        return;
    } else if (temp->meta.magic == 123456 && temp->meta.isFree == 0) {
//        block *temp = (block *) ptr;
//        temp--;

        if (temp->meta.isFree) {
            perror("Attempting to free already freed pointer\n");
            //      exit(1);
            return;
        }
        //printf("b_free: Block->tag=%u, k=%u, &=%p\n", TMP->tag, TMP->kval, TMP);

        size_t buddy = 0;
        block *BUD = NULL;
        size_t k = temp->meta.kval;

        if (k < MAX) {

            BUD = (block *) ((((size_t) temp - (size_t) BASE) ^ ((size_t)(1 << k)))
                             + (size_t) BASE);
            //printf("b_free: Bud->tag=%u, k=%u, &=%p\n", BUD->tag, BUD->kval, BUD);
            //printf("b_free: tmp=%zx bud=%zx k=%u\n", (size_t)TMP - (size_t) BASE ,(size_t) BUD - (size_t) BASE, k);

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

            //printf("b_free: Buddy Available!\n");

            block * head;
            // Find the address that is smallest
            if (BUD < temp)
                head = BUD;
            else
                head = temp;
            //printf("b_free: Mask=%zx, k=%u\n",((size_t) 1 << (TMP->kval)), TMP->kval);
            //printf("b_free: Merged&=%p k=%u, T&=%p, B&=%p k=%u\n", MERGE, k+1, TMP, BUD, k);

            k++;
            head->meta.kval = k;
            head->meta.isFree = 0;
            head->meta.size = (size_t)(1<<k);
            head++;

            buddy_free((void *) head);

        } else {
            //printf("b_free: no buddy\n");
            if (AVAIL[k] == NULL){
                temp->meta.prev = NULL;
                AVAIL[k] = temp;
            }
            else {
                block *curr = AVAIL[k];
                //printf("b_free: curr=%p k=%u, TMP&=%p k=%u\n", curr, k, TMP, TMP->kval);
//            if (curr != NULL) {
                while (curr->meta.next != NULL) {
                    curr = curr->meta.next;
                }
                temp->meta.prev = curr;
                curr->meta.next = temp;
//            } else {
//                AVAIL[k] = TMP;
//            }
            }
            temp->meta.kval = k;
            temp->meta.next = NULL;
            temp->meta.isFree = 1;
            temp->meta.size = (size_t)(1 << k);
        }

        //printf("b_free: End of buddy free\n");
//    MALLOC_UNLOCK;
        return;
    }
}

/*
 * Returns the k for that size
 */
unsigned get_k(size_t size)
{
    //printf("get_k: Start of getk\n");
    unsigned k = LOW_EXPONENT;

    size_t calcSize = (1 << k) - sizeof(block);
    //if (!calcSize) printf ("get_k: k is too small\n");
    //printf("actualSize = %llu, hex = 0x%llx \n", actualSize, actualSize);
    while (size > calcSize) {
//    while (size > calcSize && k < HIGH_EXPONENT) {
        calcSize = ((size_t) 1 << ++k) - sizeof(block);
        //      printf("calcSize = %lu\n", calcSize);
    }
    //printf("get_k: End of getk\n");
    return k;
}

/*
 *
 * Split a node.
 *
 */
void split(unsigned k)
{
    //printf("split: Start of split k=%u\n", k);
    block * curr = AVAIL[k];
    while (curr != NULL && curr->meta.tid != pthread_self()) {
        curr = curr->meta.next;
    }

    if (curr == NULL && k + 1 <= HIGH_EXPONENT) {
        split(k + 1);
    } else if (curr == NULL && k + 1 > HIGH_EXPONENT) {

        // Allocate 4 more pages
        printf("************************************************\n");
        printf("Allocate 4 more pages\n");
        printf("************************************************\n");
        unsigned m = HIGH_EXPONENT;
        void * ptr = sbrk(1 << m);
        while (ptr < 0 || errno == ENOMEM) {
//        m--;
//        BASE = sbrk(1UL << m);
//
//        if (BASE < 0
//            && (size > ((1UL << m) - sizeof(block)) || m < LOW_EXPONENT)) {
//            errno = ENOMEM;
//            exit(1);
//        }
            errno = ENOMEM;
            exit(1);
        }


        // Add block of max size to the mth list
        AVAIL[m] = (block *)ptr;
        AVAIL[m]->meta.isFree = 1;
        AVAIL[m]->meta.kval = m;
        AVAIL[m]->meta.next = NULL;
        AVAIL[m]->meta.prev = NULL;
        AVAIL[m]->meta.size = (size_t)(1 << m);
        AVAIL[m]->meta.tid = pthread_self();
        AVAIL[m]->meta.magic=123456;
    }

    block * curr2 = AVAIL[k];
    while (curr2 != NULL && curr2->meta.tid != pthread_self()) {
        curr2 = curr2->meta.next;
    }


    // we can split now
    if (curr2 != NULL) {
        //      printf("split: AVAIL[k] isn't NULL\n");
        block *tmp = curr2;
        block *PREV = curr2->meta.prev;
        block *NEXT = curr2->meta.next;

        if (PREV == NULL) {
            //printf("b_free: BUD PREV was NULL\n");
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
        //      (block*) ((((void*)TMP - BASE) + ((size_t)1<<k)) + BASE);
        block *SEC = (block *) ((size_t) tmp + ((size_t)(1 << (k - 1))));
        //      printf("split: tmp=%p sex=%p from %u to %u\n", tmp, SEC, k, k-1);
        //      printf("split: tmp=%zx sex=%zx\n", (size_t)tmp - (size_t) BASE ,(size_t) SEC - (size_t) BASE);
        tmp->meta.next = SEC;
        SEC->meta.isFree = 1;
        SEC->meta.kval = k - 1;
        SEC->meta.next = NULL;
        SEC->meta.prev = tmp;
        SEC->meta.size = (size_t)(1 << (k - 1));
        SEC->meta.tid = tmp->meta.tid;
        SEC->meta.magic=123456;
        printf("Split: AVAIL[%u] &=%p, AVAIL[%u]->next &%p\n", k-1, AVAIL[k-1], k-1,  SEC);
    }
    printf("Split: Finished split at k= %u\n", k);
    return;
}


