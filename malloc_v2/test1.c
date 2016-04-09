#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    size_t size = 666;
    void *mem = malloc(size);
    printf("Successfully malloc'd %zu bytes at addr %p\n", size, mem);
    assert(mem != NULL);
//    free(mem);
    printf("Successfully free'd %zu bytes from addr %p\n", size, mem);

//    void * mem2[34];
//    size = 488;
//    for (int i = 0; i < 34; ++i) {
//        printf("test1.c malloc i=%d\n", i);
//        mem2[i] = malloc(size);
//    }
//
//    for (int i = 0; i < 34; ++i) {
//        printf("test1.c free i=%d\n", i);
//        free(mem2[i]);
//    }

    return 0;
}
