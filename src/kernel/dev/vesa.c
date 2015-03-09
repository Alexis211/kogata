#include <string.h>

#include <framebuffer.h>		// common header

#include <vfs.h>
#include <nullfs.h>

#include <dev/vesa.h>
#include <dev/v86.h>

//  ---- kogata logo data (thank you gimp!)


/* GIMP RGB C-Source image dump 1-byte-run-length-encoded (kogata-logo-small.c) */

#define KOGATA_LOGO_RUN_LENGTH_DECODE(image_buf, rle_data, size, bpp) do \
{ unsigned int __bpp; unsigned char *__ip; const unsigned char *__il, *__rd; \
  __bpp = (bpp); __ip = (image_buf); __il = __ip + (size) * __bpp; \
  __rd = (rle_data); if (__bpp > 3) { /* RGBA */ \
    while (__ip < __il) { unsigned int __l = *(__rd++); \
      if (__l & 128) { __l = __l - 128; \
        do { memcpy (__ip, __rd, 4); __ip += 4; } while (--__l); __rd += 4; \
      } else { __l *= 4; memcpy (__ip, __rd, __l); \
               __ip += __l; __rd += __l; } } \
  } else { /* RGB */ \
    while (__ip < __il) { unsigned int __l = *(__rd++); \
      if (__l & 128) { __l = __l - 128; \
        do { memcpy (__ip, __rd, 3); __ip += 3; } while (--__l); __rd += 3; \
      } else { __l *= 3; memcpy (__ip, __rd, __l); \
               __ip += __l; __rd += __l; } } \
  } } while (0)
