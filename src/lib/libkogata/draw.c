#include <stdlib.h>
#include <string.h>

#include <proto/font_file.h>

#include <kogata/printf.h>
#include <kogata/region_alloc.h>

#include <kogata/syscall.h>
#include <kogata/draw.h>

// ----

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#include "stb/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

// ----

fb_t *g_fb_from_file(fd_t file, fb_info_t *geom) {
	fb_t *ret = (fb_t*)malloc(sizeof(fb_t));
	if (ret == 0) return 0;

	memcpy(&ret->geom, geom, sizeof(fb_info_t));
	ret->fd = file;

	ret->data = (uint8_t*)region_alloc(geom->height * geom->pitch, "Framebuffer");
	if (ret->data == 0) goto error;

	bool map_ok = sc_mmap_file(file, 0, ret->data, geom->height * geom->pitch, MM_READ | MM_WRITE);
	if (!map_ok) goto error;

	ret->nrefs = 1;

	return ret;

error:
	if (ret && ret->data) region_free(ret->data);
	if (ret) free(ret);
	return 0;
}

fb_t *g_fb_from_mem(uint8_t* data, fb_info_t *geom, bool own_data) {
	fb_t *ret = (fb_t*)malloc(sizeof(fb_t));
	if (ret == 0) return 0;

	memcpy(&ret->geom, geom, sizeof(fb_info_t));
	ret->fd = 0;
	ret->data = data;

	ret->nrefs = 1;
	ret->own_data = own_data;

	return ret;
}

fb_t *g_load_image(const char* filename) {
	int w, h, n;
	uint8_t *data = stbi_load(filename, &w, &h, &n, 0);
	if (data == NULL) {
		return NULL;
	}

	fb_info_t geom;
	geom.width = w;
	geom.height = h;
	geom.pitch = w * n;
	geom.bpp = n * 8;
	if (n == 1) geom.memory_model = FB_MM_GREY8;
	if (n == 2) geom.memory_model = FB_MM_GA16;
	if (n == 3) geom.memory_model = FB_MM_RGB24;
	if (n == 4) geom.memory_model = FB_MM_RGBA32;

	fb_t *fb = g_fb_from_mem(data, &geom, true);
	if (fb == NULL) {
		free(data);
		return NULL;
	}

	return fb;
}

void g_incref_fb(fb_t *fb) {
	fb->nrefs++;
}

void g_decref_fb(fb_t *fb) {
	fb->nrefs--;
	if (fb->nrefs == 0) {
		if (fb->fd != 0) {
			sc_munmap(fb->data);
			region_free(fb->data);
		} else if (fb->own_data) {
			free(fb->data);
		}
		free(fb);
	}
}

//  ---- Color management

color_t g_color_rgb(fb_t *f, uint8_t r, uint8_t g, uint8_t b) {
	int m = f->geom.memory_model;
	if (m == FB_MM_RGB24 || m == FB_MM_RGB32 || m == FB_MM_RGBA32) return (r << 16) | (g << 8) | b;
	if (m == FB_MM_BGR24 || m == FB_MM_BGR32) return (b << 16) | (g << 8) | r;
	if (m == FB_MM_GREY8 || m == FB_MM_GA16) return ((r + g + b) / 3) & 0xFF;
	if (m == FB_MM_RGB16 || m == FB_MM_RGB15) return ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
	if (m == FB_MM_BGR16 || m == FB_MM_BGR15) return ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
	return 0;	// unknown?
}

//  ---- Drawing functions

static inline void g_plot24(uint8_t* p, color_t c) {
	p[0] = c & 0xFF;
	p[1] = (c >> 8) & 0xFF;
	p[2] = (c >> 16) & 0xFF;
}

// # Plot #

inline void g_plot(fb_t *fb, int x, int y, color_t c) {
	if (x < 0 || y < 0 || x >= fb->geom.width || y >= fb->geom.height) return;

	if (fb->geom.bpp == 8) {
		fb->data[y * fb->geom.pitch + x] = (c & 0xFF);
	} else if (fb->geom.bpp == 15 || fb->geom.bpp == 16) {
		uint16_t *p = (uint16_t*)(fb->data + y * fb->geom.pitch + 2 * x);
		*p = (c & 0xFFFF);
	} else if (fb->geom.bpp == 24) {
		g_plot24(fb->data + y * fb->geom.pitch + 3 * x, c);
	} else if (fb->geom.bpp == 32) {
		uint32_t *p = (uint32_t*)(fb->data + y * fb->geom.pitch + 4 * x);
		*p = c;
	}
}

