//
// Created on 1/28/16.
//

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include "worker.h"

#define READ_END 0
#define WRITE_END 1

int main(int argc, char **argv) {
    if (argc != 5 && argc != 6) {
        fprintf(stderr, "Please provide 5 or 6 exact arguments.\n");
        exit(EXIT_FAILURE);
    }
    // Coming from the master
    if (argc == 6){
        double res;
        if (atoi(argv[4]) == 0){
            res = 1;
        }
        else {
//            res = compute_worker(atoi(argv[2]), atoi(argv[4]));
            res = (double)compute_worker2(atoi(argv[2]), atoi(argv[4]));
        }
        write(WRITE_END, &res, sizeof(double));
        close(WRITE_END);
    }
    // Coming from Terminal
    if (argc == 5){
        long double res;
        if (atoi(argv[4]) == 0){
            res = 1;
        }
        else {
//            res = compute_worker(atoi(argv[2]), atoi(argv[4]));
            res = compute_worker2(atoi(argv[2]), atoi(argv[4]));
        }
        fprintf(stderr, "x^n / n!: %Lg\n", res);
    }
    return 0;
}

double compute_worker(int value, int n) {
    double numerator = pow(value, n) * 1.0;
    fprintf(stderr, "numberator: %f\n", numerator);
    signed long long int denominator = compute_factorial(n);
    fprintf(stderr, "denominator: %lli\n", denominator);
    if (denominator != 0) {
        return numerator/denominator;
    }
    else {
        return 0;
    }
}

signed long long int compute_factorial(int n) {
    fprintf(stderr, "In compute factorial, n: %d\n", n);
    signed long long int c, fact = 1;
    for (c = 1; c <= n; c++) {
        fact = fact * c;
        fprintf(stderr, "c=%lli, ans=%lli\n", c, fact);
    }
    return fact;
}

long double compute_worker2(int x, int n) {
    long double res = 1;
    for (long double i = 1; i <= n; ++i) {
        res *=  x/i;
//        fprintf(stderr, "c=%Lg, ans=%Lg\n", i, res);
    }
    return res;
}