static const struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned int 	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  unsigned char	 rle_pixel_data[2940 + 1];
} kogata_logo = {
  128, 128, 3,
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\304\377\377\377\5\302\302\302\264"
  "\264\264\274\274\274\314\314\314\355\355\355\371\377\377\377\2\352\352\352"
  "\1\1\1\206\0\0\0\2+++\217\217\217\366\377\377\377\1\203\203\203\212\0\0\0"
  "\1yyy\364\377\377\377\1\236\236\236\213\0\0\0\1$$$\364\377\377\377\1\7\7"
  "\7\213\0\0\0\3\16\16\16\344\344\344\376\376\376\361\377\377\377\1\331\331"
  "\331\215\0\0\0\1\221\221\221\362\377\377\377\2\373\373\373---\214\0\0\0\1"
  "\210\210\210\363\377\377\377\1www\214\0\0\0\1\202\202\202\363\377\377\377"
  "\1|||\214\0\0\0\1\177\177\177\363\377\377\377\1GGG\214\0\0\0\1\274\274\274"
  "\362\377\377\377\1\354\354\354\214\0\0\0\2\7\7\7\300\300\300\362\377\377"
  "\377\1\23\23\23\215\0\0\0\1\330\330\330\361\377\377\377\1WWW\215\0\0\0\1"
  "\26\26\26\361\377\377\377\1\204\204\204\216\0\0\0\1\347\347\347\360\377\377"
  "\377\1\245\245\245\216\0\0\0\1\355\355\355\360\377\377\377\1\274\274\274"
  "\215\0\0\0\1\2\2\2\361\377\377\377\1\317\317\317\215\0\0\0\1""111\361\377"
  "\377\377\1\342\342\342\215\0\0\0\1JJJ\361\377\377\377\1\365\365\365\215\0"
  "\0\0\1]]]\362\377\377\377\215\0\0\0\1jjj\362\377\377\377\215\0\0\0\1uuu\362"
  "\377\377\377\215\0\0\0\1\177\177\177\362\377\377\377\215\0\0\0\1\212\212"
  "\212\362\377\377\377\1\4\4\4\214\0\0\0\1\225\225\225\362\377\377\377\1\13"
  "\13\13\214\0\0\0\1\240\240\240\362\377\377\377\1\21\21\21\214\0\0\0\1\252"
  "\252\252\362\377\377\377\1\26\26\26\214\0\0\0\1\260\260\260\362\377\377\377"
  "\1\33\33\33\214\0\0\0\1\265\265\265\362\377\377\377\1!!!\214\0\0\0\1\273"
  "\273\273\362\377\377\377\1&&&\214\0\0\0\1\300\300\300\362\377\377\377\1,"
  ",,\214\0\0\0\1\306\306\306\362\377\377\377\1///\214\0\0\0\1\314\314\314\362"
  "\377\377\377\1""222\214\0\0\0\1\321\321\321\362\377\377\377\1""555\214\0"
  "\0\0\1\327\327\327\362\377\377\377\1""777\214\0\0\0\1\334\334\334\362\377"
  "\377\377\1;;;\214\0\0\0\1\336\336\336\217\377\377\377\202\376\376\376\7p"
  "pp###\14\14\14\30\30\30@@@|||\307\307\307\332\377\377\377\1===\214\0\0\0"
  "\1\340\340\340\217\377\377\377\1\352\352\352\211\0\0\0\2GGG\301\301\301\327"
  "\377\377\377\1@@@\214\0\0\0\1\343\343\343\217\377\377\377\1qqq\213\0\0\0"
  "\3\6\6\6ttt\352\352\352\324\377\377\377\1CCC\214\0\0\0\1\344\344\344\217"
  "\377\377\377\1\254\254\254\216\0\0\0\2***\277\277\277\255\377\377\377\3\266"
  "\266\266fffvvv\242\377\377\377\1FFF\214\0\0\0\1\346\346\346\220\377\377\377"
  "\1###\217\0\0\0\2\34\34\34\313\313\313\250\377\377\377\3\375\375\375\363"
  "\363\363\37\37\37\203\0\0\0\1uuu\241\377\377\377\1FFF\214\0\0\0\1\350\350"
  "\350\221\377\377\377\1\11\11\11\220\0\0\0\1""888\247\377\377\377\2\363\363"
  "\363SSS\204\0\0\0\1\30\30\30\241\377\377\377\1EEE\214\0\0\0\1\352\352\352"
  "\220\377\377\377\3\376\376\376\377\377\377\27\27\27\220\0\0\0\2\7\7\7\316"
  "\316\316\245\377\377\377\1\26\26\26\206\0\0\0\215\377\377\377\1\335\335\335"
  "\202\273\273\273\1\372\372\372\220\377\377\377\1DDD\214\0\0\0\1\355\355\355"
  "\223\377\377\377\1\26\26\26\221\0\0\0\1ppp\243\377\377\377\1DDD\207\0\0\0"
  "\211\377\377\377\3\367\367\367\223\223\223333\204\0\0\0\1___\220\377\377"
  "\377\1DDD\214\0\0\0\1\356\356\356\223\377\377\377\2\373\373\373\17\17\17"
  "\221\0\0\0\1kkk\241\377\377\377\1\311\311\311\210\0\0\0\1\354\354\354\206"
  "\377\377\377\2\273\273\273...\206\0\0\0\1\36\36\36\221\377\377\377\1CCC\214"
  "\0\0\0\1\361\361\361\224\377\377\377\1\345\345\345\222\0\0\0\1uuu\240\377"
  "\377\377\1\22\22\22\210\0\0\0\1\244\244\244\203\377\377\377\3\376\376\376"
  "\226\226\226\20\20\20\207\0\0\0\1BBB\222\377\377\377\1BBB\214\0\0\0\1\361"
  "\361\361\224\377\377\377\2\376\376\376\214\214\214\222\0\0\0\1\330\330\330"
  "\236\377\377\377\1[[[\211\0\0\0\4\22\22\22\277\277\277\217\217\217---\211"
  "\0\0\0\1~~~\223\377\377\377\1AAA\214\0\0\0\1\362\362\362\226\377\377\377"
  "\1ttt\221\0\0\0\1\14\14\14\235\377\377\377\1\374\374\374\225\0\0\0\2\26\26"
  "\26\362\362\362\224\377\377\377\1@@@\214\0\0\0\1\363\363\363\227\377\377"
  "\377\1FFF\221\0\0\0\1BBB\234\377\377\377\1DDD\224\0\0\0\1\40\40\40\226\377"
  "\377\377\1;;;\214\0\0\0\1\363\363\363\230\377\377\377\1\36\36\36\221\0\0"
  "\0\1\315\315\315\232\377\377\377\1\355\355\355\224\0\0\0\1+++\227\377\377"
  "\377\1""555\214\0\0\0\1\364\364\364\230\377\377\377\1\373\373\373\221\0\0"
  "\0\1\34\34\34\232\377\377\377\1???\223\0\0\0\1---\230\377\377\377\1""000"
  "\214\0\0\0\1\365\365\365\231\377\377\377\1\206\206\206\221\0\0\0\1\261\261"
  "\261\230\377\377\377\1\364\364\364\223\0\0\0\1FFF\231\377\377\377\1***\214"
  "\0\0\0\1\365\365\365\232\377\377\377\1]]]\220\0\0\0\1\23\23\23\230\377\377"
  "\377\1TTT\222\0\0\0\1)))\232\377\377\377\1%%%\214\0\0\0\1\365\365\365\233"
  "\377\377\377\1'''\220\0\0\0\1\264\264\264\227\377\377\377\222\0\0\0\1\36"
  "\36\36\233\377\377\377\1\37\37\37\214\0\0\0\1\366\366\366\234\377\377\377"
  "\1\14\14\14\217\0\0\0\1&&&\226\377\377\377\1\231\231\231\221\0\0\0\1\24\24"
  "\24\234\377\377\377\1\27\27\27\214\0\0\0\1\366\366\366\234\377\377\377\1"
  "\326\326\326\220\0\0\0\1\346\346\346\225\377\377\377\1\24\24\24\221\0\0\0"
  "\1\370\370\370\234\377\377\377\1\11\11\11\214\0\0\0\1\367\367\367\235\377"
  "\377\377\1kkk\217\0\0\0\1\207\207\207\224\377\377\377\1\346\346\346\221\0"
  "\0\0\2\215\215\215\376\376\376\234\377\377\377\215\0\0\0\1\370\370\370\236"
  "\377\377\377\1""222\216\0\0\0\1>>>\224\377\377\377\1\207\207\207\220\0\0"
  "\0\1www\236\377\377\377\215\0\0\0\1\370\370\370\237\377\377\377\1\24\24\24"
  "\215\0\0\0\1\24\24\24\224\377\377\377\1:::\217\0\0\0\1NNN\237\377\377\377"
  "\215\0\0\0\1\370\370\370\237\377\377\377\1\355\355\355\215\0\0\0\1\23\23"
  "\23\224\377\377\377\1\16\16\16\216\0\0\0\1!!!\237\377\377\377\1\372\372\372"
  "\215\0\0\0\1\371\371\371\240\377\377\377\1\200\200\200\214\0\0\0\1CCC\224"
  "\377\377\377\1\25\25\25\216\0\0\0\1\361\361\361\237\377\377\377\1\346\346"
  "\346\215\0\0\0\1\372\372\372\241\377\377\377\1]]]\213\0\0\0\1\244\244\244"
  "\224\377\377\377\1""999\215\0\0\0\1BBB\240\377\377\377\1\320\320\320\215"
  "\0\0\0\1\372\372\372\242\377\377\377\1""111\211\0\0\0\1\22\22\22\225\377"
  "\377\377\1ppp\215\0\0\0\1\254\254\254\240\377\377\377\1\272\272\272\215\0"
  "\0\0\1\373\373\373\243\377\377\377\1&&&\210\0\0\0\1\325\325\325\225\377\377"
  "\377\1\275\275\275\215\0\0\0\1\261\261\261\240\377\377\377\1\244\244\244"
  "\215\0\0\0\1\373\373\373\244\377\377\377\1###\206\0\0\0\1kkk\227\377\377"
  "\377\215\0\0\0\1\217\217\217\240\377\377\377\1\211\211\211\215\0\0\0\1\374"
  "\374\374\245\377\377\377\1CCC\204\0\0\0\1^^^\230\377\377\377\1HHH\214\0\0"
  "\0\1sss\240\377\377\377\1ccc\215\0\0\0\1\374\374\374\246\377\377\377\4\335"
  "\335\335sssooo\330\330\330\231\377\377\377\1\324\324\324\214\0\0\0\1fff\216"
  "\377\377\377\3\335\335\335\312\312\312\356\356\356\217\377\377\377\1===\215"
  "\0\0\0\1\375\375\375\304\377\377\377\1\35\35\35\213\0\0\0\1\233\233\233\216"
  "\377\377\377\1""333\202\0\0\0\4\3\3\3QQQ\235\235\235\371\371\371\213\377"
  "\377\377\1\14\14\14\215\0\0\0\1\375\375\375\304\377\377\377\1\302\302\302"
  "\212\0\0\0\1\14\14\14\217\377\377\377\2\371\371\371\30\30\30\205\0\0\0\3"
  "$$$\223\223\223\361\361\361\207\377\377\377\1\367\367\367\216\0\0\0\1\376"
  "\376\376\305\377\377\377\1""000\211\0\0\0\1\334\334\334\221\377\377\377\1"
  "///\207\0\0\0\3\35\35\35\200\200\200\321\321\321\204\377\377\377\1\241\241"
  "\241\216\0\0\0\1\376\376\376\305\377\377\377\1\373\373\373\210\0\0\0\1\211"
  "\211\211\223\377\377\377\1bbb\212\0\0\0\4///sss\222\222\222\3\3\3\216\0\0"
  "\0\1\376\376\376\306\377\377\377\2\331\331\331\12\12\12\204\0\0\0\2""111"
  "\364\364\364\225\377\377\377\1\204\204\204\232\0\0\0\1\2\2\2\311\377\377"
  "\377\4\267\267\267\221\221\221\244\244\244\356\356\356\230\377\377\377\1"
  "\215\215\215\231\0\0\0\1ccc\346\377\377\377\1\223\223\223\230\0\0\0\1\312"
  "\312\312\346\377\377\377\2\376\376\376\221\221\221\226\0\0\0\1\11\11\11\351"
  "\377\377\377\1\210\210\210\225\0\0\0\1qqq\352\377\377\377\1rrr\224\0\0\0"
  "\1\334\334\334\353\377\377\377\1>>>\222\0\0\0\1\21\21\21\355\377\377\377"
  "\1\26\26\26\221\0\0\0\1~~~\355\377\377\377\1\325\325\325\221\0\0\0\1\355"
  "\355\355\356\377\377\377\1JJJ\217\0\0\0\1!!!\360\377\377\377\1\7\7\7\216"
  "\0\0\0\1\232\232\232\360\377\377\377\1\236\236\236\216\0\0\0\1\376\376\376"
  "\361\377\377\377\1\16\16\16\214\0\0\0\1""222\362\377\377\377\1\232\232\232"
  "\214\0\0\0\1\256\256\256\363\377\377\377\213\0\0\0\1\1\1\1\364\377\377\377"
  "\1SSS\212\0\0\0\1kkk\364\377\377\377\1\316\316\316\212\0\0\0\1\342\342\342"
  "\365\377\377\377\1\5\5\5\210\0\0\0\1(((\366\377\377\377\1bbb\210\0\0\0\1"
  "\277\277\277\366\377\377\377\1\270\270\270\207\0\0\0\1!!!\370\377\377\377"
  "\207\0\0\0\1\354\354\354\370\377\377\377\1CCC\205\0\0\0\1vvv\371\377\377"
  "\377\1\320\320\320\204\0\0\0\1VVV\373\377\377\377\1""666\202\0\0\0\1jjj\375"
  "\377\377\377\2\276\276\276\360\360\360\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\324\377\377\377",
};

