#include <string.h>
#include <debug.h>

#include <fs/iso9660.h>

static bool iso9660_make(fs_handle_t *source, const char* opts, fs_t *t);
static void iso9660_fs_shutdown(fs_ptr f);

static bool iso9660_node_stat(fs_node_ptr n, stat_t *st);
static void iso9660_node_dispose(fs_node_ptr n);
static bool iso9660_dir_open(fs_node_ptr n, int mode, fs_handle_t *s);
static bool iso9660_dir_walk(fs_node_ptr n, const char* file, struct fs_node *node_d);
static bool iso9660_file_open(fs_node_ptr n, int mode, fs_handle_t *s);

static size_t iso9660_fh_read(fs_handle_ptr f, size_t offset, size_t len, char* buf);
static void iso9660_fh_close(fs_handle_ptr f);
static bool iso9660_dh_readdir(fs_handle_ptr f, dirent_t *d);
static void iso9660_dh_close(fs_handle_ptr f);

static fs_driver_ops_t iso9660_driver_ops = {
	.make = iso9660_make,
};

static fs_ops_t iso9660_fs_ops = {
	.add_source = 0,
	.shutdown = iso9660_fs_shutdown
};

static fs_node_ops_t iso9660_dir_ops = {
	.open = iso9660_dir_open,
	.stat = iso9660_node_stat,
	.walk = iso9660_dir_walk,
	.dispose = iso9660_node_dispose,
	.delete = 0,
	.move = 0,
	.create = 0,
};

static fs_node_ops_t iso9660_file_ops = {
	.open = iso9660_file_open,
	.stat = iso9660_node_stat,
	.dispose = iso9660_node_dispose,
	.walk = 0,
	.delete = 0,
	.move = 0,
	.create = 0,
};

static fs_handle_ops_t iso9660_dh_ops = {
	.readdir = iso9660_dh_readdir,
	.close = iso9660_dh_close,
	.read = 0,
	.write = 0,
	.ioctl = 0,
	.get_page = 0,
	.commit_page = 0,
};

