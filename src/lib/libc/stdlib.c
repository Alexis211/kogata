#include <stdlib.h>
#include <ctype.h>

#include <kogata/debug.h>

int rand(void) {
	return 0;		// TODO
}

void srand(unsigned int seed) {
	//TODO
}

void abort() {
	PANIC("Aborted.");
}

float strtof(const char *nptr, const char **endptr) {
	return (float)strtod(nptr, endptr);
}
double strtod(const char *nptr, const char **endptr) {
	// TODO: better (inf, nan, ...)

	const char* p = nptr;
	while (isspace(*p)) p++;

	double val = 0;
	double sign = 1;
	if (*p == '-') sign = -1;
	if (*p == '-' || *p == '+') p++;
	while (isdigit(*p)) {
		val = val*10. + (double)((int)*p - '0');
		p++;
	}
	if (*p == '.') {
		p++;
		double fac = 0.1;
		while (isdigit(*p)) {
			val += fac * (double)((int)*p - '0');
			fac /= 10.;
			p++;
		}
	}
	if (*p == 'e' || *p == 'E') {
		p++;
		int exp = 0;
		int sexp = 1;
		if (*p == '-') sexp = -1;
		if (*p == '-' || *p =='+') p++;
		while (isdigit(*p)) {
			exp = exp * 10 + (*p - '0');
			p++;
		}
		if (sexp == 1) {
			for (int i = 0; i < exp; i++) val *= 10;
		} else {
			for (int i = 0; i < exp; i++) val /= 10;
		}
	}
	if (endptr != NULL) *endptr = p-1;

	return val * sign;
}

char *getenv(const char *name) {
	// TODO
	return 0;
}

int system(const char *command) {
	// TODO
	return -1;
}

int abs(int j) {
	if (j < 0) return -j;
	return j;
}

/* vim: set sts=0 ts=4 sw=4 tw=0 noet :*/
