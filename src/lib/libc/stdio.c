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
	f->buf_mode = 0;
	f->out_buf = NULL;
	f->out_buf_size = f->out_buf_used = 0;
	f->out_buf_owned = false;

	if (buf_mode != 0) {
		f->out_buf = malloc(BUFSIZ);
		if (f->out_buf == NULL) {
			f->file_mode &= ~FM_WRITE;
			return;
		}
		f->buf_mode = buf_mode;
		f->out_buf_size = BUFSIZ;
		f->out_buf_used = 0;
		f->out_buf_owned = true;
	}
}

void initialize_in_buffer(FILE* f) {
	f->in_buf = NULL;
	f->in_buf_size = 0;
	f->in_buf_begin = f->in_buf_end = 0;

	if (f->file_mode & FM_READ) {
		f->in_buf = malloc(BUFSIZ);
		if (f->in_buf == NULL) {
			f->file_mode &= ~FM_READ;
			return;
		}
		f->in_buf_size = BUFSIZ;
	}
}

void setup_libc_stdio() {
	if (sc_stat_open(STD_FD_TTY_STDIO, &libc_tty_stdio.st)) {
		libc_tty_stdio.fd = STD_FD_TTY_STDIO;
		libc_tty_stdio.file_mode = (libc_tty_stdio.st.access & (FM_READ | FM_WRITE));
		libc_tty_stdio.pos = 0;
		libc_tty_stdio.flags = 0;

		initialize_out_buffer(&libc_tty_stdio, _IOLBF);
		initialize_in_buffer(&libc_tty_stdio);

		// For interactive input, enable blocking mode
		sc_fctl(libc_tty_stdio.fd, FC_SET_BLOCKING, 0);
		libc_tty_stdio.file_mode |= FM_BLOCKING;

		stdin = &libc_tty_stdio;
		stdout = &libc_tty_stdio;
		stderr = &libc_tty_stdio;
	}
	if (sc_stat_open(STD_FD_STDIN, &libc_stdin.st)) {
		libc_stdin.fd = STD_FD_STDIN;
		ASSERT(libc_stdin.st.access & FM_READ);
		libc_stdin.file_mode = FM_READ;
		libc_stdin.pos = 0;
		libc_stdin.flags = 0;

		initialize_out_buffer(&libc_stdin, 0);
		initialize_in_buffer(&libc_stdin);

		int mode = sc_fctl(libc_stdin.fd, FC_GET_MODE, 0);
		libc_stdin.file_mode |= (mode & FM_BLOCKING);

		stdin = &libc_stdin;
	}
	if (sc_stat_open(STD_FD_STDOUT, &libc_stdout.st)) {
		libc_stdout.fd = STD_FD_STDOUT;
		ASSERT(libc_stdout.st.access & FM_WRITE);
		libc_stdout.file_mode = FM_WRITE;
		libc_stdout.pos = 0;
		libc_stdout.flags = 0;

		initialize_out_buffer(&libc_stdout, _IOLBF);
		initialize_in_buffer(&libc_stdout);

		stdout = &libc_stdout;
	}
	if (sc_stat_open(STD_FD_STDERR, &libc_stderr.st)) {
		libc_stderr.fd = STD_FD_STDERR;
		ASSERT(libc_stderr.st.access & FM_WRITE);
		libc_stderr.file_mode = FM_WRITE;
		libc_stderr.pos = 0;
		libc_stderr.flags = 0;

		initialize_out_buffer(&libc_stderr, _IONBF);
		initialize_in_buffer(&libc_stderr);

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
	if (path == NULL) return NULL;
	if (mode == NULL || strlen(mode) == 0) {
		mode = "r";
	}

	int flags = 0;
	if (mode[0] == 'r') flags |= FM_READ;
	if (mode[0] == 'w') flags |= FM_WRITE | FM_CREATE | FM_TRUNC;
	if (mode[0] == 'a') flags |= FM_WRITE | FM_CREATE | FM_APPEND;
	if (mode[1] == '+' || (mode[1] == 'b' && mode[2] == '+'))
		flags |= FM_READ | FM_WRITE;

	FILE* f = (FILE*)malloc(sizeof(FILE));
	if (f == NULL) return NULL;

	f->fd = sc_open(path, flags);
	dbg_printf("FOPEN %s %s: fd=%d, file=%p\n", path, mode, f->fd, f);
	if (f->fd == 0) goto error;

	if (!sc_stat_open(f->fd, &f->st)) goto error;
	dbg_printf("FOPEN %s %s: stat ok\n", path, mode);
	f->file_mode = flags;

	f->flags = 0;
	if ((flags & FM_APPEND) && f->st.type == FT_REGULAR) {
		f->pos = f->st.size;
	} else {
		f->pos = 0;
	}

	initialize_out_buffer(f, (flags & FM_WRITE ? _IOLBF : 0));
	initialize_in_buffer(f);

	return f;

error:
	if (f->fd != 0) sc_close(f->fd);
	free(f);
	return NULL;
}
FILE *freopen(const char *path, const char *mode, FILE *stream) {
	dbg_printf("FREOPEN %s %s %p (UNIMPLEMENTED)\n", path, mode, stream);
	// TODO
	return NULL;
}
int fclose(FILE* f) {
	dbg_printf("FCLOSE %p\n", f);

	if (fflush(f) != 0) return EOF;

	sc_close(f->fd);
	if (f->out_buf != 0 && f->out_buf_owned)
		free(f->out_buf);
	if (f->in_buf != 0)
		free(f->in_buf);
	free(f);

	return 0;
}


// ---------------
// INPUT FUNCTIONS
// ---------------

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	dbg_printf("FREAD %d * %d, file=%p\n", size, nmemb, stream);

	ASSERT(size == 1);	//TODO all cases

	if (fflush(stream) == EOF) return 0;

	if (stream == NULL || stream->fd == 0) return 0;

	// TODO buffering
	size_t ret =  sc_read(stream->fd, stream->pos, size * nmemb, ptr);
	if (!(stream->st.type & (FT_CHARDEV | FT_CHANNEL | FT_DIR))) {
		stream->pos += ret;
	}

	if (ret < size * nmemb) {
		stream->flags |= STDIO_FL_EOF;
	}

	dbg_printf("fread = %d\n", ret);

	return ret;
}

