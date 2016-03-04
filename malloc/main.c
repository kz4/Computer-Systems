#include "main.h"

void buddy_init(size_t size, bucket * * curr)
{
    unsigned pn = PAGE_NUMBER;
    * curr = (bucket *) mmap(sbrk(0), pn * PAGE_SIZE + sizeof(bucket), PROT_READ|PROT_EXEC|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED , -1, 0);
    while (curr < 0 || errno == ENOMEM) {
        pn = pn >> 1;
        if (pn != 0) {
            * curr = (bucket *) mmap(sbrk(0), pn * PAGE_SIZE + sizeof(bucket), PROT_READ|PROT_EXEC|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED , -1, 0);
        } else {
            errno = ENOMEM;
            exit(1);
        }
    }

    // Now have k value for block size.
    long maxk = pn + (long) PAGE_K;

    // Create a new bucket
    bucket * b;

    // Add block of max size to the mth list
    (* curr)->freelist[maxk] = (block *) (curr + 1);
    (* curr)->kval = maxk;
    (* curr)->max_avail_k = maxk;
    (* curr)->next = NULL;
    (* curr)->prev = NULL;

    (* curr)->freelist[maxk]->unused = 1;
    (* curr)->freelist[maxk]->kval = maxk;
    (* curr)->freelist[maxk]->next = NULL;
    (* curr)->freelist[maxk]->prev = NULL;
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
    while (size > calcSize && k < PAGE_K + PAGE_NUMBER) {
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
void split(unsigned k, bucket * * curr)
{
    //printf("split: Start of split k=%u\n", k);

    if ((* curr)->freelist[k] == NULL && k + 1 <= (* curr)->kval) {
        //      printf("split: splitting on %u\n", k+1);
        split(k + 1, curr);
    }
    if ((* curr)->freelist[k] != NULL) {
        //      printf("split: AVAIL[k] isn't NULL");
        block *temp = (* curr)->freelist[k];
        (* curr)->freelist[k] = (* curr)->freelist[k]->next;
        if ((* curr)->freelist[k] != NULL) {
            (* curr)->freelist[k]->prev = NULL;
        } else if (k == (* curr)->max_avail_k) {
            (* curr)->max_avail_k = k - 1;
        }
        (* curr)->freelist[k - 1] = temp;
        (* curr)->freelist[k - 1]->unused = 1;
        (* curr)->freelist[k - 1]->kval = k - 1;
        (* curr)->freelist[k - 1]->prev = NULL;
        //      (block*) ((((void*)TMP - BASE) + ((size_t)1<<k)) + BASE);
        block *sec = (block *) (((size_t) (* curr)->freelist[k - 1]) + ((size_t) 1 << (k - 1)));
        //      printf("split: tmp=%p sex=%p from %u to %u\n", tmp, SEC, k, k-1);
        //      printf("split: tmp=%zx sex=%zx\n", (size_t)tmp - (size_t) BASE ,(size_t) SEC - (size_t) BASE);
        (* curr)->freelist[k - 1]->next = sec;
        sec->unused = 1;
        sec->kval = k - 1;
        sec->next = NULL;
        sec->prev = (* curr)->freelist[k - 1];
        //      printf("Split: AVAIL[%u] &=%p, AVAIL[%u]->next &%p\n", k-1, AVAIL[k-1], k-1,  SEC);
    }
    //printf("Split: Finished split at k= %u\n", k);
}

void *buddy_malloc(size_t size)
{
    // loop to find size of k
    unsigned k = get_k(size);
    //printf("b_malloc:  k= %u\n", k);
    bucket * prev_bucket = BASE;
    bucket * curr_bucket = BASE;

    while (curr_bucket != NULL) {
        if (curr_bucket->max_avail_k >= k) {
            break;
        } else {
            prev_bucket = curr_bucket;
            curr_bucket = curr_bucket->next;
        }
    }

    if (curr_bucket == NULL) {
        buddy_init((size_t) size, &curr_bucket);
        if (BASE == NULL) {
            BASE = curr_bucket;
        } else {
            curr_bucket->prev = prev_bucket;
            prev_bucket->next = curr_bucket;
        }
    }

    void *rval = NULL;

    // if kth list is null, split on k+1 if its not the max k
    if (k < curr_bucket->kval && curr_bucket->freelist[k] == NULL) {
        //      printf("b_malloc: Splitting on %u\n", k+1);
        split(k + 1, &curr_bucket);
    }
    if (k <= curr_bucket->kval && curr_bucket->freelist[k] != NULL) {
        //      printf("b_malloc: AVAIL[%u] not null\n", k);
        //      printf("b_malloc: AVAIL[%u] tag=%u\n", AVAIL[k]->kval , AVAIL[k]->tag);

        block * tmp = curr_bucket->freelist[k];
        curr_bucket->freelist[k] = curr_bucket->freelist[k]->next;
        if (curr_bucket->freelist[k] != NULL)
            curr_bucket->freelist[k]->prev = NULL;
        tmp->prev = NULL;
        tmp->next = NULL;
        tmp->unused = 0;
        tmp++;
        rval = (void *) tmp;
    } else {
        errno = ENOMEM;
        //      perror("Couldn't allocate memory, in buddy_malloc");
    }
    //printf("b_malloc: End of bmalloc\n");
    return rval;
}

bucket * find_bucket(block * b) {
    bucket * prev_bucket = BASE;
    bucket * curr_bucket = BASE;
    while (curr_bucket != NULL && (long) curr_bucket < (long) b) {
        prev_bucket = curr_bucket;
        curr_bucket = curr_bucket->next;
    }

    return prev_bucket;
}

void buddy_free(void * ptr)
{
    if (ptr == NULL)
        return;
    //printf("b_free: Start of free\n");

    void * topOfHeap = sbrk(0);
    if (BASE == NULL || (long) ptr <= (long) BASE + sizeof(bucket) + sizeof(block) || (long) ptr >= (long) topOfHeap) {
//        exit(1);
        return;
    }
    block * curr = (block *) ptr;
    curr--;

    if (curr->unused) {
        perror("Double Free Error\n");
        //      exit(1);
        return;
    }
    //printf("b_free: Block->tag=%u, k=%u, &=%p\n", TMP->tag, TMP->kval, TMP);

    long k = curr->kval;
    bucket * bucket = find_bucket(curr);

    if (bucket == NULL) {
        return;
    }

    void * base = (void *) (bucket + 1);
    block * buddy = NULL;
    long buddy_unused = 0;


    if (k < MAX) {

        buddy = (block *) ((((size_t) curr - (size_t) base) ^ ((size_t) 1 << k)) + (size_t) base);
        //printf("b_free: Bud->tag=%u, k=%u, &=%p\n", BUD->tag, BUD->kval, BUD);
        //printf("b_free: tmp=%zx bud=%zx k=%u\n", (size_t)TMP - (size_t) BASE ,(size_t) BUD - (size_t) BASE, k);

        // MIRACLE WORKER
        if (k == buddy->kval) //?????
            buddy_unused = buddy->unused;
    }
    if (buddy_unused) {
        block *prev_block = buddy->prev;

        //printf("b_free: AV=%p T=%p B=%p P=%p\n", AVAIL[k],TMP, BUD, PREV);
        if (buddy == bucket->freelist[k]) {
            //printf("b_free: BUD was head\n");
            bucket->freelist[k] = buddy->next;
        } //?????


        if (prev_block == NULL) {
            //printf("b_free: BUD PREV was NULL\n");
            bucket->freelist[k] = buddy->next;
        } else {
            prev_block->next = buddy->next;
        }
        block *next_block = buddy->next;
        if (next_block != NULL)
            next_block->prev = buddy->prev;

        //printf("b_free: Buddy Available!\n");

        // Find the address that is smallest
        if (buddy < curr) //?????
            curr = buddy;
        //printf("b_free: Mask=%zx, k=%u\n",((size_t) 1 << (TMP->kval)), TMP->kval);
        //printf("b_free: Merged&=%p k=%u, T&=%p, B&=%p k=%u\n", MERGE, k+1, TMP, BUD, k);

        k++;
        curr->kval = k;
        curr->unused = 0;
        curr++;

        buddy_free((void *) curr);

    } else {
        //printf("b_free: no buddy\n");
        block * temp = (block *) bucket->freelist[k];
        //printf("b_free: curr=%p k=%u, TMP&=%p k=%u\n", curr, k, TMP, TMP->kval);
        if (temp != NULL) {
            while (temp->next != NULL) {
                temp = temp->next;
            }
            temp->next = curr;
        } else {
            bucket->freelist[k] = curr;
        }
        curr->kval = k;
        curr->prev = curr;
        curr->next = NULL;
        curr->unused = 1;

    }

    //printf("b_free: End of buddy free\n");
}


void * buddy_calloc(size_t num, size_t size)
{
    if (num == 0 || size == 0)
        return NULL;

    /*
     * // allocate block of memory num * size bytes
     * if (BASE == NULL)
     *   buddy_init((size_t) (size * num));
     */

    void *rval = buddy_malloc(num * size);
    // MUST INITIALIZE NEW DATA TO ZERO
    if (rval != NULL) {
        memset(rval, 0, num * size);
    }

    return rval;
}


void *buddy_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {
        return buddy_malloc(size);
    }

    if (size == 0) {
        buddy_free(ptr);
        return ptr;
    }

    /*
    // this should never be the case, but we'll take it
    if (BASE == NULL)
        buddy_init(size);
    */

    block * curr = (block *) ptr;
    curr--;

    size_t oldSize = (1 << curr->kval) - sizeof(block);

    void * rval;

    if (oldSize != size) {
        rval = buddy_malloc(size);
        memcpy(rval, ptr, size);
        buddy_free(ptr);
    } else {
        rval = ptr;
    }

    return rval;
}

void *malloc(size_t size)
{
    //printf("interposed: buddy_malloc\n");

    return buddy_malloc(size);
}

void free(void *ptr)
{
    //printf("interposed: buddy_free\n");

    buddy_free(ptr);
}

void *calloc(size_t nmemb, size_t size)
{
    //printf("interposed: buddy_calloc\n");

    return buddy_calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size)
{
    //printf("interposed: buddy_realloc\n");
    return buddy_realloc(ptr, size);

}

int main() {
    printf("Hello, World!\n");
    return 0;
}