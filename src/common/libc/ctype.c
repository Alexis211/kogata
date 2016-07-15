#include <ctype.h>


int isalnum(int c) {
	return isalpha(c) || isdigit(c);
}
int isalpha(int c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
int isdigit(int c) {
	return (c >= '0' && c <= '9');
}
int isxdigit(int c) {
	return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
int isspace(int c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}
int isprint(int c) {
	return (c >= ' ' && c < 256) || isspace(c);
}
int isupper(int c) {
	return c >= 'A' && c <= 'Z';
}
int islower(int c) {
	return c >= 'a' && c <= 'z';
}
int ispunct(int c) {
	return isprint(c) && !isspace(c);
}
int isgraph(int c) {
	return c > ' ' && c < 256;
}
int iscntrl(int c) {
	return c > 0 && c < ' ';
}

int toupper(int c) {
	if (islower(c))
		return c + 'A' - 'a';
	else
		return c;
}
int tolower(int c) {
	if (isupper(c))
		return c + 'a' - 'A';
	else
		return c;
}

/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
