#include <printf.h>
#include <stdarg.h>

int snprintf(char * buff, size_t len, const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	len = vsnprintf(buff, len, format, ap);
	va_end(ap);

	return len;
}

int vsnprintf(char *buff, size_t len, const char* format, va_list ap){
	size_t i, result;

	if (!buff || !format)
		return -1;

#define PUTCHAR(thechar) \
	do { \
		if (result < len-1) \
		*buff++ = (thechar);  \
		result++; \
	} while (0)

	result = 0;
	for(i = 0; format[i] != '\0' ; i++) {
		if (format[i] == '%') {
			i++;
			switch(format[i]) {
				case '%':
					PUTCHAR('%');
					break;

				case 'i':;
				case 'd': {
					 int integer = va_arg(ap,int);
					 int cpt2 = 0;
					 char buff_int[16];

					 if (integer<0)
						 PUTCHAR('-');

					 do {
						 int m10 = integer%10;
						 m10 = (m10 < 0)? -m10:m10;
						 buff_int[cpt2++] = (char)('0'+ m10);
						 integer = integer/10;
					 } while(integer != 0);

					 for(cpt2 = cpt2 - 1; cpt2 >= 0; cpt2--)
						 PUTCHAR(buff_int[cpt2]);

					 break;
				}
				case 'c': {
					 int value = va_arg(ap,int);
					 PUTCHAR((char)value);
					 break;
				}
				case 's': {
					 char *string = va_arg(ap,char *);
					 if (!string)
						 string = "(null)";
					 for(; *string != '\0' ; string++)
						 PUTCHAR(*string);
					 break;
				}
				case 'x': {
					 unsigned int hexa = va_arg(ap,int);
					 int had_nonzero = 0;
					 for(int j = 0; j < 8; j++) {
						 unsigned int nb = (unsigned int)(hexa << (j*4));
						 nb = (nb >> 28) & 0xf;
						 // Skip the leading zeros
						 if (nb == 0) {
							 if (had_nonzero)
								 PUTCHAR('0');
						 } else {
							 had_nonzero = 1;
							 if (nb < 10)
								 PUTCHAR('0'+nb);
							 else
								 PUTCHAR('a'+(nb-10));
						 }
					 }
					 if (!had_nonzero)
						 PUTCHAR('0');
					 break;
				}
				case 'p': {
					 unsigned int hexa = va_arg(ap,int);
					 for (int j = 0; j < 8; j++) {
						 unsigned int nb = (unsigned int)(hexa << (j*4));
						 nb = (nb >> 28) & 0xf;
						 if (nb < 10)
							 PUTCHAR('0'+nb);
						 else
							 PUTCHAR('a'+(nb-10));
					 }
					 break;
				}
				default:
					 PUTCHAR('%');
					 PUTCHAR(format[i]);
			}
		} else {
			PUTCHAR(format[i]);
		}
	}

	*buff = '\0';
	return result;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
