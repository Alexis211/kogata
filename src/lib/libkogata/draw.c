#include <syscall.h>
#include <malloc.h>
#include <string.h>
#include <printf.h>

#include <proto/font_file.h>

#include <user_region.h>

#include <draw.h>

fb_t *g_fb_from_file(fd_t file, fb_info_t *geom) {
	fb_t *ret = (fb_t*)malloc(sizeof(fb_t));
	if (ret == 0) return 0;

	memcpy(&ret->geom, geom, sizeof(fb_info_t));
	ret->fd = file;

	ret->data = (uint8_t*)region_alloc(geom->height * geom->pitch, "Framebuffer");
	if (ret->data == 0) goto error;

	bool map_ok = mmap_file(file, 0, ret->data, geom->height * geom->pitch, MM_READ | MM_WRITE);
	if (!map_ok) goto error;

	return ret;

error:
	if (ret && ret->data) region_free(ret->data);
	if (ret) free(ret);
	return 0;
}

fb_t *g_fb_from_mem(uint8_t* data, fb_info_t *geom) {
	fb_t *ret = (fb_t*)malloc(sizeof(fb_t));
	if (ret == 0) return 0;

	memcpy(&ret->geom, geom, sizeof(fb_info_t));
	ret->fd = 0;
	ret->data = data;

	return ret;
}

void g_delete_fb(fb_t *fb) {
	if (fb->fd != 0) {
		munmap(fb->data);
		region_free(fb->data);
	}
	free(fb);
}

//  ---- Color management

color_t g_color_rgb(fb_t *f, uint8_t r, uint8_t g, uint8_t b) {
	int m = f->geom.memory_model;
	if (m == FB_MM_RGB24 || m == FB_MM_RGB32) return (r << 16) | (g << 8) | b;
	if (m == FB_MM_BGR24 || m == FB_MM_BGR32) return (b << 16) | (g << 8) | r;
	if (m == FB_MM_GREY8) return ((r + g + b) / 3) & 0xFF;
	if (m == FB_MM_RGB16 || m == FB_MM_RGB15) return ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
	if (m == FB_MM_BGR16 || m == FB_MM_BGR15) return ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
	return 0;	// unknown?
}

//  ---- Plot

static inline void g_plot24(uint8_t* p, color_t c) {
	p[0] = c & 0xFF;
	p[1] = (c >> 8) & 0xFF;
	p[2] = (c >> 16) & 0xFF;
}

void g_plot(fb_t *fb, int x, int y, color_t c) {
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

void g_hline(fb_t *fb, int x, int y, int w, color_t c) {
	if (fb->geom.bpp == 8) {
		for (int u = x; u < x + w; u++) {
			fb->data[y * fb->geom.pitch + u] = (c & 0xFF);
		}
	} else if (fb->geom.bpp == 15 || fb->geom.bpp == 15) {
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
			*p = c;
		}
	}
}

