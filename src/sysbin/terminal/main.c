#include <string.h>
#include <malloc.h>
#include <user_region.h>
#include <debug.h>

#include <gip.h>
#include <draw.h>
#include <keyboard.h>

typedef char tchar_t;

typedef struct {
	fb_info_t mode;
	fd_t fd;
	fb_t *fb;

	uint32_t sv_features, cl_features;

	font_t *font;
	int cw, ch;	// size of a character
	color_t fg, bg;

	keyboard_t *kb;

	int w, h;
	tchar_t* scr_chars;
	int csrl, csrc;
	bool csr_visible;

	mainloop_fd_t app;
	char rd_c_buf;
	char wr_c_buf;
} term_t;

void c_buffer_info(gip_handler_t *s, gip_msg_header *p, gip_buffer_info_msg *m);
void c_key_down(gip_handler_t *s, gip_msg_header *p);
void c_key_up(gip_handler_t *s, gip_msg_header *p);
void c_unknown_msg(gip_handler_t *s, gip_msg_header *p);
void c_fd_error(gip_handler_t *s);

void term_on_rd_c(mainloop_fd_t *fd);
void term_app_on_error(mainloop_fd_t *fd);

void c_async_initiate(gip_handler_t *s, gip_msg_header *p, void* msgdata, void* hdata);

gip_handler_callbacks_t term_gip_cb = {
	.reset = 0,
	.initiate = 0,
	.ok = 0,
	.failure = 0,
	.enable_features = 0,
	.disable_features = 0,
	.query_mode = 0,
	.set_mode = 0,
	.buffer_info = c_buffer_info,
	.mode_info = 0,
	.buffer_damage = 0,
	.key_down = c_key_down,
	.key_up = c_key_up,
	.unknown_msg = c_unknown_msg,
	.fd_error = c_fd_error,
};


int main(int argc, char **argv) {
	dbg_print("[terminal] Starting up.\n");

	term_t term;
	memset(&term, 0, sizeof(term));

	term.kb = init_keyboard();
	ASSERT(term.kb != 0);

	term.font = g_load_font("default");
	ASSERT(term.font != 0);
	term.cw = g_text_width(term.font, "#");
	term.ch = g_text_height(term.font, "#");

	gip_handler_t *h = new_gip_handler(&term_gip_cb, &term);
	ASSERT(h != 0);

	h->mainloop_item.fd = 1;
	mainloop_add_fd(&h->mainloop_item);

	// setup communication with app
	term.app.fd = 2;
	term.app.on_error = term_app_on_error;
	term.app.data = &term;
	mainloop_expect(&term.app, &term.rd_c_buf, 1, term_on_rd_c);

	gip_msg_header reset_msg = { .code = GIPC_RESET, .arg = 0 };
	gip_cmd(h, &reset_msg, 0, c_async_initiate, 0);

	mainloop_run();

	return 0;
}

// Terminal features

void term_draw_c(term_t *t, int l, int c) {
	color_t bg = t->bg;
	if (l == t->csrl && c == t->csrc) bg = g_color_rgb(t->fb, 0, 100, 0);

	char ss[2];
	ss[0] = t->scr_chars[l * t->w + c];
	ss[1] = 0;

	if (t->fb) {
		g_fillrect(t->fb, c * t->cw, l * t->ch, t->cw, t->ch, bg);
		g_write(t->fb, c * t->cw, l * t->ch, ss, t->font, t->fg);
	}
}

void term_move_cursor(term_t *t, int l, int c) {
	int l0 = t->csrl, c0 = t->csrc;
	t->csrl = l;
	t->csrc = c;
	term_draw_c(t, l0, c0);
	term_draw_c(t, l, c);
}

void term_clear_screen(term_t *t) {
	t->csr_visible = true;

	for (int i = 0; i < t->w * t->h; i++) t->scr_chars[i] = ' ';

	if (t->fb)
		g_fillrect(t->fb, 0, 0, t->mode.width, t->mode.height, t->bg);

	term_move_cursor(t, 0, 0);
}

void term_scroll1(term_t *t) {
	for (int i = 0; i < t->w * (t->h - 1); i++)
		t->scr_chars[i] = t->scr_chars[i + t->w];
	for (int i = 0; i < t->w; i++) {
		t->scr_chars[i + (t->h - 1) * t->w] = ' ';
	}
	t->csrl--;

	if (t->fb) {
		g_scroll_up(t->fb, t->ch);
		g_fillrect(t->fb, 0, t->ch * (t->h - 1), t->cw * t->w, t->ch, t->bg);
	}
}