//  ---- VESA data structures

typedef struct {
	char vbe_signature[4];				// == "VESA"
	uint16_t vbe_version;
	v86_farptr_t oem_string_ptr;
	uint8_t capabilities[4];
	v86_farptr_t video_mode_ptr;
	uint16_t total_memory;				// as # of 64KB blocks
} __attribute__((packed)) vbe_info_block_t;

typedef struct {
	uint16_t attributes;
	uint8_t winA, winB;
	uint16_t granularity;
	uint16_t winsize;
	uint16_t segmentA, segmentB;
	v86_farptr_t real_fct_ptr;
	uint16_t pitch; // bytes per scanline

	uint16_t Xres, Yres;
	uint8_t Wchar, Ychar, planes, bpp, banks;
	uint8_t memory_model, bank_size, image_pages;
	uint8_t reserved0;

	uint8_t red_mask, red_position;
	uint8_t green_mask, green_position;
	uint8_t blue_mask, blue_position;
	uint8_t rsv_mask, rsv_position;
	uint8_t directcolor_attributes;

	uint32_t physbase;		// the offset of the framebuffer in physical memory!
	uint32_t reserved1;
	uint16_t reserved2;
} __attribute__((packed)) vbe_mode_info_block_t;

//  ---- VESA driver data structures

