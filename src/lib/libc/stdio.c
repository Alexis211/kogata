#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <proto/launch.h>

#include <kogata/syscall.h>
#include <kogata/printf.h>

FILE *stdin = 0;
FILE *stdout = 0;
FILE *stderr = 0;

FILE libc_tty_stdio, libc_stdin, libc_stdout, libc_stderr;

void initialize_out_buffer(FILE* f, int buf_mode) {
	if (buf_mode == 0) {
		f->buf_mode = 0;
		f->out_buf = 0;
		f->out_buf_size = f->out_buf_used = 0;
		f->out_buf_owned = false;
	} else {
		f->buf_mode = buf_mode;
		f->out_buf = malloc(BUFSIZ);
		f->out_buf_size = BUFSIZ;
		f->out_buf_used = 0;
		f->out_buf_owned = true;
	}
}

void setup_libc_stdio() {
	if (sc_stat_open(STD_FD_TTY_STDIO, &libc_tty_stdio.st)) {
		libc_tty_stdio.fd = STD_FD_TTY_STDIO;
		libc_tty_stdio.file_mode = (libc_tty_stdio.st.access & (FM_READ | FM_WRITE));
		libc_tty_stdio.pos = 0;

		initialize_out_buffer(&libc_tty_stdio, _IOLBF);

		sc_fctl(libc_tty_stdio.fd, FC_SET_BLOCKING, 0);
		libc_tty_stdio.file_mode |= FM_BLOCKING;
		libc_tty_stdio.ungetc_char = EOF;

		stdin = &libc_tty_stdio;
		stdout = &libc_tty_stdio;
		stderr = &libc_tty_stdio;
	}
	if (sc_stat_open(STD_FD_STDIN, &libc_stdin.st)) {
		libc_stdin.fd = STD_FD_STDIN;
		ASSERT(libc_stdin.st.access & FM_READ);
		libc_stdin.file_mode = FM_READ;
		libc_stdin.pos = 0;

		initialize_out_buffer(&libc_stdin, 0);

		sc_fctl(libc_stdin.fd, FC_SET_BLOCKING, 0);
		libc_stdin.file_mode |= FM_BLOCKING;
		libc_stdin.ungetc_char = EOF;

		stdin = &libc_stdin;
	}
	if (sc_stat_open(STD_FD_STDOUT, &libc_stdout.st)) {
		libc_stdout.fd = STD_FD_STDOUT;
		ASSERT(libc_stdout.st.access & FM_WRITE);
		libc_stdout.file_mode = FM_WRITE;
		libc_stdout.pos = 0;

		initialize_out_buffer(&libc_stdout, _IOLBF);

		stdout = &libc_stdout;
	}
	if (sc_stat_open(STD_FD_STDERR, &libc_stderr.st)) {
		libc_stderr.fd = STD_FD_STDERR;
		ASSERT(libc_stderr.st.access & FM_WRITE);
		libc_stderr.file_mode = FM_WRITE;
		libc_stderr.pos = 0;

		initialize_out_buffer(&libc_stderr, _IONBF);

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
	ASSERT(size == 1);	//TODO all cases

	if (stream == NULL || stream->fd == 0) return 0;

	// TODO buffering
	size_t ret =  sc_read(stream->fd, stream->pos, size * nmemb, ptr);
	if (!(stream->st.type & (FT_CHARDEV | FT_CHANNEL | FT_DIR))) {
		stream->pos += ret;
	}
	return ret;
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
			if (l > 0) {
				l--;
			} else {
				//HACK
				if (stream == &libc_tty_stdio) {
					putc(' ', stream);
				}
			}
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

// buffering

void setbuf(FILE *stream, char *buf) {
	setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}
void setbuffer(FILE *stream, char *buf, size_t size) {
	setvbuf(stream, buf, buf ? _IOFBF : _IONBF, size);
}
void setlinebuf(FILE *stream) {
	setvbuf(stream, NULL, _IOLBF, 0);
}
int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
	if (stream == NULL || stream->fd == 0
		|| !(stream->file_mode & FM_WRITE)) return EOF;

	if (fflush(stream) == EOF) return EOF;

	if (stream->out_buf_owned) free(stream->out_buf);
	if (buf == NULL) {
		stream->out_buf = malloc(size);

		// if we cannot allocate a buffer, set file to read-only mode
		if (stream->out_buf == NULL) {
			stream->file_mode &= ~FM_WRITE;
			stream->buf_mode = 0;
			return EOF;
		}

		stream->out_buf_owned = true;
	} else {
		stream->out_buf = buf;
		stream->out_buf_owned = false;
	}

	stream->buf_mode = mode;

	return 0;
}

int fflush(FILE* stream) {
	if (stream == NULL || stream->fd == 0
		|| !(stream->file_mode & FM_WRITE)) return EOF;

	if (stream->buf_mode != 0 && stream->out_buf_used > 0) {
		size_t ret = sc_write(stream->fd, stream->pos, stream->out_buf_used, stream->out_buf);
		stream->out_buf_used = 0;

		if (ret != stream->out_buf_used) {
			return EOF;
		}
		if (!(stream->st.type & (FT_CHARDEV | FT_CHANNEL | FT_DIR))) {
			stream->pos += ret;
		}
	}
	return 0;
}

int buffered_putc(int c, FILE* stream) {
	ASSERT(stream->buf_mode != 0 && stream->out_buf_used < stream->out_buf_size);
	stream->out_buf[stream->out_buf_used++] = c;
	if (stream->out_buf_used == stream->out_buf_size ||
		(stream->buf_mode == _IOLBF && c == '\n')) {
		fflush(stream);
	}
	return c;
}


// real output functions

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	if (stream == NULL || stream->fd == 0
		|| !(stream->file_mode & FM_WRITE)) return 0;

	fflush(stream);

	size_t ret = sc_write(stream->fd, stream->pos, size * nmemb, ptr);
	if (!(stream->st.type & (FT_CHARDEV | FT_CHANNEL | FT_DIR))) {
		stream->pos += ret;
	}
	return ret / size;
}

