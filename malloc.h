//
// Created by kaichen on 3/9/16.
//

#ifndef HW3_2_MALLOC_H
#define HW3_2_MALLOC_H

/*
 * This function initilizes the buddy malloc system
 *
 * @param the size to init, this is ignored and 32 Gb is initialized
 *
 */
void buddy_init();

/*
 * This function is effectively a buddy_malloc( num * size )
 *
 * @param num number of elements to allocated
 * @param size size of each element to allocate
 */
void *buddy_calloc(size_t, size_t);



/*
 * This function returns a pointer to memory to be used. The size of the
 * memory allocated is not likely to be exactle the same memory requested.
 *
 * @param size The size of the memory to allocate
 *
 */
void *buddy_malloc(size_t);


/*
 *
 * This function takes an existing pointer and gives back a new
 * pointer with a new size, the memory is the same, size allowing.
 *
 * @param ptr pointer to realloc
 * @param size size of new memory
 * @return void* pointer to new memory
 */
void *buddy_realloc(void *ptr, size_t size);


/*
 * This function frees the memory and adds it back to AVAIL list
 *
 * @parama ptr the ptr to memory to free
 *
 */
void buddy_free(void *);


void split(unsigned k);
unsigned get_k(size_t size);

#endif //HW3_2_MALLOC_H
