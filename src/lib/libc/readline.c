#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <readline/readline.h>

// ** READLINE

#define READLINE_MAX_LEN 256

typedef struct _rdln_hist {
	int max;
	int n;
	char **str;
} readline_history;

readline_history stdio_history = {0, 0, 0};


char *readline(const char* prompt) {
	// readline_history *h = &stdio_history;

	puts(prompt);

	char* buf = malloc(READLINE_MAX_LEN);
	if (buf == NULL) return NULL;

	char* ret = fgets(buf, READLINE_MAX_LEN, stdin);
	if (ret == NULL) {
		free(buf);
		return NULL;
	}

	int n = strlen(ret);
	if (n > 0 && ret[n-1] == '\n') ret[n-1] = 0;

	// TODO

	return ret;
}

void add_history(const char* line) {
	// readline_history *h = &stdio_history;

	// TODO
}


/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