int fputc(int c, FILE *stream) {
	if (stream == NULL || stream->fd == 0
		|| !(stream->file_mode & FM_WRITE)) return EOF;

	buffered_putc(c, stream);
	if (stream->buf_mode == _IONBF) {
		if (fflush(stream) == EOF) return EOF;
	}
	return c;
}
int fputs(const char *s, FILE *stream) {
	if (stream == NULL || stream->fd == 0
		|| !(stream->file_mode & FM_WRITE)) return EOF;

	int i = 0;
	while (s[i]) {
		buffered_putc(s[i], stream);
		i++;
	}
	if (stream->buf_mode == _IONBF) {
		if (fflush(stream) == EOF) return EOF;
	}
	return i;
}
int putc(int c, FILE *stream) {
	// just an alias
	return fputc(c, stream);
}

int fprintf(FILE *stream, const char *fmt, ...) {
	if (stream == NULL || stream->fd == 0
		|| !(stream->file_mode & FM_WRITE)) return -1;

	va_list ap;

	va_start(ap, fmt);
	int ret = vfprintf(stream, fmt, ap);
	va_end(ap);

	return ret;
}

int vfprintf_putc_fun(int c, void* p) {
	FILE *stream = (FILE*)p;
	if (buffered_putc(c, stream) == EOF)
		return -1;
	else
		return 1;
}
int vfprintf(FILE *stream, const char *format, va_list ap) {
	if (stream == NULL || stream->fd == 0
		|| !(stream->file_mode & FM_WRITE)) return -1;

	int ret = vcprintf(vfprintf_putc_fun, stream, format, ap);

	if (stream->buf_mode == _IONBF) {
		if (fflush(stream) == EOF) return -1;
	}

	return ret;
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