inline void g_region_plot(fb_t *fb, fb_region_t *reg, int x, int y, color_t c) {
	if (x < 0 || y < 0 || x >= reg->w || y >= reg->h) return;
	g_plot(fb, x + reg->x, y + reg->y, c);
}


// # Horizontal line #

inline void g_hline(fb_t *fb, int x, int y, int w, color_t c) {
	if (x < 0) w = w + x, x = 0;
	if (x >= fb->geom.width) return;
	if (x + w < 0) return;
	if (x + w > fb->geom.width) w = fb->geom.width - x;
	if (y < 0 || y >= fb->geom.height) return;
	if (w <= 0) return;

	if (fb->geom.bpp == 8) {
		for (int u = x; u < x + w; u++) {
			fb->data[y * fb->geom.pitch + u] = (c & 0xFF);
		}
	} else if (fb->geom.bpp == 15 || fb->geom.bpp == 16) {
		for (int u = x; u < x + w; u++) {
			uint16_t *p = (uint16_t*)(fb->data + y * fb->geom.pitch + 2 * u);
			*p = (c & 0xFFFF);
		}
	} else if (fb->geom.bpp == 24) {
		for (int u = x; u < x + w; u++) {
			g_plot24(fb->data + y * fb->geom.pitch + 3 * u, c);
		}
	} else if (fb->geom.bpp == 32) {
		for (int u = x; u < x + w; u++) {
			uint32_t *p = (uint32_t*)(fb->data + y * fb->geom.pitch + 4 * u);
			*p = c | 0xFF000000;	// alpha = 0xFF
		}
	}
}

inline void g_region_hline(fb_t *fb, fb_region_t *reg, int x, int y, int w, color_t c) {
	if (x < 0) w = w + x, x = 0;
	if (x >= reg->w) return;
	if (x + w < 0) return;
	if (x + w > reg->w) w = reg->w - x;
	if (y < 0 || y >= reg->h) return;
	if (w <= 0) return;

	g_hline(fb, reg->x + x, reg->y + y, w, c);
}


// # Vertical line #

inline void g_vline(fb_t *fb, int x, int y, int h, color_t c) {
	if (y < 0) h = h + y, y = 0;
	if (y >= fb->geom.height) return;
	if (y + h < 0) return;
	if (y + h > fb->geom.width) h = fb->geom.height - y;
	if (x < 0 || x >= fb->geom.width) return;
	if (h <= 0) return;

	if (fb->geom.bpp == 8) {
		for (int v = y; v < y + h; v++) {
			fb->data[v * fb->geom.pitch + x] = (c & 0xFF);
		}
	} else if (fb->geom.bpp == 15 || fb->geom.bpp == 16) {
		for (int v = y; v < y + h; v++) {
			uint16_t *p = (uint16_t*)(fb->data + v * fb->geom.pitch + 2 * x);
			*p = (c & 0xFFFF);
		}
	} else if (fb->geom.bpp == 24) {
		for (int v = y; v < y + h; v++) {
			g_plot24(fb->data + v * fb->geom.pitch + 3 * x, c);
		}
	} else if (fb->geom.bpp == 32) {
		for (int v = y; v < y + h; v++) {
			uint32_t *p = (uint32_t*)(fb->data + v * fb->geom.pitch + 4 * x);
			*p = c;
		}
	}
}

inline void g_region_vline(fb_t *fb, fb_region_t *reg, int x, int y, int h, color_t c) {
	if (y < 0) h = h + y, y = 0;
	if (y >= reg->h) return;
	if (y + h < 0) return;
	if (y + h > reg->h) h = reg->h - y;
	if (x < 0 || x >= reg->w) return;
	if (h <= 0) return;

	g_vline(fb, reg->x + x, reg->y + y, h, c);
}


// # Line #

void g_line(fb_t *fb, int x1, int y1, int x2, int y2, color_t c) {
	// TODO
}

void g_region_line(fb_t *fb, fb_region_t *reg, int x1, int y1, int x2, int y2, color_t c) {
	// TODO
}


// # Rectangle #