void g_vline(fb_t *fb, int x, int y, int h, color_t c) {
	if (fb->geom.bpp == 8) {
		for (int v = y; v < y + h; v++) {
			fb->data[v * fb->geom.pitch + x] = (c & 0xFF);
		}
	} else if (fb->geom.bpp == 15 || fb->geom.bpp == 15) {
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

void g_line(fb_t *fb, int x1, int y1, int x2, int y2, color_t c) {
	// TODO
}

void g_rect(fb_t *fb, int x, int y, int w, int h, color_t c) {
	g_hline(fb, x, y, w, c);
	g_hline(fb, x, y+h-1, w, c);
	g_vline(fb, x, y, h, c);
	g_vline(fb, x+w-1, y, h, c);
}

void g_fillrect(fb_t *fb, int x, int y, int w, int h, color_t c) {
	if (fb->geom.bpp == 8) {
		for (int v = y; v < y + h; v++) {
			for (int u = x; u < x + w; u++) {
				fb->data[v * fb->geom.pitch + u] = (c & 0xFF);
			}
		}
	} else if (fb->geom.bpp == 15 || fb->geom.bpp == 15) {
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

void g_rect_r(fb_t *fb, fb_region_t reg, color_t c) {
	g_rect(fb, reg.x, reg.y, reg.w, reg.h, c);
}

void g_fillrect_r(fb_t *fb, fb_region_t reg, color_t c) {
	g_fillrect(fb, reg.x, reg.y, reg.w, reg.h, c);
}

void g_circle(fb_t *fb, int cx, int cy, int r, color_t c) {
	// TODO
}

void g_fillcircle(fb_t *fb, int cx, int cy, int r, color_t c) {
	// TODO
}

void g_blit(fb_t *dst, int x, int y, fb_t *src) {
	fb_region_t r;

	r.x = 0;
	r.y = 0;
	r.w = src->geom.width;
	r.h = src->geom.height;

	g_blit_region(dst, x, y, src, r);
}

void g_blit_region(fb_t *dst, int x, int y, fb_t *src, fb_region_t reg) {
	if (src->geom.memory_model == dst->geom.memory_model) {
		for (uint32_t i = 0; i < reg.h; i++) {
			memcpy(
				dst->data + (y + i) * dst->geom.pitch + x * (dst->geom.bpp / 8),
				src->data + (reg.y + i) * src->geom.pitch + reg.x * (src->geom.bpp / 8),
				reg.w * (src->geom.bpp / 8));
		}
	} else {
		dbg_printf("Unsupported blit between different memory models.\n");
	}
}

//  ---- Text manipulation

#define FONT_ASCII_BITMAP	1
// more font types to come

typedef struct font {
	int type;
	union {
		struct {
			uint8_t* data;
			uint8_t cw, ch;		// width, height of a character
			uint32_t nchars;
		} ascii_bitmap;
	};
} font_t;

font_t *g_load_ascii_bitmap_font(fd_t f) {
	font_t *font = 0;

	ascii_bitmap_font_header h;

	size_t s = read(f, 0, sizeof(h), (char*)&h);
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

	size_t rd = read(f, sizeof(h), h.ch * h.nchars, (char*)font->ascii_bitmap.data);
	if (rd != h.ch * h.nchars) goto error;

	return font;

error:
	if (font && font->ascii_bitmap.data) free(font->ascii_bitmap.data);
	if (font) free(font);
	close(f);
	return 0;
}

font_t *g_load_font(const char* fontname) {
	char buf[128];

	snprintf(buf, 128, "sys:/fonts/%s.bf", fontname);
	fd_t f = open(buf, FM_READ);
	if (f != 0) return g_load_ascii_bitmap_font(f);

	return 0;
}

void g_free_font(font_t *f) {
	if (f->type == FONT_ASCII_BITMAP) {
		free(f->ascii_bitmap.data);
	}

	free(f);
}

int g_text_width(font_t *font, const char* text) {
	if (font->type == FONT_ASCII_BITMAP) {
		return font->ascii_bitmap.cw * strlen(text);
	}
	return 0;
}

int g_text_height(font_t *font, const char* text) {
	if (font->type == FONT_ASCII_BITMAP) {
		return font->ascii_bitmap.ch;
	}
	return 0;
}

void g_write(fb_t *fb, int x, int y, const char* text, font_t *font, color_t c) {
	if (font->type == FONT_ASCII_BITMAP) {
		while (*text != 0) {
			uint8_t id = (uint8_t)*text;
			if (id < font->ascii_bitmap.nchars) {
				uint8_t *d = font->ascii_bitmap.data + (id * font->ascii_bitmap.ch);
				for (int r = 0; r < font->ascii_bitmap.ch; r++) {
					for (int j = 0; j < 8; j++) {
						if (d[r] & (0x80 >> j)) {
							g_plot(fb, x + j, y + r, c);
						}
					}
				}
			}
			text++;
			x += font->ascii_bitmap.cw;
		}
	}
}


/* vim: set ts=4 sw=4 tw=0 noet :*/
