#include <debug.h>

#include <fs/iso9660.h>

static bool iso9660_make(fs_handle_t *source, const char* opts, fs_t *t);

static fs_driver_ops_t iso9660_driver_ops = {
	.make = iso9660_make,
};

void register_iso9660_driver() {
	ASSERT(sizeof(iso9660_dr_date_t) == 7);
	ASSERT(sizeof(iso9660_dr_t) == 34);
	ASSERT(sizeof(iso9660_vdt_entry_t) == 2048);

	register_fs_driver("iso9660", &iso9660_driver_ops);
}

// ============================== //
// FILESYSTEM DETECTION AND SETUP //
// ============================== //

static bool iso9660_make(fs_handle_t *source, const char* opts, fs_t *t) {
	stat_t st;
	if (!file_stat(source, &st)) return false;
	if ((st.type & FT_BLOCKDEV) == 0) return false;

	int block_size = file_ioctl(source, IOCTL_BLOCKDEV_GET_BLOCK_SIZE, 0);
	if (block_size != 2048) return false;

	// Look up primary volume descriptor entry
	iso9660_vdt_entry_t ent;
	int ent_sect = 0x10;
	while (true) {
		if (file_read(source, ent_sect * 2048, 2048, ent.buf) != 2048) return false;
		if (ent.boot.type == ISO9660_VDT_PRIMARY) break; // ok got it!
		if (ent.boot.type == ISO9660_VDT_TERMINATOR) return false;	// doesn't exist ?
		ent_sect++;
	}

	return false;	// TODO
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
