#pragma once

#include <config.h>

void outb(uint16_t port, uint8_t value);
void outw(uint16_t port, uint16_t value);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);


void panic(const char* message, const char* file, int line);
void panic_assert(const char* assertion, const char* file, int line);
#define PANIC(s) panic(s, __FILE__, __LINE__);
#define ASSERT(s) { if (!(s)) panic_assert(#s, __FILE__, __LINE__); }