typedef struct {
	framebuffer_info_t info;
	uint16_t vesa_mode_id;
	void* phys_fb_addr;
} vesa_mode_t;

typedef struct {
	vesa_mode_t *modes;

	int nmodes;
	int current_mode;

	pager_t *pager;
} vesa_driver_t;

//  ---- VESA code

bool vesa_open(fs_node_ptr n, int mode);
size_t vesa_read(fs_handle_t *f, size_t offset, size_t len, char* buf);
size_t vesa_write(fs_handle_t *f, size_t offset, size_t len, const char* buf);
void vesa_close(fs_handle_t *f);
int vesa_ioctl(fs_node_ptr n, int command, void* data);
bool vesa_stat(fs_node_ptr n, stat_t *st);

bool vesa_set_mode(vesa_driver_t *d, int n);

fs_node_ops_t vesa_fs_ops = {
	.open = vesa_open,
	.read = vesa_read,
	.write = vesa_write,
	.close = vesa_close,
	.ioctl = vesa_ioctl,
	.stat = vesa_stat,
	.readdir = 0,
	.poll = 0,
	.walk = 0,
	.delete = 0,
	.move = 0,
	.create = 0,
	.dispose = 0,
};

void vesa_detect(fs_t *iofs) {
	if (!v86_begin_session()) return;
	
	vbe_info_block_t *i = (vbe_info_block_t*)v86_alloc(512);
	memset(i, 0, 512);
	strncpy(i->vbe_signature, "VBE2", 4);

	v86_regs.ax = 0x4F00;
	v86_regs.es = V86_SEG_OF_LIN(i);
	v86_regs.di = V86_OFF_OF_LIN(i);

	vesa_mode_t *mode_data = 0;
	int mode_data_c = 0;

	if (!v86_bios_int(0x10) || v86_regs.ax != 0x004F) {
		dbg_printf("Could not call int 0x10 to detect VESA.\n");
		goto end_detect;
	}

	dbg_printf("Detected VESA (sig %s, ver 0x%x, oem %s) with %d kb ram.\n",
		i->vbe_signature, i->vbe_version, v86_lin_of_fp(i->oem_string_ptr), 64 * i->total_memory);

	vbe_mode_info_block_t *mi = (vbe_mode_info_block_t*)v86_alloc(256);

	uint16_t *modes = (uint16_t*)v86_lin_of_fp(i->video_mode_ptr);
	uint16_t *last_mode = modes;
	while (*last_mode != 0xFFFF) last_mode++;

	mode_data = (vesa_mode_t*)malloc((last_mode - modes) * sizeof(vesa_mode_t));
	if (mode_data == 0) goto end_detect;

	for (uint16_t *mode = modes; mode < last_mode; mode++) {
		v86_regs.ax = 0x4F01;
		v86_regs.cx = *mode;
		v86_regs.es = V86_SEG_OF_LIN(mi);
		v86_regs.di = V86_OFF_OF_LIN(mi);
		if (!v86_bios_int(0x10) || v86_regs.ax != 0x004F) continue;

		if ((mi->attributes & 0x90) != 0x90) continue;	// not linear framebuffer
		if (mi->memory_model != 4 && mi->memory_model != 6) continue;

		int x = mode_data_c;
		mode_data[x].info.width = mi->Xres;
		mode_data[x].info.height = mi->Yres;
		mode_data[x].info.bpp = mi->bpp;
		mode_data[x].info.pitch = mi->pitch;
		mode_data[x].phys_fb_addr = (void*)mi->physbase;
		mode_data[x].vesa_mode_id = *mode;
		mode_data_c++;
	}

end_detect:
	v86_end_session();

	if (mode_data == 0) return;

	vesa_driver_t *d = (vesa_driver_t*)malloc(sizeof(vesa_driver_t));
	if (d == 0) goto fail_setup;

	d->pager = new_device_pager(0, 0);
	if (d == 0) goto fail_setup;

	d->modes = mode_data;
	d->nmodes = mode_data_c;
	d->current_mode = -1;

	bool add_ok = nullfs_add_node(iofs, "/display/vesa", d, &vesa_fs_ops, d->pager);
	if (!add_ok) goto fail_setup;

	// Lookup a valid mode and set it
	for (int i = 0; i < mode_data_c; i++) {
		if (mode_data[i].info.bpp == 24 && mode_data[i].info.width <= 800 && mode_data[i].info.width >= 600) {
			if (vesa_set_mode(d, i)) break;
		}
	}

	dbg_printf("Successfully registered VESA driver (%d modes)\n", mode_data_c);
	return;	// success

fail_setup:
	if (d != 0 && d->pager != 0) delete_pager(d->pager);
	if (d) free(d);
	if (mode_data) free(mode_data);
}

