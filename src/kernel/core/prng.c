#include <prng.h>
#include <thread.h>

#define EPOOLSIZE 2048

static uint32_t x = 211;
static int n = 0;
static char entropy[EPOOLSIZE];
static int entropy_count = 0;
static const uint32_t a = 16807;
static const uint32_t m = 0x7FFFFFFF;

uint16_t prng_word() {
	if (++n >= 100) {
		n = 0;
		if (entropy_count) {
			x += entropy[--entropy_count];
		}
	}
	x = (x * a) % m;
	return x & 0xFFFF;
}

void prng_bytes(uint8_t* out, size_t nbytes) {
	uint16_t *d = (uint16_t*)out;
	for (size_t i = 0; i < nbytes / 2; i++) {
		d[i] = prng_word();
	}
	if (nbytes & 1) out[nbytes-1] = prng_word() & 0xFF;
}

void prng_add_entropy(const uint8_t* ptr, size_t nbytes) {
	int st = enter_critical(CL_NOINT);

	while (nbytes-- && entropy_count < EPOOLSIZE - 1) {
		entropy[entropy_count++] = *(ptr++);
	}

	exit_critical(st);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
