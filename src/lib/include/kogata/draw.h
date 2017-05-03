#pragma once

#include <kogata/syscall.h>
#include <proto/fb.h>

//  ---- Generic drawing functions

//  ---- Data structures

typedef struct {
	fb_info_t geom;

	fd_t fd;
	uint8_t* data;

	int nrefs;
	bool own_data;
} fb_t;

typedef struct font font_t;

typedef uint32_t color_t;	// a color is always linked to a FB on which it is to be applied

//  ---- Buffer creation

fb_t *g_fb_from_file(fd_t file, fb_info_t *geom);
fb_t *g_fb_from_mem(uint8_t* region, fb_info_t *geom, bool own_data);

fb_t *g_load_image(const char* filename);

void g_incref_fb(fb_t *fb);
void g_decref_fb(fb_t *fb);

//  ---- Color manipulation

color_t g_color_rgb(fb_t *f, uint8_t r, uint8_t g, uint8_t b);

//  ---- Drawing primitives

void g_plot(fb_t *fb, int x, int y, color_t c);

void g_hline(fb_t *fb, int x, int y, int w, color_t c);		// horizontal line
void g_vline(fb_t *fb, int x, int y, int h, color_t c);		// vertical line
void g_line(fb_t *fb, int x1, int y1, int x2, int y2, color_t c);

void g_rect(fb_t *fb, int x, int y, int w, int h, color_t c);
void g_fillrect(fb_t *fb, int x, int y, int w, int h, color_t c);
void g_rectregion(fb_t *fb, fb_region_t reg, color_t c);
void g_fillregion(fb_t *fb, fb_region_t reg, color_t c);

void g_circle(fb_t *fb, int cx, int cy, int r, color_t c);
void g_fillcircle(fb_t *fb, int cx, int cy, int r, color_t c);

void g_blit(fb_t *dst, int x, int y, fb_t *src);
void g_blit_region(fb_t *dst, int x, int y, fb_t *src, fb_region_t reg);

void g_scroll_up(fb_t *fb, int l);

//  ---- Text manipulation

font_t *g_load_font(const char* filename);
void g_incref_font(font_t *f);
void g_decref_font(font_t *f);

int g_text_width(font_t *f, const char* text);
int g_text_height(font_t *f, const char* text);

void g_write(fb_t *fb, int x, int y, const char* text, font_t *font, color_t c);


/* vim: set ts=4 sw=4 tw=0 noet :*/
