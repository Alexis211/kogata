#pragma once

#include <stdarg.h>

#include <kogata/printf.h>

#include <kogata/syscall.h>

void setup_libc_stdio();


//TODO below
struct file_t {
	// TODO
};
typedef struct file_t FILE;


int fgetc(FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int getc(FILE *stream);
int getchar(void);
int ungetc(int c, FILE *stream);

int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
int putc(int c, FILE *stream);
int putchar(int c);
int puts(const char *s);

FILE *fopen(const char *path, const char *mode);
FILE *freopen(const char *path, const char *mode, FILE *stream);

void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
int fileno(FILE *stream);


size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

int fflush(FILE* f);
int fclose(FILE* f);

#define EOF ((int)-1)

extern FILE *stdin, *stdout, *stderr;

#define BUFSIZ 0
void setbuf(FILE *stream, char *buf);
void setbuffer(FILE *stream, char *buf, size_t size);
void setlinebuf(FILE *stream);
int setvbuf(FILE *stream, char *buf, int mode, size_t size);

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

typedef size_t fpos_t;	//TODO

int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);
int fgetpos(FILE *stream, fpos_t *pos);
int fsetpos(FILE *stream, const fpos_t *pos);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define L_tmpnam 128
FILE *tmpfile(void);
char *tmpnam(char *s);

int rename(const char *old, const char *new);
int remove(const char *pathname);


int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int dprintf(int fd, const char *format, ...);
int sprintf(char *str, const char *format, ...);


/* vim: set ts=4 sw=4 tw=0 noet :*/