void g_rect(fb_t *fb, int x, int y, int w, int h, color_t c) {
	g_hline(fb, x, y, w, c);
	g_hline(fb, x, y+h-1, w, c);
	g_vline(fb, x, y, h, c);
	g_vline(fb, x+w-1, y, h, c);
}

void g_region_rect(fb_t *fb, fb_region_t *reg, int x, int y, int w, int h, color_t c) {
	g_region_hline(fb, reg, x, y, w, c);
	g_region_hline(fb, reg, x, y+h-1, w, c);
	g_region_vline(fb, reg, x, y, h, c);
	g_region_vline(fb, reg, x+w-1, y, h, c);
}


// # Filled rectangle #

void g_fillrect(fb_t *fb, int x, int y, int w, int h, color_t c) {
	if (x < 0) w = w + x, x = 0;
	if (y < 0) h = h + y, y = 0;

	if (x + w > fb->geom.width) w = fb->geom.width - x;
	if (y + h > fb->geom.height) h = fb->geom.height - y;

	if (w <= 0 || h <= 0) return;

	if (fb->geom.bpp == 8) {
		for (int v = y; v < y + h; v++) {
			for (int u = x; u < x + w; u++) {
				fb->data[v * fb->geom.pitch + u] = (c & 0xFF);
			}
		}
	} else if (fb->geom.bpp == 15 || fb->geom.bpp == 16) {
		for (int v = y; v < y + h; v++) {
			for (int u = x; u < x + w; u++) {
				uint16_t *p = (uint16_t*)(fb->data + v * fb->geom.pitch + 2 * u);
				*p = (c & 0xFFFF);
			}
		}
	} else if (fb->geom.bpp == 24) {
		for (int v = y; v < y + h; v++) {
			for (int u = x; u < x + w; u++) {
				g_plot24(fb->data + v * fb->geom.pitch + 3 * u, c);
			}
		}
	} else if (fb->geom.bpp == 32) {
		for (int v = y; v < y + h; v++) {
			for (int u = x; u < x + w; u++) {
				uint32_t *p = (uint32_t*)(fb->data + v * fb->geom.pitch + 4 * u);
				*p = c;
			}
		}
	}
}

void g_region_fillrect(fb_t *fb, fb_region_t *reg, int x, int y, int w, int h, color_t c) {
	if (x < 0) w = w + x, x = 0;
	if (y < 0) h = h + y, y = 0;

	if (x + w > reg->w) w = reg->w - x;
	if (y + h > reg->h) h = reg->h - y;

	if (w <= 0 || h <= 0) return;

	g_fillrect(fb, x + reg->x, y + reg->y, w, h, c);
}


// # Circle #

void g_circle(fb_t *fb, int cx, int cy, int r, color_t c) {
	// TODO
}

void g_region_circle(fb_t *fb, fb_region_t *reg, int cx, int cy, int r, color_t c) {
	// TODO
}


// # Filled circle #

void g_fillcircle(fb_t *fb, int cx, int cy, int r, color_t c) {
	// TODO
}

void g_region_fillcircle(fb_t *fb, fb_region_t *reg, int cx, int cy, int r, color_t c) {
	// TODO
}


// # Blit #

void g_blit(fb_t *dst, int x, int y, fb_t *src) {
	fb_region_t r;

	r.x = 0;
	r.y = 0;
	r.w = src->geom.width;
	r.h = src->geom.height;

	g_blit_region(dst, x, y, src, r);
}

void g_region_blit(fb_t *dst, fb_region_t *reg, int x, int y, fb_t *src) {
	fb_region_t r;

	r.x = 0;
	r.y = 0;
	r.w = src->geom.width;
	r.h = src->geom.height;

	g_region_blit_region(dst, reg, x, y, src, r);
}

