#include <string.h>
#include <stdio.h>

#include <proto/launch.h>

#include <kogata/syscall.h>
#include <kogata/printf.h>

FILE *stdin = 0;
FILE *stdout = 0;
FILE *stderr = 0;

FILE libc_tty_stdio, libc_stdin, libc_stdout, libc_stderr;

void setup_libc_stdio() {
	if (sc_stat_open(STD_FD_TTY_STDIO, &libc_tty_stdio.st)) {
		libc_tty_stdio.fd = STD_FD_TTY_STDIO;
		sc_fctl(libc_tty_stdio.fd, FC_SET_BLOCKING, 0);
		// TODO: initialize libc_tty_stdio as a TTY
		stdin = &libc_tty_stdio;
		stdout = &libc_tty_stdio;
		stderr = &libc_tty_stdio;
	}
	if (sc_stat_open(STD_FD_STDIN, &libc_stdin.st)) {
		libc_stdin.fd = STD_FD_STDIN;
		// TODO: initialize
		stdin = &libc_stdin;
	}
	if (sc_stat_open(STD_FD_STDOUT, &libc_stdout.st)) {
		libc_stdout.fd = STD_FD_STDOUT;
		// TODO: initialize
		stdout = &libc_stdout;
	}
	if (sc_stat_open(STD_FD_STDERR, &libc_stderr.st)) {
		libc_stderr.fd = STD_FD_STDERR;
		// TODO: initialize
		stderr = &libc_stderr;
	}
}


int getchar() {
	return fgetc(stdin);
}

int putchar(int c) {
	return fputc(c, stdout);
}

int puts(const char* s) {
	return fputs(s, stdout);
}

int printf(const char* fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	int ret = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return ret;
}

// ==================
// BELOW IS TODO
// ==================


FILE *fopen(const char *path, const char *mode) {
	// TODO
	return 0;
}
FILE *freopen(const char *path, const char *mode, FILE *stream) {
	// TODO
	return 0;
}
int fclose(FILE* f) {
	// TODO
	return 0;
}


// ---------------
// INPUT FUNCTIONS
// ---------------

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	if (stream == NULL || stream->fd == 0) return 0;

	// TODO buffering
	// TODO position
	return sc_read(stream->fd, 0, size * nmemb, ptr);
}

int fgetc(FILE *stream) {
	// TODO buffering && ungetc
	char x;
	if (fread(&x, 1, 1, stream)) {
		return x;
	} else {
		return EOF;
	}
}
char *fgets(char *s, int size, FILE *stream) {
	int l = 0;
	while (l < size - 1) {
		int c = fgetc(stream);
		if (c == EOF) {
			break;
		} else if (c == '\b') {
			if (l > 0) l--;
			// TODO if terminal write back space or something
		} else {
			s[l] = c;
			l++;
			if (c == '\n') break;
		}
	}
	s[l] = 0;
	return s;
}
int getc(FILE *stream) {
	// TODO
	return 0;
}
int ungetc(int c, FILE *stream) {
	// TODO
	return 0;
}

// ----------------
// OUTPUT FUNCTIONS
// ----------------

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	if (stream == NULL || stream->fd == 0) return 0;

	// TODO buffering
	// TODO position
	return sc_write(stream->fd, 0, size * nmemb, ptr);
}

int fputc(int c, FILE *stream) {
	unsigned char x = c;
	return fwrite(&x, 1, 1, stream);
}
int fputs(const char *s, FILE *stream) {
	return fwrite(s, strlen(s), 1, stream);
}
int putc(int c, FILE *stream) {
	return fputc(c, stream);
}

int fprintf(FILE *stream, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	int ret = vfprintf(stream, fmt, ap);
	va_end(ap);

	return ret;
}
int vfprintf(FILE *stream, const char *format, va_list ap) {
	char buf[1024];
	vsnprintf(buf, 1024, format, ap);

	return fputs(buf, stream);
}

// buffering

void setbuf(FILE *stream, char *buf) {
	setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}
void setbuffer(FILE *stream, char *buf, size_t size) {
	// TODO
}
void setlinebuf(FILE *stream) {
	setvbuf(stream, NULL, _IOLBF, 0);
}
int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
	// TODO
	return 0;
}

int fflush(FILE* f) {
	// TODO
	return 0;
}

// ---------------------
// COMPLICATED FUNCTIONS
// ---------------------

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


// ------------------
// POSITION FUNCTIONS
// ------------------


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

// ---------------------
// PATH & FILE FUNCTIONS
// ---------------------

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



/* vim: set ts=4 sw=4 tw=0 noet :*/
