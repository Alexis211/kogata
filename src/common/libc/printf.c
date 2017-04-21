#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>

#include <kogata/debug.h>
#include <kogata/printf.h>

int snprintf(char * buff, size_t len, const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	len = vsnprintf(buff, len, format, ap);
	va_end(ap);

	return len;
}


struct vsnprintf_s {
	char* buff;
	size_t len;
	size_t i;
};
int vsnprintf_putc_fun(int c, void* p) {
	struct vsnprintf_s *s = (struct vsnprintf_s*)p;
	if (s->i < s->len - 1) {
		s->buff[s->i] = c;
		s->i++;
		return 1;
	} else {
		return 0;
	}
}
int vsnprintf(char *buff, size_t len, const char* format, va_list arg){
	if (!buff || !format)
		return -1;

	struct vsnprintf_s s;
	s.buff = buff;
	s.len = len;
	s.i = 0;

	size_t ret = vcprintf(vsnprintf_putc_fun, &s, format, arg);
	ASSERT(ret == s.i);
	
	buff[s.i] = 0;
	return s.i;
}

int vcprintf(int (*putc_fun)(int c, void* p), void* p, const char* format, va_list ap) {
	if (!format) return -1;

	int ret = 0;
#define PUTCHAR(thechar) { int _tmp = putc_fun(thechar, p);\
						   if (_tmp == 1) ret++; \
						   else if (_tmp == 0) return ret; \
						   else if (_tmp < 0) return _tmp; }

	size_t i;
	for(i = 0; format[i] != '\0' ; i++) {
		if (format[i] == '%') {
			i++;
			int na = -1;
			int nb = -1;
			int u = 0;
			int l = 0;
			bool dotspec = false;
			bool spec = true;
			while (spec) {
				if (format[i] == 'l') {
					l++;
					i++;
				} else if (format[i] == 'u') {
					u = 1;
					i++;
				} else if (isdigit(format[i])) {
					if (dotspec) {
						if (nb == -1) nb = 0;
						nb = nb * 10 + (format[i] - '0');
					} else {
						if (na == -1) na = 0;
						na = na * 10 + (format[i] - '0');
					}
					i++;
				} else if (format[i] == '.') {
					dotspec = true;
					i++;
				} else {
					spec = false;
				}
			}
			if (format[i] == '%') {
					PUTCHAR('%');
			} else if (format[i] == 'i' || format[i] == 'd') {
				if (u) {
					// TODO
				} else {
					long long int integer;
					if (l == 0) integer = va_arg(ap, int);
					if (l == 1) integer = va_arg(ap, long int);
					if (l == 2) integer = va_arg(ap, long long int);

					int cpt2 = 0;
					char buff_int[32];

					if (integer<0) {
						PUTCHAR('-');
					}

					do {
						int m10 = integer%10;
						m10 = (m10 < 0)? -m10:m10;
						buff_int[cpt2++] = (char)('0'+ m10);
						integer = integer/10;
					} while(integer != 0);

					for(cpt2 = cpt2 - 1; cpt2 >= 0; cpt2--)
						PUTCHAR(buff_int[cpt2]);
				}
			} else if (format[i] == 'c') {
					 int value = va_arg(ap,int);
					 PUTCHAR((char)value);
					 break;
			} else if (format[i] == 's') {
				 char *string = va_arg(ap,char *);
				 if (!string)
					 string = "(null)";
				 for(; *string != '\0' ; string++)
					 PUTCHAR(*string);
			} else if (format[i] == 'x') {
				unsigned long long int hexa;
				if (l == 0) hexa = va_arg(ap, unsigned int);
				if (l == 1) hexa = va_arg(ap, unsigned long int);
				if (l == 2) hexa = va_arg(ap, unsigned long long int);

				int had_nonzero = 0;
				for(int j = 0; j < 8; j++) {
					unsigned int nb = (unsigned int)(hexa << (j*4));
					nb = (nb >> 28) & 0xf;
					// Skip the leading zeros
					if (nb == 0) {
						if (had_nonzero) {
							PUTCHAR('0');
						}
					} else {
						had_nonzero = 1;
						if (nb < 10) {
							PUTCHAR('0'+nb);
						} else {
							PUTCHAR('a'+(nb-10));
						}
					}
				}
				if (!had_nonzero) {
					PUTCHAR('0');
				}
			} else if (format[i] == 'p') {
				unsigned int hexa = va_arg(ap,int);
				for (int j = 0; j < 8; j++) {
					unsigned int nb = (unsigned int)(hexa << (j*4));
					nb = (nb >> 28) & 0xf;
					if (nb < 10) {
						PUTCHAR('0'+nb);
					} else {
						PUTCHAR('a'+(nb-10));
					}
				}
			} else {
				PUTCHAR('%');
				PUTCHAR(format[i]);
			}
		} else {
			PUTCHAR(format[i]);
		}
	}

	return ret;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