int fgetc(FILE *stream) {
	dbg_printf("FGETC %p\n", stream);

	// TODO buffering && ungetc
	char x;
	if (fread(&x, 1, 1, stream)) {
		dbg_printf("fgetc = %d\n", x);
		return x;
	} else {
		dbg_printf("fgetc = EOF\n");
		return EOF;
	}
}
char *fgets(char *s, int size, FILE *stream) {
	dbg_printf("FGETS %p %d\n", stream, size);

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
	return fgetc(stream);
}
int ungetc(int c, FILE *stream) {
	dbg_printf("UNGETC %p, %d (UNIMPLEMENTED)\n", stream, c);
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
	dbg_printf("SETVBUF %p, %p, %d, %d\n", stream, buf, mode, size);

	if (stream == NULL || stream->fd == 0
		|| !(stream->file_mode & FM_WRITE)
		|| size == 0) return EOF;

	if (fflush(stream) != 0) return EOF;

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
	dbg_printf("FFLUSH %p\n", stream);

	if (!(stream->file_mode & FM_WRITE)) return 0;
	if (stream == NULL || stream->fd == 0) {
		return EOF;
	}

	if (stream->buf_mode != 0 && stream->out_buf_used > 0) {
		size_t ret = sc_write(stream->fd, stream->pos, stream->out_buf_used, stream->out_buf);

		if (ret != stream->out_buf_used) {
			return EOF;
		}

		stream->out_buf_used = 0;
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
		if (fflush(stream) != 0) return EOF;
	}
	return c;
}


// real output functions

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	dbg_printf("FWRITE %d * %d, %p\n", size, nmemb, stream);

	if (stream == NULL || stream->fd == 0
		|| !(stream->file_mode & FM_WRITE)) return 0;

	if (fflush(stream) != 0) return EOF;

	size_t ret = sc_write(stream->fd, stream->pos, size * nmemb, ptr);
	if (!(stream->st.type & (FT_CHARDEV | FT_CHANNEL | FT_DIR))) {
		stream->pos += ret;
	}
	return ret / size;
}

int fputc(int c, FILE *stream) {
	dbg_printf("FPUTC %p %d\n", stream, c);

	if (stream == NULL || stream->fd == 0
		|| !(stream->file_mode & FM_WRITE)) return EOF;

	buffered_putc(c, stream);
	if (stream->buf_mode == _IONBF) {
		if (fflush(stream) != 0) return EOF;
	}
	return c;
}
int fputs(const char *s, FILE *stream) {
	dbg_printf("FPUTS %p\n", stream);

	if (stream == NULL || stream->fd == 0
		|| !(stream->file_mode & FM_WRITE)) return EOF;

	int i = 0;
	while (s[i]) {
		buffered_putc(s[i], stream);
		i++;
	}
	if (stream->buf_mode == _IONBF) {
		if (fflush(stream) != 0) return EOF;
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
		if (fflush(stream) != 0) return -1;
	}

	return ret;
}


// ----------------------
// FLAG-RELATED FUNCTIONS
// ----------------------

void clearerr(FILE *stream) {
	stream->flags = 0;
}
int feof(FILE *stream) {
	dbg_printf("FEOF %p = %d\n", stream, (stream->flags & STDIO_FL_EOF));

	return (stream->flags & STDIO_FL_EOF);
}
int ferror(FILE *stream) {
	dbg_printf("FERROR %p = %d\n", stream, (stream->flags & STDIO_FL_ERR));

	return (stream->flags & STDIO_FL_ERR);
}
int fileno(FILE *stream) {
	return stream->fd;
}


// ------------------
// POSITION FUNCTIONS
// ------------------


int fseek(FILE *stream, long offset, int whence) {
	dbg_printf("FSEEK %p, %d, %d\n", stream, offset, whence);

	fpos_t pos;
	if (whence == SEEK_SET) {
		pos = offset;
	} else if (whence == SEEK_CUR) {
		pos = stream->pos + offset;
	} else if (whence == SEEK_END) {
		pos = stream->st.size + offset;
	} else {
		stream->flags |= STDIO_FL_ERR;
		return -1;
	}
	return fsetpos(stream, &pos);
}
long ftell(FILE *stream) {
	return stream->pos;
}
void rewind(FILE *stream) {
	dbg_printf("REWIND %p\n", stream);

	fflush(stream);
	stream->pos = 0;
}
int fgetpos(FILE *stream, fpos_t *pos) {
	*pos = ftell(stream);
	return 0;
}
int fsetpos(FILE *stream, const fpos_t *pos) {
	dbg_printf("FSETPOS %p, %d\n", stream, pos);

	fflush(stream);
	if (*pos <= stream->st.size) {
		stream->pos = *pos;
		return 0;
	} else {
		stream->flags |= STDIO_FL_EOF;
		return -1;
	}
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
