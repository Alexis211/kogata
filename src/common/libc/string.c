#include <stdbool.h>
#include <string.h>

#include <kogata/malloc.h>

size_t strlen(const char* str) {
	size_t ret = 0;
	while (str[ret] != 0)
		ret++;
	return ret;
}

char *strchr(const char *str, char c) {
	do {
		if (*str == c) return (char*)str;
	} while (*(str++));
	return NULL;
}

char *strrchr(const char *str, char c) {
	char* ret = NULL;
	do {
		if (*str == c) ret = (char*)str;
	} while (*(str++));
	return ret;
}

char *strcpy(char *dest, const char *src) {
	memcpy(dest, src, strlen(src) + 1);
	return (char*)dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
	size_t x = strlen(src);
	if (n < x) x = n;
	memcpy(dest, src, x);
	if (n > x) memset(dest + x, 0, n - x);
	return (char*)dest;
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
	return (*(unsigned char*)s1 - *(unsigned char*)s2);
}

int strncmp(const char *s1, const char *s2, size_t n) {
	size_t i = 0;
	while ((*s1) && (*s1 == *s2) && i != n) {
		s1++;
		s2++;
		i++;
	}
	if (i == n) return 0;
	return (*(unsigned char*)s1 - *(unsigned char*)s2);
}

void *memcpy(void *vd, const void *vs, size_t count) {
	uint8_t *dest = (uint8_t*)vd, *src = (uint8_t*)vs;
	size_t f = count % 4, n = count / 4, i;
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

void *memmove(void *vd, const void *vs, size_t count) {
	uint8_t *dest = (uint8_t*)vd, *src = (uint8_t*)vs;

	if (vd < vs) {
		for (size_t i = 0; i < count; i++)
			dest[i] = src[i];
	} else {
		for (size_t i = 0; i < count; i++)
			dest[count - i] = src[count - i];
	}
	return vd;
}

int memcmp(const void *va, const void *vb, size_t count) {
	uint8_t *a = (uint8_t*)va;
	uint8_t *b = (uint8_t*)vb;
	for (size_t i = 0; i < count; i++) {
		if (a[i] != b[i]) return (int)a[i] - (int)b[i];
	}
	return 0;
}

void *memset(void *dest, int val, size_t count) {
	uint8_t uval = (val & 0xFF);
	uint32_t wval = (uval<<24)|(uval<<16)|(uval<<8)|uval;

	uint32_t *dest_w = (uint32_t*)dest;
	for (size_t i = 0; i < count/4; i++) {
		dest_w[i] = wval;
	}

	uint8_t *dest_c = (uint8_t*)dest;
	for (size_t i = count - (count%4); i < count; i++) {
		dest_c[i] = uval;
	}
	return dest;
}

char *strdup(const char* str) {
	size_t len = strlen(str) + 1;

	char* ret = (char*)malloc(len);
	if (ret == 0) return 0;

	memcpy(ret, str, len);
	return ret;
}

char *strndup(const char* str, size_t count) {
	size_t len = strlen(str);
	if (count < len) len = count;

	char* ret = (char*)malloc(len + 1);
	if (ret == 0) return 0;

	memcpy(ret, str, len);
	ret[len] = 0;
	return ret;
}

int strcoll(const char *s1, const char *s2) {
	// TODO locale handling
	return strcmp(s1, s2);
}

void *memchr(const void *s, int c, size_t n) {
	unsigned char *p = (unsigned char*)s;
	for (size_t i = 0; i < n; i++) {
		if (p[i] == (unsigned char)c)
			return &p[i];
	}
	return NULL;
}

size_t strspn(const char *s, const char *accept) {
	size_t l = 0;
	while (s[l] != 0) {
		bool ok = false;
		for (const char* p = accept; *p != 0; p++) {
			if (s[l] == *p) {
				ok = true;
				break;
			}
		}
		if (!ok) break;
		l++;
	}
	return l;
}

const char *strstr(const char *haystack, const char *needle) {
	int n = strlen(needle);
	for (const char* p = haystack; *p != 0; p++) {
		if (!strncmp(p, needle, n)) return p;
	}
	return NULL;
}

char* strerror(int errnum) {
	// TODO
	return "(unspecified error)";
}

const char *strpbrk(const char *s, const char *accept) {
	while (*s) {
		for (const char *p = accept; *p != 0; p++) {
			if (*s == *p) return s;
		}
		s++;
	}
	return NULL;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
