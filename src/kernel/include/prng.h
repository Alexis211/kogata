#pragma once

#include <stdint.h>
#include <stddef.h>

void prng_add_entropy(const uint8_t* ptr, size_t nbytes);	// add entropy to generator

uint16_t prng_word();
void prng_bytes(uint8_t* out, size_t nbytes);				// generate some bytes

/* vim: set ts=4 sw=4 tw=0 noet :*/
