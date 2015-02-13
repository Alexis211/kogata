#include <stdbool.h>

#include <debug.h>

int main(int argc, char **argv) {
	asm volatile("int $0x80");
	while(true);
}

