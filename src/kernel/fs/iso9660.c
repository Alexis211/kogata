#include <debug.h>

#include <fs/iso9660.h>

static bool iso9660_make(fs_handle_t *source, const char* opts, fs_t *t);
static bool iso9660_detect(fs_handle_t *source);

static fs_driver_ops_t iso9660_driver_ops = {
	.make = iso9660_make,
	.detect = iso9660_detect,
};

void register_iso9660_driver() {
	ASSERT(sizeof(iso9660_vdt_entry_t) == 2048);
	ASSERT(sizeof(iso9660_dr_t) == 34);

	register_fs_driver("iso9660", &iso9660_driver_ops);
}

// ============================== //
// FILESYSTEM DETECTION AND SETUP //
// ============================== //

static bool iso9660_detect(fs_handle_t *source) {
	stat_t st;
	if (!file_stat(source, &st)) return false;
	if ((st.type & FT_BLOCKDEV) != 0) return false;

	uint32_t block_size = file_ioctl(source, IOCTL_BLOCKDEV_GET_BLOCK_SIZE, 0);
	if (block_size != 2048) return false;

	return false;	// TODO
}

static bool iso9660_make(fs_handle_t *source, const char* opts, fs_t *t) {
	return false; // TODO
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
