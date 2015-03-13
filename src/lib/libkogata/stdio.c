#include <syscall.h>

#include <printf.h>
#include <stdio.h>


fd_t stdio = 1;

int getc() {
	sel_fd_t fd;
	fd.fd = stdio;
	fd.req_flags = SEL_READ;
	ASSERT(select(&fd, 1, -1));
	ASSERT(fd.got_flags & SEL_READ);

	char chr;
	size_t sz = read(stdio, 0, 1, &chr);
	ASSERT(sz == 1);
	return chr;
}


void putc(int c) {
	char chr = c;
	write(stdio, 0, 1, &chr);
}

void puts(char* s) {
	while (*s) putc(*(s++));
}

void getline(char* buf, size_t l) {
	size_t i = 0;
	while (true) {
		int c = getc();
		if (c == '\n') {
			putc('\n');
			buf[i] = 0;
			break;
		} else if (c == '\b') {
			if (i > 0) {
				 i--;
				putc('\b');
			}
		} else if (c >= ' ') {
			buf[i] = c;
			if (i < l-1) {
				i++;
				putc(c);
			}
		}
	}
}

void printf(char* fmt, ...) {
	va_list ap;
	char buffer[256];

	va_start(ap, fmt);
	vsnprintf(buffer, 256, fmt, ap);
	va_end(ap);

	puts(buffer);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
