#include <string.h>
#include <stdio.h>

#include <kogata/syscall.h>
#include <kogata/printf.h>


fd_t stdio = 1;

int getchar() {
	char chr;
	size_t sz = read(stdio, 0, 1, &chr);
	ASSERT(sz == 1);
	return chr;
}


int putchar(int c) {
	char chr = c;
	write(stdio, 0, 1, &chr);
	return 0;	//TODO what?
}

int puts(const char* s) {
	// TODO return EOF on error
	return write(stdio, 0, strlen(s), s);
}

void getline(char* buf, size_t l) {
	size_t i = 0;
	while (true) {
		int c = getchar();
		if (c == '\n') {
			putchar('\n');
			buf[i] = 0;
			break;
		} else if (c == '\b') {
			if (i > 0) {
				 i--;
				putchar('\b');
			}
		} else if (c >= ' ') {
			buf[i] = c;
			if (i < l-1) {
				i++;
				putchar(c);
			}
		}
	}
}

int printf(const char* fmt, ...) {
	va_list ap;
	char buffer[256];

	va_start(ap, fmt);
	vsnprintf(buffer, 256, fmt, ap);
	va_end(ap);

	return puts(buffer);
}

// ==================
// BELOW IS TODO
// ==================


int fgetc(FILE *stream) {
	// TODO
	return 0;
}
char *fgets(char *s, int size, FILE *stream) {
	// TODO
	return 0;
}
int getc(FILE *stream) {
	// TODO
	return 0;
}
int ungetc(int c, FILE *stream) {
	// TODO
	return 0;
}

int fputc(int c, FILE *stream) {
	// TODO
	return 0;
}
int fputs(const char *s, FILE *stream) {
	// TODO
	return 0;
}
int putc(int c, FILE *stream) {
	// TODO
	return 0;
}

FILE *fopen(const char *path, const char *mode) {
	// TODO
	return 0;
}
FILE *freopen(const char *path, const char *mode, FILE *stream) {
	// TODO
	return 0;
}

void clearerr(FILE *stream) {
	// TODO
}
int feof(FILE *stream) {
	// TODO
	return 0;
}
int ferror(FILE *stream) {
	// TODO
	return 0;
}
int fileno(FILE *stream) {
	// TODO
	return 0;
}


size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	// TODO
	return 0;
}
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	// TODO
	return 0;
}

int fflush(FILE* f) {
	// TODO
	return 0;
}
int fclose(FILE* f) {
	// TODO
	return 0;
}

FILE *stdin = 0;
FILE *stdout = 0;
FILE *stderr = 0;

void setbuf(FILE *stream, char *buf) {
	// TODO
}
void setbuffer(FILE *stream, char *buf, size_t size) {
	// TODO
}
void setlinebuf(FILE *stream) {
	// TODO
}
int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
	// TODO
	return 0;
}


int fseek(FILE *stream, long offset, int whence) {
	// TODO
	return 0;
}
long ftell(FILE *stream) {
	// TODO
	return 0;
}
void rewind(FILE *stream) {
	// TODO
}
int fgetpos(FILE *stream, fpos_t *pos) {
	// TODO
	return 0;
}
int fsetpos(FILE *stream, const fpos_t *pos) {
	// TODO
	return 0;
}

FILE *tmpfile(void) {
	// TODO
	return 0;
}
char *tmpnam(char *s) {
	// TODO
	return 0;
}

int rename(const char *old, const char *new) {
	// TODO
	return 0;
}
int remove(const char *pathname) {
	// TODO
	return 0;
}



int fprintf(FILE *stream, const char *format, ...) {
	// TODO
	return 0;
}
int dprintf(int fd, const char *format, ...) {
	// TODO
	return 0;
}
int sprintf(char *str, const char *format, ...) {
	// TODO
	return 0;
}


/* vim: set ts=4 sw=4 tw=0 noet :*/
