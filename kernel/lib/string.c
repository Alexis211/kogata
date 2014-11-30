#include <string.h>


size_t strlen(const char* str) {
	size_t ret = 0;
	while (str[ret] != 0)
		ret++;
	return ret;
}

char *strchr(const char *str, char c) {
	while (*str) {
		if (*str == c) return (char*)str;
		str++;
	}
	return NULL;
}

char *strcpy(char *dest, const char *src) {
	memcpy(dest, src, strlen(src) + 1);
	return (char*)src;
}

char *strcat(char *dest, const char *src) {
	char *dest2 = dest;
	dest2 += strlen(dest) - 1;
	while (*src) {
		*dest2 = *src;
		src++;
		dest2++;
	}
	*dest2 = 0;
	return dest;
}

int strcmp(const char *s1, const char *s2) {
	while ((*s1) && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return (* (unsigned char*)s1 - *(unsigned char*)s2);
}

void *memcpy(void *vd, const void *vs, int count) {
	uint8_t *dest = (uint8_t*)vd, *src = (uint8_t*)vs;
	int f = count % 4, n = count / 4, i;
	const uint32_t* s = (uint32_t*)src;
	uint32_t* d = (uint32_t*)dest;
	for (i = 0; i < n; i++) {
		d[i] = s[i];
	}
	if (f != 0) {
		for (i = count - f; i < count; i++) {
			dest[i] = src[i];
		}
	}
	return vd;
}

void *memset(void *dest, int val, int count) {
	uint8_t *dest_c = (uint8_t*)dest;
	int i;
	for (i = 0; i < count; i++) {
		dest_c[i] = val;
	}
	return dest;
}

