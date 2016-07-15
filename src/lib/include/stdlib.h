#pragma once

#include <kogata/malloc.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 255

int rand(void);
//int rand_r(unsigned int *seedp);
void srand(unsigned int seed);

#define RAND_MAX 1

void abort() __attribute__((__noreturn__));


double strtod(const char *nptr, char **endptr);
float strtof(const char *nptr, char **endptr);
long double strtold(const char *nptr, char **endptr);

char *getenv(const char *name);

int system(const char *command);

int abs(int j);



/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