static fs_handle_ops_t iso9660_fh_ops = {
	.readdir = 0,
	.close = iso9660_fh_close,
	.read = iso9660_fh_read,
	.write = 0,
	.ioctl = 0,
	.get_page = 0,
	.commit_page = 0,
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

bool iso9660_make(fs_handle_t *source, const char* opts, fs_t *t) {
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

	// Looks like we are good, go ahead.
	iso9660_fs_t *fs = (iso9660_fs_t*)malloc(sizeof(iso9660_fs_t));
	if (fs == 0) return false;

	iso9660_node_t *root = (iso9660_node_t*)malloc(sizeof(iso9660_node_t));
	if (root == 0) {
		free(fs);
		return false;
	}

	memcpy(&fs->vol_descr, &ent.prim, sizeof(iso9660_pvd_t));
	fs->disk = source;
	fs->use_lowercase = (strchr(opts, 'l') != 0);

	memcpy(&root->dr, &ent.prim.root_directory, sizeof(iso9660_dr_t));
	root->fs = fs;

	t->data = fs;
	t->ops = &iso9660_fs_ops;

	t->root->data = root;
	t->root->ops = &iso9660_dir_ops;

	return true;	// TODO
}

void iso9660_fs_shutdown(fs_ptr f) {
	iso9660_fs_t *fs = (iso9660_fs_t*)f;
	free(fs);
}

// ======================================= //
// READING STUFF THAT IS ON THE FILESYSTEM //
// ======================================= //

//  ---- Helper functions 

static void convert_dr_to_lowercase(iso9660_dr_t *dr) {
	for (int i = 0; i < dr->name_len; i++) {
		if (dr->name[i] >= 'A' && dr->name[i] <= 'Z')
			dr->name[i] += ('a' - 'A');
	}
}

static void dr_stat(iso9660_dr_t *dr, stat_t *st) {
	if (dr->flags & ISO9660_DR_FLAG_DIR) {
		st->type = FT_DIR;
		st->access = FM_READDIR;
		st->size = (dr->size.lsb / 256);
	} else {
		st->type = FT_REGULAR;
		st->access = FM_READ;
		st->size = dr->size.lsb;
	}
}

//  ---- The actual code

bool iso9660_node_stat(fs_node_ptr n, stat_t *st) {
	iso9660_node_t *node = (iso9660_node_t*)n;

	dr_stat(&node->dr, st);
	return true;
}

void iso9660_node_dispose(fs_node_ptr n) {
	iso9660_node_t *node = (iso9660_node_t*)n;
	free(node);
}

bool iso9660_dir_open(fs_node_ptr n, int mode, fs_handle_t *s) {
	iso9660_node_t *node = (iso9660_node_t*)n;

	if (mode != FM_READDIR) return false;

	iso9660_dh_t *handle = (iso9660_dh_t*)malloc(sizeof(iso9660_dh_t));
	if (handle == 0) return false;

	handle->n = node;
	handle->pos = 0;

	s->mode = mode;
	s->data = handle;
	s->ops = &iso9660_dh_ops;

	return true;
}

bool iso9660_dir_walk(fs_node_ptr n, const char* search_for, struct fs_node *node_d) {
	iso9660_node_t *node = (iso9660_node_t*)n;

	size_t filename_len = strlen(search_for);

	dbg_printf("Looking up %s...\n", search_for);

	char buffer[2048];
	size_t dr_len = 0;
	for (size_t pos = 0; pos < node->dr.size.lsb; pos += dr_len) {
		if (pos % 2048 == 0) {
			if (file_read(node->fs->disk, node->dr.lba_loc.lsb * 2048 + pos, 2048, buffer) != 2048)
				return false;
		}

		iso9660_dr_t *dr = (iso9660_dr_t*)(buffer + (pos % 2048));
		dr_len = dr->len;
		if (dr_len == 0) return false;

		if (node->fs->use_lowercase) convert_dr_to_lowercase(dr);

		char* name = dr->name;
		size_t len = dr->name_len - 2;

		if (name[len] != ';') continue;
		if (len != filename_len) continue;
		
		if (strncmp(name, search_for, len) == 0) {
			// Found it !
			iso9660_node_t *n = (iso9660_node_t*)malloc(sizeof(iso9660_node_t));
			if (n == 0) return false;

			memcpy(&n->dr, dr, sizeof(iso9660_dr_t));
			n->fs = node->fs;

			node_d->data = n;
			node_d->ops = (dr->flags & ISO9660_DR_FLAG_DIR ? &iso9660_dir_ops : &iso9660_file_ops);

			return true;
		}
	}
	return false;	// not found
}

bool iso9660_file_open(fs_node_ptr n, int mode, fs_handle_t *s) {
	iso9660_node_t *node = (iso9660_node_t*)n;

	if (mode != FM_READ) return false;

	s->data = node;
	s->ops = &iso9660_fh_ops;
	s->mode = mode;
	return true;
}

size_t iso9660_fh_read(fs_handle_ptr f, size_t offset, size_t len, char* buf) {
	iso9660_node_t *node = (iso9660_node_t*)f;

	if (offset >= node->dr.size.lsb) return 0;
	if (offset + len > node->dr.size.lsb) len =  node->dr.size.lsb - offset;

	size_t ret = 0;

	const size_t off0 = offset % 2048;
	const size_t first_block = offset - off0;

	char buffer[2048];

	for (size_t i = first_block; i < offset + len; i += 2048) {
		if (file_read(node->fs->disk, node->dr.lba_loc.lsb * 2048 + i, 2048, buffer) != 2048) {
			dbg_printf("Could not read data at %d\n", i);
			break;
		}

		size_t block_pos;
		size_t block_buf_ofs;
		if (i <= offset) {
			block_pos = 0;
			block_buf_ofs = off0;
		} else {
			block_pos = i - offset - off0;
			block_buf_ofs = 0;
		}
		size_t block_buf_end = (i + 2048 > offset + len ? offset + len - i : 2048);
		size_t block_len = block_buf_end - block_buf_ofs;

		memcpy(buf + block_pos, buffer + block_buf_ofs, block_len);
		ret += block_len;
	}

	return ret;
}

void iso9660_fh_close(fs_handle_ptr f) {
	// nothing to do
}

bool iso9660_dh_readdir(fs_handle_ptr f, dirent_t *d) {
	iso9660_dh_t *handle = (iso9660_dh_t*)f;
	iso9660_node_t *node = handle->n;

	while (true) {
		if (handle->pos >= node->dr.size.lsb) return false;

		if (handle->pos % 2048 == 0) {
			if (file_read(node->fs->disk, node->dr.lba_loc.lsb * 2048 + handle->pos, 2048, handle->buffer) != 2048)
				return false;
		}

		iso9660_dr_t *dr = (iso9660_dr_t*)(handle->buffer + (handle->pos % 2048));
		if (node->fs->use_lowercase) convert_dr_to_lowercase(dr);

		handle->pos += dr->len;

		size_t len = dr->name_len - 2;
		if (dr->name_len > 0 && dr->name[len] == ';') {
			if (len >= DIR_MAX) len = DIR_MAX - 1;
			memcpy(d->name, dr->name, len);
			d->name[len] = 0;
			dr_stat(dr, &d->st);

			return true;
		}
		// else skip this record
	}
}

void iso9660_dh_close(fs_handle_ptr f) {
	iso9660_dh_t* handle = (iso9660_dh_t*)f;
	free(handle);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
