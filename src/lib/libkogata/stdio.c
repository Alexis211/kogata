#include <syscall.h>
#include <string.h>

#include <printf.h>
#include <stdio.h>


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

/* vim: set ts=4 sw=4 tw=0 noet :*/