void g_blit_region(fb_t *dst, int x, int y, fb_t *src, fb_region_t reg) {
	if (x < 0 || y < 0 || reg.x < 0 || reg.y < 0 || reg.w < 0 || reg.h < 0) return;	// invalid argument

	if (reg.x + reg.w > src->geom.width) reg.w = src->geom.width - reg.x;
	if (reg.y + reg.h > src->geom.height) reg.h = src->geom.height - reg.y;
	if (reg.w <= 0 || reg.h <= 0) return;

	if (x + reg.w > dst->geom.width) reg.w = dst->geom.width - x;
	if (y + reg.h > dst->geom.height) reg.h = dst->geom.height - y;
	if (reg.w <= 0 || reg.h <= 0) return;

	if (src->geom.memory_model == dst->geom.memory_model
			&& src->geom.memory_model != FB_MM_RGBA32
			&& src->geom.memory_model != FB_MM_GA16) {
		for (int i = 0; i < reg.h; i++) {
			memcpy(
				dst->data + (y + i) * dst->geom.pitch + x * (dst->geom.bpp / 8),
				src->data + (reg.y + i) * src->geom.pitch + reg.x * (src->geom.bpp / 8),
				reg.w * (src->geom.bpp / 8));
		}
	} else if (src->geom.memory_model == FB_MM_RGBA32 && dst->geom.memory_model == FB_MM_RGB32) {
		for (int i = 0; i < reg.h; i++) {
			uint8_t *dln = dst->data + (y + i) * dst->geom.pitch + x * 4;
			uint8_t *sln = src->data + (reg.y + i) * src->geom.pitch + reg.x * 4;
			uint32_t *dln32 = (uint32_t*) dln;
			uint32_t *sln32 = (uint32_t*) sln;
			for (int j = 0; j < reg.w; j++) {
				uint8_t a = sln[4*j+3];
				if (a == 0xFF) {
					dln32[j] = sln32[j];
				} else if (a > 0) {
					uint16_t r = (0x100 - a) * (uint16_t)dln[4*j] + a * (uint16_t)sln[4*j];
					uint16_t g = (0x100 - a) * (uint16_t)dln[4*j+1] + a * (uint16_t)sln[4*j+1];
					uint16_t b = (0x100 - a) * (uint16_t)dln[4*j+2] + a * (uint16_t)sln[4*j+2];
					dln[4*j] = r >> 8;
					dln[4*j+1] = g >> 8;
					dln[4*j+2] = b >> 8;
				}
			}
		}
	} else if (src->geom.memory_model == FB_MM_RGBA32 && dst->geom.memory_model == FB_MM_RGB24) {
		for (int i = 0; i < reg.h; i++) {
			uint8_t *dln = dst->data + (y + i) * dst->geom.pitch + x * 4;
			uint8_t *sln = src->data + (reg.y + i) * src->geom.pitch + reg.x * 4;
			for (int j = 0; j < reg.w; j++) {
				uint8_t a = sln[4*j+3];
				if (a == 0xFF) {
					dln[3*j] = sln[4*j];
					dln[3*j+1] = sln[4*j+1];
					dln[3*j+2] = sln[4*j+2];
				} else if (a > 0) {
					uint16_t r = (0x100 - a) * (uint16_t)dln[3*j] + a * (uint16_t)sln[4*j];
					uint16_t g = (0x100 - a) * (uint16_t)dln[3*j+1] + a * (uint16_t)sln[4*j+1];
					uint16_t b = (0x100 - a) * (uint16_t)dln[3*j+2] + a * (uint16_t)sln[4*j+2];
					dln[3*j] = r >> 8;
					dln[3*j+1] = g >> 8;
					dln[3*j+2] = b >> 8;
				}
			}
		}
	} else {
		dbg_printf("Unsupported blit between different memory models %d and %d.\n", src->geom.memory_model, dst->geom.memory_model);
	}
}

void g_region_blit_region(fb_t *dst, fb_region_t *reg, int x, int y, fb_t *src, fb_region_t r) {
	if (x + r.w > reg->w) r.w = reg->w - x;
	if (y + r.h > reg->h) r.h = reg->h - y;

	g_blit_region(dst, x + reg->x, y + reg->y, src, r);
}

// # Scroll #

void g_scroll_up(fb_t *dst, int l) {
	for (int y = 0; y < dst->geom.height - l; y++) {
		memcpy(dst->data + y * dst->geom.pitch,
				dst->data + (y + l) * dst->geom.pitch,
				dst->geom.pitch);
	}
}

void g_region_scroll_up(fb_t *fb, fb_region_t *reg, int l) {
	// TODO
}

//  ---- Text manipulation

#define FONT_ASCII_BITMAP	1
#define FONT_STBTT			2
// more font types to come

