#pragma once

#include <stddef.h>
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t count);
void *memset(void *dest, int val, size_t count);
int memcmp(const void *s1, const void *s2, size_t n);
void *memmove(void *dest, const void *src, size_t count);

size_t strlen(const char *str);
char *strchr(const char *str, char c);
char *strrchr(const char *str, char c);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

char *strdup(const char* str);
char *strndup(const char* str, size_t count);

/* vim: set ts=4 sw=4 tw=0 noet :*/
