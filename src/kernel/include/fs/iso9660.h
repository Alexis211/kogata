#pragma once

#include <vfs.h>

#define ISO9660_VDT_BOOT	0
#define ISO9660_VDT_PRIMARY 1
#define ISO9660_VDT_SUPPLEMENTARY 2
#define ISO9660_VDT_PARTITION 3
#define ISO9660_VDT_TERMINATOR 255

#define ISO9660_DR_FLAG_DIR (1 << 1)

typedef struct {
	uint32_t lsb, msb;
} uint32_lsb_msb_t;

typedef struct {
	uint16_t lsb, msb;
} uint16_lsb_msb_t;

typedef struct {
	uint8_t years_since_1990;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t gmt_offset;
} iso9660_dr_date_t;

typedef struct {		// Directory record
	uint8_t len;		// length of the record
	uint8_t ear_len;	// length of extended attribute record
	uint32_lsb_msb_t lba_loc;	// location of extent
	uint32_lsb_msb_t size;		// size of extent
	iso9660_dr_date_t date;
	uint8_t flags;
	uint8_t interleave_unit_size;
	uint8_t interleave_gap_size;
	uint16_lsb_msb_t vol_seq_num;
	uint8_t name_len;
	char name[1];		// we want sizeof(iso9660_dr_t) == 34
} __attribute__((packed)) iso9660_dr_t;

typedef struct {		// Boot record
	uint8_t type;
	char ident[5];
	uint8_t version;	// always 1
	char sys_ident[32];
	char boot_ident[32];
	char reserved[1977];
} __attribute__((packed)) iso9660_bootrecord_t;

typedef struct {		// Primary volume descriptor
	uint8_t type;
	char ident[5];
	uint8_t version;	// always 1

	uint8_t unused0;

	char sys_ident[32];
	char vol_ident[32];

	uint8_t unused1[8];

	uint32_lsb_msb_t volume_size;
	uint8_t unused2[32];

	uint16_lsb_msb_t volume_set_size, volume_seq_number;
	uint16_lsb_msb_t block_size;
	uint32_lsb_msb_t path_table_size;
	uint32_t lpt_loc, olpt_loc, mpt_loc, ompt_loc;	// location of path tables

	iso9660_dr_t root_directory;

	// more fields (not interesting...)
} __attribute__((packed)) iso9660_pvd_t;

typedef struct {
	uint8_t type;
	char ident[5];
	uint8_t version;	// always 1
} __attribute__((packed)) iso9660_vdt_terminator_t;		// volume descriptor table terminator

typedef union {
	iso9660_bootrecord_t boot;
	iso9660_pvd_t prim;
	iso9660_vdt_terminator_t term;
	char buf[2048];
} iso9660_vdt_entry_t;

typedef struct {
	iso9660_pvd_t vol_descr;
	fs_handle_t *disk;
	bool use_lowercase;	// lowercase all names
} iso9660_fs_t;

typedef struct {
	iso9660_dr_t dr;
	iso9660_fs_t *fs;
} iso9660_node_t;

typedef struct {
	iso9660_node_t *n;
	size_t pos;
	char buffer[2048];
} iso9660_dh_t;		// handle to an open directory

void register_iso9660_driver();

/* vim: set ts=4 sw=4 tw=0 noet :*/