void term_putc(term_t *t, int c) {
	int nl = t->csrl, nc = t->csrc;
	if (c == '\n') {
		nl++; nc = 0;
	} else if (c == '\b') {
		if (nc > 0) {
			nc--;
		} else {
			nl--;
			nc = t->w - 1;
		}
		t->scr_chars[nl * t->w + nc] = ' ';
	} else if (c == '\t') {
		while (nc % 4 != 0) nc++;
	} else if (c >= ' ' && c < 128) {
		t->scr_chars[nl * t->w + nc] = c;
		nc++;
	}
	if (nc >= t->w) {
		nc = 0;
		nl++;
	}
	if (nl >= t->h) {
		term_scroll1(t);
		nl--;
	}
	term_move_cursor(t, nl, nc);
}

void term_puts(term_t *t, char* s) {
	while ((*s) != 0) {
		term_putc(t, *s);
		s++;
	}
}

// GIP client connection

void c_buffer_info(gip_handler_t *s, gip_msg_header *p, gip_buffer_info_msg *m) {
	dbg_printf("[terminal] Got buffer info msg\n");

	term_t *c = (term_t*)s->data;

	if (c->fb != 0) {
		g_delete_fb(c->fb);
		c->fb = 0;
	}
	if (c->fd != 0) {
		close(c->fd);
		c->fd = 0;
	}

	c->fd = use_token(&m->tok);
	if (c->fd != 0) {
		memcpy(&c->mode, &m->geom, sizeof(fb_info_t));

		dbg_printf("[terminal] Got buffer on FD %d, %dx%dx%d\n",
			c->fd, c->mode.width, c->mode.height, c->mode.bpp);

		if (c->scr_chars != 0) {
			free(c->scr_chars);
			c->scr_chars = 0;
		}
		c->w = c->mode.width / c->cw;
		c->h = c->mode.height / c->ch;
		c->scr_chars = (tchar_t*)malloc(c->w * c->h * sizeof(tchar_t));
		ASSERT(c->scr_chars != 0);

		c->fb = g_fb_from_file(c->fd, &m->geom);
		if (c->fb != 0) {
			c->bg = g_color_rgb(c->fb, 0, 0, 0);
			c->fg = g_color_rgb(c->fb, 200, 200, 200);
			term_clear_screen(c);

			mainloop_rm_fd(&c->app);
			mainloop_add_fd(&c->app);
		} else {
			dbg_printf("[terminal] Could not open framebuffer file %d\n", c->fd);
		}

	}
}

void c_key_down(gip_handler_t *s, gip_msg_header *p) {
	term_t *c = (term_t*)s->data;

	key_t k = keyboard_press(c->kb, p->arg);

	c->wr_c_buf = 0;
	if (k.flags & KBD_CHAR) {
		c->wr_c_buf = k.chr;
	} else {
		if (k.key == KBD_CODE_RETURN) c->wr_c_buf = '\n';
		if (k.key == KBD_CODE_TAB) c->wr_c_buf = '\t';
		if (k.key == KBD_CODE_BKSP) c->wr_c_buf = '\b';
	}
	mainloop_nonblocking_write(&c->app, &c->wr_c_buf, 1, false);
}

void c_key_up(gip_handler_t *s, gip_msg_header *p) {
	term_t *c = (term_t*)s->data;

	keyboard_release(c->kb, p->arg);
}

void c_unknown_msg(gip_handler_t *s, gip_msg_header *p) {
	// TODO
}

void c_fd_error(gip_handler_t *s) {
	// TODO
}

void term_on_rd_c(mainloop_fd_t *fd) {
	term_t *t = (term_t*)fd->data;

	term_putc(t, t->rd_c_buf);
}

void term_app_on_error(mainloop_fd_t *fd) {
	// TODO
}

void c_async_initiate(gip_handler_t *s, gip_msg_header *p, void* msgdata, void* hdata) {
	term_t *c = (term_t*)s->data;
	c->sv_features = p->arg;

	dbg_printf("[temrinal] Got initiate reply to reset request (features 0x%p).\n", c->sv_features);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
