#include <string.h>

#include <dev/vesa.h>
#include <dev/v86.h>

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

//  ---- VESA code

void vesa_detect(fs_t *iofs) {
	if (!v86_begin_session()) return;
	
	vbe_info_block_t *i = (vbe_info_block_t*)v86_alloc(512);
	memset(i, 0, 512);
	strncpy(i->vbe_signature, "VBE2", 4);

	v86_regs.ax = 0x4F00;
	v86_regs.es = V86_SEG_OF_LIN(i);
	v86_regs.di = V86_OFF_OF_LIN(i);

	if (v86_bios_int(0x10)) {
		if (v86_regs.ax == 0x004F) {
			dbg_printf("Detected VESA (sig %s, ver 0x%x, oem %s) with %d kb ram.\n",
				i->vbe_signature, i->vbe_version, v86_lin_of_fp(i->oem_string_ptr), 64 * i->total_memory);

			vbe_mode_info_block_t *mi = (vbe_mode_info_block_t*)v86_alloc(256);

			uint16_t *modes = (uint16_t*)v86_lin_of_fp(i->video_mode_ptr);
			for (int n = 0; modes[n] != 0xFFFF; n++) {
				v86_regs.ax = 0x4F01;
				v86_regs.cx = modes[n];
				v86_regs.es = V86_SEG_OF_LIN(mi);
				v86_regs.di = V86_OFF_OF_LIN(mi);
				if (v86_bios_int(0x10) && v86_regs.ax == 0x004F) {
					dbg_printf("Mode 0x%x : %dx%dx%d at 0x%p\n", modes[n], mi->Xres, mi->Yres, mi->bpp, mi->physbase);
				} else {
					dbg_printf("Mode 0x%x : could not get info\n", modes[n]);
				}
			}
		} else {
			dbg_printf("Error in BIOS int 0x10/0x4F00.\n");
		}
	} else {
		dbg_printf("Could not call BIOS int 0x10.\n");
	}

	v86_end_session();
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