bool vesa_open(fs_node_ptr n, int mode) {
	int ok_modes = FM_READ | FM_WRITE | FM_MMAP;
	if (mode & ~ok_modes) return false;

	return true;
}

size_t vesa_read(fs_handle_t *f, size_t offset, size_t len, char* buf) {
	vesa_driver_t *d = (vesa_driver_t*)f->node->data;

	return pager_read(d->pager, offset, len, buf);
}

size_t vesa_write(fs_handle_t *f, size_t offset, size_t len, const char* buf) {
	vesa_driver_t *d = (vesa_driver_t*)f->node->data;

	return pager_write(d->pager, offset, len, buf);
}

void vesa_close(fs_handle_t *f) {
	// nothing to do
}

int vesa_ioctl(fs_node_ptr n, int command, void* data) {
	// TODO
	return 0;
}

bool vesa_stat(fs_node_ptr n, stat_t *st) {
	vesa_driver_t *d = (vesa_driver_t*)d;
	
	framebuffer_info_t *i = (d->current_mode == -1 ? 0 : &d->modes[d->current_mode].info);

	st->type = FT_DEV | FT_FRAMEBUFFER;
	st->size = (i ? i->width * i->pitch : 0);
	st->access = FM_READ | FM_WRITE | FM_MMAP;

	return true;
}