typedef struct font {
	int type;
	union {
		struct {
			uint8_t* data;
			uint8_t cw, ch;		// width, height of a character
			uint32_t nchars;
		} ascii_bitmap;
		struct {
			uint8_t* data;
			stbtt_fontinfo info;
			float scale;
		} stbtt;
	};
	int nrefs;
} font_t;

font_t *g_load_ascii_bitmap_font(const char* filename) {
	fd_t f = sc_open(filename, FM_READ);
	if (f == 0) return NULL;

	font_t *font = 0;

	ascii_bitmap_font_header h;

	size_t s = sc_read(f, 0, sizeof(h), (char*)&h);
	if (s != sizeof(h)) goto error;

	if (h.magic != ASCII_BITMAP_FONT_MAGIC) goto error;
	if (h.cw != 8) goto error;

	font = malloc(sizeof(font_t));
	if (font == 0) goto error;
	memset(font, 0, sizeof(font_t));

	font->type = FONT_ASCII_BITMAP;
	font->ascii_bitmap.cw = h.cw;
	font->ascii_bitmap.ch = h.ch;
	font->ascii_bitmap.nchars = h.nchars;

	font->ascii_bitmap.data = (uint8_t*)malloc(h.ch * h.nchars);
	if (font->ascii_bitmap.data == 0) goto error;

	size_t rd = sc_read(f, sizeof(h), h.ch * h.nchars, (char*)font->ascii_bitmap.data);
	if (rd != h.ch * h.nchars) goto error;

	font->nrefs = 1;

	return font;

error:
	if (font && font->ascii_bitmap.data) free(font->ascii_bitmap.data);
	if (font) free(font);
	sc_close(f);
	return NULL;
}

font_t *g_load_ttf_font(const char* filename) {
	fd_t f = sc_open(filename, FM_READ);
	if (f == 0) return NULL;

	font_t *font = 0;
	font = malloc(sizeof(font_t));
	if (font == 0) goto error;

	font->type = FONT_STBTT;
	font->stbtt.data = NULL;

	stat_t st;
	bool ok = sc_stat_open(f, &st);
	if (!ok) goto error;

	font->stbtt.data = malloc(st.size);
	if (font->stbtt.data == NULL) goto error;

	size_t ret = sc_read(f, 0, st.size, (void*)font->stbtt.data);
	if (ret < st.size) goto error;

	if (!stbtt_InitFont(&font->stbtt.info, font->stbtt.data, 0)) {
		goto error;
	}

	font->nrefs = 1;
	return font;


error:
	if (font->stbtt.data) free(font->stbtt.data);
	if (font) free(font);
	sc_close(f);
	return NULL;
}

void g_incref_font(font_t *f) {
	f->nrefs++;
}

void g_decref_font(font_t *f) {
	f->nrefs--;

	if (f->nrefs == 0) {
		if (f->type == FONT_ASCII_BITMAP) {
			free(f->ascii_bitmap.data);
		}
		free(f);
	}
}

int g_text_width(font_t *font, const char* text, int size) {
	if (font->type == FONT_ASCII_BITMAP) {
		return font->ascii_bitmap.cw * strlen(text);
	} else if (font->type == FONT_STBTT) {
		float scale = stbtt_ScaleForPixelHeight(&font->stbtt.info, size);
		float xpos = 1;
		while (*text != 0) {
			int codepoint = *text;	// TODO utf8
			text++;

			int advance, lsb;
			stbtt_GetCodepointHMetrics(&font->stbtt.info, codepoint, &advance, &lsb);
			xpos += scale * advance;
			if (*(text+1))
				xpos += scale * stbtt_GetCodepointKernAdvance(&font->stbtt.info, *text, *(text+1));
		}
		return ceil(xpos);

	}
	return 0;
}

int g_text_height(font_t *font, const char* text, int size) {
	if (font->type == FONT_ASCII_BITMAP) {
		return font->ascii_bitmap.ch;
	} else if (font->type == FONT_STBTT) {
		float scale = stbtt_ScaleForPixelHeight(&font->stbtt.info, size);
		int max_h = 0;
		while (*text != 0) {
			int codepoint = *text;	// TODO utf8
			text++;

			int x0, y0, x1, y1;
			stbtt_GetCodepointBitmapBox(&font->stbtt.info, codepoint, scale, scale, &x0, &y0, &x1, &y1);
			if (y1 - y0 > max_h) max_h = y1 - y0;
		}
		return max_h;
	}
	return 0;
}

