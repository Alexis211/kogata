#include <locale.h>
#include <stddef.h>
#include <limits.h>

struct lconv c_locale_lconv = {
	".", "", "", "", "", "", "", "",
	"", "", CHAR_MAX, CHAR_MAX, CHAR_MAX,
	CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
	CHAR_MAX
};

struct lconv *localeconv() {
	// TODO
	return &c_locale_lconv;
}

char* setlocale(int catebory, const char* locale) {
	// TODO
	return NULL;
}

/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