bool vesa_set_mode(vesa_driver_t *d, int n) {
	ASSERT(n >= 0 && n < d->nmodes);

	if (!v86_begin_session()) return false;

	bool ok = true;

	v86_regs.ax = 0x4F02;
	v86_regs.bx = d->modes[n].vesa_mode_id;
	if (!v86_bios_int(0x10) || v86_regs.ax != 0x004F) ok = false;

	v86_end_session();

	if (ok) {
		size_t fb_size = d->modes[n].info.pitch * d->modes[n].info.width;

		d->current_mode = n;
		change_device_pager(d->pager, fb_size, d->modes[n].phys_fb_addr);

		// clear screen & put kogata logo
		void* region = region_alloc(fb_size, "VESA");
		if (region) {
			bool ok = true;
			for (void* x = region; x < region + fb_size; x += PAGE_SIZE) {
				void* paddr = d->modes[n].phys_fb_addr + (x - region);
				if (!pd_map_page(x, (uint32_t)paddr / PAGE_SIZE, true)) ok = false;
			}
			if (ok) {
				memset(region, 255, fb_size);

				void* buffer = malloc(kogata_logo.width * kogata_logo.height * kogata_logo.bytes_per_pixel);
				if (buffer) {
					KOGATA_LOGO_RUN_LENGTH_DECODE(buffer,
						kogata_logo.rle_pixel_data,
						kogata_logo.width * kogata_logo.height,
						kogata_logo.bytes_per_pixel);

					int tly = (d->modes[n].info.height / 2) - (kogata_logo.height / 2);
					int tlx = (d->modes[n].info.width / 2) - (kogata_logo.width / 2);
					int mbpp = (d->modes[n].info.bpp / 8);

					for (unsigned l = 0; l < kogata_logo.height; l++) {
						memcpy(region + (tly + l) * d->modes[n].info.pitch + tlx * mbpp,
							buffer + kogata_logo.width * l * kogata_logo.bytes_per_pixel,
							kogata_logo.width * kogata_logo.bytes_per_pixel);
					}

					free(buffer);
				}
			}
			for (void* x = region; x < region + fb_size; x += PAGE_SIZE) {
				pd_unmap_page(x);
			}
			region_free(region);
		}
	}

	return ok;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
