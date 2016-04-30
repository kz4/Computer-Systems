#ifndef MULTI_THREAD_COMPUTATION_MAIN_H
#define MULTI_THREAD_COMPUTATION_MAIN_H

#include <math.h>

typedef struct Args {
    int x, i;
    pthread_mutex_t mutex;
} TermStruct;

int convertCStringToInt(char *string);
signed long long int compute_factorial(int n);
long double computeTerm(int x, int n);
void computeOneTerm(TermStruct * termStruct);

#endif //MULTI_THREAD_COMPUTATION_MAIN_H
