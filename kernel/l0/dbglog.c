#include <stdarg.h>
#include <string.h>
#include <printf.h>
#include <dbglog.h>
#include <config.h>
#include <sys.h>

// ==================================================================

#ifdef DBGLOG_TO_SCREEN

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

static uint8_t vga_color = 7;
static uint16_t* vga_buffer = 0;
static uint16_t vga_row = 0, vga_column = 0;

static uint16_t make_vgaentry(char c, uint8_t color) {
	uint16_t c16 = c;
	uint16_t color16 = color;
	return c16 | color16 << 8;
}

static void vga_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	vga_buffer[index] = make_vgaentry(c, color);
}

static void vga_update_cursor() {
	uint16_t cursor_location = vga_row * 80 + vga_column;
	outb(0x3D4, 14);	//Sending high cursor byte
	outb(0x3D5, cursor_location >> 8);
	outb(0x3D4, 15);	//Sending high cursor byte
	outb(0x3D5, cursor_location);
}
 
static void vga_init() {
	vga_row = 0;
	vga_column = 0;
	vga_buffer = (uint16_t*) (&k_highhalf_addr + 0xB8000);

	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			vga_putentryat(' ', vga_color, x, y);
		}
	}

	vga_update_cursor();
}

static void vga_scroll() {
	for (size_t i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
		vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
	}
	for (size_t x = 0; x < VGA_WIDTH; x++) {
		vga_putentryat(' ', vga_color, x, VGA_HEIGHT - 1);
	}
	vga_row--;
}

static void vga_newline() {
	vga_column = 0;
	if (++vga_row == VGA_HEIGHT) {
		vga_scroll();
	}
}
 
static void vga_putc(char c) {
	if (c == '\n') {
		vga_newline();
	} else if (c == '\t') {
		vga_putc(' ');
		while (vga_column % 4 != 0) vga_putc(' ');
	} else {
		vga_putentryat(c, vga_color, vga_column, vga_row);
		if (++vga_column == VGA_WIDTH) {
			vga_newline();
		}
	}
}
 
static void vga_puts(const char* data) {
	size_t datalen = strlen(data);
	for (size_t i = 0; i < datalen; i++)
		vga_putc(data[i]);

	vga_update_cursor();
}

#endif	// DBGLOG_TO_SCREEN

// ==================================================================

#ifdef DBGLOG_TO_SERIAL

#define SER_PORT 0x3F8		// COM1

static void serial_init() {
	outb(SER_PORT + 1, 0x00);		// disable interrupts
	outb(SER_PORT + 3, 0x80);		// set baud rate
	outb(SER_PORT + 0, 0x03);		// set divisor to 3 (38400 baud)
	outb(SER_PORT + 1, 0x00);
	outb(SER_PORT + 3, 0x03);		// 8 bits, no parity, one stop bit
	outb(SER_PORT + 2, 0xC7);		// enable FIFO, clear them, with 14-byte threshold
}

static void serial_putc(const char c) {
	while (!(inb(SER_PORT + 5) & 0x20));
	outb(SER_PORT, c);
}

static void serial_puts(const char *c) {
	while (*c) {
		serial_putc(*c);
		c++;
	}
}

#endif	// DBGLOG_TO_SERIAL

// ==================================================================

void dbglog_setup() {
#ifdef DBGLOG_TO_SCREEN
	vga_init();
#endif
#ifdef DBGLOG_TO_SERIAL
	serial_init();
#endif
}

void dbg_print(const char* str) {
#ifdef DBGLOG_TO_SCREEN
	vga_puts(str);
#endif
#ifdef DBGLOG_TO_SERIAL
	serial_puts(str);
#endif
}

void dbg_printf(const char* fmt, ...) {
	va_list ap;
	char buffer[256];

	va_start(ap, fmt);
	vsnprintf(buffer, 256, fmt, ap);
	va_end(ap);

	dbg_print(buffer);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
