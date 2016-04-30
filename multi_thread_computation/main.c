#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "threadpool.h"
#include "main.h"


double sum = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

long double computeTerm(int x, int n) {
    long double res = 1;
    for (long double i = 1; i <= n; ++i) {
        res *=  x/i;
    }
    return res;
}

void computeOneTerm(TermStruct * termStruct) {
//    pthread_mutex_t *lock = termStruct->mutex;

    pthread_mutex_lock(&termStruct->mutex);

    int i = termStruct->i;
    int x = termStruct->x;

    double termRes = computeTerm(x, i);
    sum += termRes;
    fprintf(stderr, "sum: %f from %u \n", sum, (int)pthread_self());

    pthread_mutex_unlock(&termStruct->mutex);
}

int main(int argc, char **argv) {

    if (argc != 7) {
        fprintf(stderr, "There should be 7 arguments, you have %d. The correct way, for example: ./main threads 2 x 2 n 12\n", argc);
    } else {
        int numOfThreads = convertCStringToInt(argv[2]);
        int x = convertCStringToInt(argv[4]);
        int n = convertCStringToInt(argv[6]);
        fprintf(stderr, "number of threads: %d, x: %d, n: %d\n", numOfThreads, x, n);

        if (n == 0) {
            fprintf(stderr, "sum: 1\n");
            return 0;
        }

        threadpool thpool = thpool_init(numOfThreads);
        for (int i = 0; i < n; ++i) {
            TermStruct * term = malloc(sizeof(TermStruct));
            term->i = i;
            term->x = x;
            term->mutex = mutex;
            thpool_add_work(thpool, (void*)computeOneTerm, (TermStruct *)term);
        }

        thpool_wait(thpool);
        thpool_destroy(thpool);

        fprintf(stderr, "final sum: %f\n", sum);
    }

    return 0;
}

int convertCStringToInt(char *string) {
    return atoi(string);
}