void g_write(fb_t *fb, int x, int y, const char* text, font_t *font, int size, color_t c) {
	fb_region_t r;
	r.x = 0;
	r.y = 0;
	r.w = fb->geom.width;
	r.h = fb->geom.height;

	g_region_write(fb, &r, x, y, text, font, size, c);
}

void g_region_write(fb_t *fb, fb_region_t *reg, int x, int y, const char* text, font_t *font, int size, color_t c) {
	if (font->type == FONT_ASCII_BITMAP) {
		while (*text != 0) {
			uint8_t id = (uint8_t)*text;
			if (id < font->ascii_bitmap.nchars) {
				uint8_t *d = font->ascii_bitmap.data + (id * font->ascii_bitmap.ch);
				for (int r = 0; r < font->ascii_bitmap.ch; r++) {
					int yy = y + reg->y + r;
					if (y + r >= reg->h || yy >= fb->geom.height) continue;
					for (int j = 0; j < 8; j++) {
						int xx = x + reg->x + j;
						if (x + j >= reg->w || xx >= fb->geom.width) continue;
						if (d[r] & (0x80 >> j)) {
							g_plot(fb, xx, yy, c);
						}
					}
				}
			}
			text++;
			x += font->ascii_bitmap.cw;
		}
	} else if (font->type == FONT_STBTT) {
		float scale = stbtt_ScaleForPixelHeight(&font->stbtt.info, size);
		float xpos = 1;

		uint8_t *tmp = (uint8_t*)malloc(size*size*2);
		size_t tmp_size = size*size*2;
		if (tmp == NULL) return;

		while (*text != 0) {
			int codepoint = *text;	// TODO utf8
			text++;

			int advance, lsb;
			int x0, y0, x1, y1;
			float x_shift = xpos - (float)floor(xpos);
			stbtt_GetCodepointHMetrics(&font->stbtt.info, codepoint, &advance, &lsb);
			stbtt_GetCodepointBitmapBoxSubpixel(&font->stbtt.info, codepoint, scale, scale, x_shift, 0, &x0, &y0, &x1, &y1);

			int w = x1 - x0, h = y1 - y0;
			if (tmp_size < (size_t)w*h) {
				free(tmp);
				tmp_size = 2*w*h;
				tmp = (uint8_t*)malloc(tmp_size);
				if (tmp == NULL) return;
			}

			stbtt_MakeCodepointBitmapSubpixel(&font->stbtt.info, tmp, w, h, w, scale, scale, x_shift, 0, codepoint);
			if (fb->geom.memory_model == FB_MM_RGB32 || fb->geom.memory_model == FB_MM_RGB24) {
				int sx = (fb->geom.memory_model == FB_MM_RGB32 ? 4 : 3);

				for (int i = 0; i < h; i++) {
					int yy = y + size + y0 + i;
					if (yy < 0) continue;
					if (yy >= reg->h) continue;
					yy += reg->y;
					if (yy < 0) continue;
					if (yy >= fb->geom.height) continue;
					uint8_t *line = fb->data + yy * fb->geom.pitch;

					for (int j = 0; j < w; j++) {
						int xx = x + x0 + (int)xpos + j;
						if (xx < 0) continue;
						if (xx >= reg->w) continue;
						xx += reg->x;
						if (xx < 0) continue;
						if (xx >= fb->geom.width) continue;

						uint8_t a = tmp[i*w+j];
						uint8_t *px = line + sx * xx;
						if (a == 0xFF) {
							g_plot24(px, c);
						} else if (a > 0) {
							uint16_t r = a * (c & 0xFF) + (0x100 - a) * px[0];
							uint16_t g = a * ((c>>8) & 0xFF) + (0x100 - a) * px[1];
							uint16_t b = a * ((c>>16) & 0xFF) + (0x100 - a) * px[2];
							px[0] = r>>8;
							px[1] = g>>8;
							px[2] = b>>8;
						}
					}
				}
			}
			xpos += scale * advance;
			if (*(text+1))
				xpos += scale * stbtt_GetCodepointKernAdvance(&font->stbtt.info, *text, *(text+1));
		}

		free(tmp);
	}
}



/* vim: set ts=4 sw=4 tw=0 noet :*/
