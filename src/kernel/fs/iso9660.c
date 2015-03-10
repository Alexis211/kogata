#include <string.h>
#include <debug.h>

#include <fs/iso9660.h>

bool iso9660_make(fs_handle_t *source, const char* opts, fs_t *t);
void iso9660_fs_shutdown(fs_ptr f);

bool iso9660_node_stat(fs_node_t *n, stat_t *st);
void iso9660_node_dispose(fs_node_t *n);
void iso9660_node_close(fs_handle_t *h);

bool iso9660_dir_open(fs_node_t *n, int mode);
bool iso9660_dir_walk(fs_node_t* n, const char* file, struct fs_node *node_d);
bool iso9660_dir_readdir(fs_handle_t *h, size_t ent_no, dirent_t *d);

bool iso9660_file_open(fs_node_t *n, int mode);

size_t iso9660_node_read(fs_node_t *n, size_t offset, size_t len, char* buf);

fs_driver_ops_t iso9660_driver_ops = {
	.make = iso9660_make,
};

fs_ops_t iso9660_fs_ops = {
	.add_source = 0,
	.shutdown = iso9660_fs_shutdown
};

fs_node_ops_t iso9660_dir_ops = {
	.open = iso9660_dir_open,
	.stat = iso9660_node_stat,
	.walk = iso9660_dir_walk,
	.dispose = iso9660_node_dispose,
	.delete = 0,
	.move = 0,
	.create = 0,
	.readdir = iso9660_dir_readdir,
	.close = iso9660_node_close,
	.read = 0,
	.write = 0,
	.ioctl = 0,
	.poll = 0,
};

fs_node_ops_t iso9660_file_ops = {
	.open = iso9660_file_open,
	.stat = iso9660_node_stat,
	.dispose = iso9660_node_dispose,
	.walk = 0,
	.delete = 0,
	.move = 0,
	.create = 0,
	.readdir = 0,
	.close = iso9660_node_close,
	.read = fs_read_from_pager,
	.write = 0,
	.ioctl = 0,
	.poll = 0,
};

vfs_pager_ops_t iso9660_pager_ops = {
	.read = iso9660_node_read,
	.write = 0,
	.resize = 0,
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
	if (root == 0) goto error;

	memcpy(&fs->vol_descr, &ent.prim, sizeof(iso9660_pvd_t));
	fs->disk = source;
	fs->use_lowercase = (strchr(opts, 'l') != 0);

	memcpy(&root->dr, &ent.prim.root_directory, sizeof(iso9660_dr_t));
	root->fs = fs;

	t->data = fs;
	t->ops = &iso9660_fs_ops;

	t->root->data = root;
	t->root->ops = &iso9660_dir_ops;
	t->root->pager = new_vfs_pager(root->dr.size.lsb, t->root, &iso9660_pager_ops);

	return true;

error:
	if (root) free(root);
	if (fs) free(fs);
	return false;
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
		st->size = dr->size.lsb;		// TODO: precise item count ?
	} else {
		st->type = FT_REGULAR;
		st->access = FM_READ | FM_MMAP;
		st->size = dr->size.lsb;
	}
}

//  ---- The actual code

bool iso9660_node_stat(fs_node_t *n, stat_t *st) {
	iso9660_node_t *node = (iso9660_node_t*)n->data;

	dr_stat(&node->dr, st);
	return true;
}

void iso9660_node_dispose(fs_node_t *n) {
	ASSERT(n->pager != 0);
	delete_pager(n->pager);

	iso9660_node_t *node = (iso9660_node_t*)n->data;
	free(node);
}

void iso9660_node_close(fs_handle_t *h) {
	// nothing to do
}

bool iso9660_dir_open(fs_node_t *n, int mode) {
	if (mode != FM_READDIR) return false;

	return true;
}

bool iso9660_dir_walk(fs_node_t *n, const char* search_for, struct fs_node *node_d) {
	iso9660_node_t *node = (iso9660_node_t*)n->data;

	size_t filename_len = strlen(search_for);

	size_t dr_len = 0;
	for (size_t pos = 0; pos < node->dr.size.lsb; pos += dr_len) {
		union {
			iso9660_dr_t dr;
			char buf[256];
		} data;

		size_t rs = pager_read(n->pager, pos, 256, data.buf);
		if (rs < sizeof(iso9660_dr_t)) return false;

		iso9660_dr_t *dr = &data.dr;

		dr_len = dr->len;
		if (dr_len == 0) return false;

		if (node->fs->use_lowercase) convert_dr_to_lowercase(dr);

		char* name = dr->name;
		size_t len = dr->name_len;
		if (!(dr->flags & ISO9660_DR_FLAG_DIR)) len -= 2;

		if (!(dr->flags & ISO9660_DR_FLAG_DIR) && name[len] != ';') continue;
		if (name[len-1] == '.') len--;	// file with no extension

		if (len != filename_len) continue;
		if (strncmp(name, search_for, len) == 0) {
			// Found it !
			iso9660_node_t *n = (iso9660_node_t*)malloc(sizeof(iso9660_node_t));
			if (n == 0) return false;

			memcpy(&n->dr, dr, sizeof(iso9660_dr_t));
			n->fs = node->fs;

			node_d->data = n;
			if (dr->flags & ISO9660_DR_FLAG_DIR) {
				node_d->ops = &iso9660_dir_ops;
			} else {
				node_d->ops = &iso9660_file_ops;
			}

			node_d->pager = new_vfs_pager(dr->size.lsb, node_d, &iso9660_pager_ops);
			if (node_d->pager == 0) {
				free(n);
				return false;
			}

			return true;
		}
	}
	return false;	// not found
}

bool iso9660_file_open(fs_node_t *n, int mode) {
	int ok_modes = FM_READ | FM_MMAP;
	
	if (mode & ~ok_modes) return false;

	return true;
}

size_t iso9660_node_read(fs_node_t *n, size_t offset, size_t len, char* buf) {
	iso9660_node_t *node = (iso9660_node_t*)n->data;

	if (offset >= node->dr.size.lsb) return 0;
	if (offset + len > node->dr.size.lsb) len = node->dr.size.lsb - offset;

	ASSERT(offset % 2048 == 0);

	size_t ret = 0;

	for (size_t i = offset; i < offset + len; i += 2048) {
		size_t stor_pos = node->dr.lba_loc.lsb * 2048 + i;

		size_t blk_len = 2048;
		if (i + blk_len > offset + len) blk_len = offset + len - i;

		if (blk_len == 2048) {
			if (file_read(node->fs->disk, stor_pos, 2048, buf + i - offset) != 2048) break;
			ret += 2048;
		} else {
			char *block_buf = (void*)malloc(2048);
			if (block_buf == 0) break;

			size_t read = file_read(node->fs->disk, stor_pos, 2048, block_buf);
			
			if (read == 2048) {
				memcpy(buf + i - offset, block_buf, blk_len);
				ret += blk_len;
			}

			free(block_buf);
		}
	}

	return ret;
}

void iso9660_file_close(fs_node_ptr f) {
	// nothing to do
}

bool iso9660_dir_readdir(fs_handle_t *h, size_t ent_no, dirent_t *d) {
	iso9660_node_t *node = (iso9660_node_t*)h->node->data;

	size_t dr_len = 0;
	size_t idx = 0;
	for (size_t pos = 0; pos < node->dr.size.lsb; pos += dr_len) {
		union {
			iso9660_dr_t dr;
			char buf[256];
		} data;

		size_t rs = pager_read(h->node->pager, pos, 256, data.buf);
		if (rs < sizeof(iso9660_dr_t)) return false;

		iso9660_dr_t *dr = &data.dr;

		dr_len = dr->len;
		if (dr_len == 0) return false;

		if (node->fs->use_lowercase) convert_dr_to_lowercase(dr);

		char* name = dr->name;
		size_t len = dr->name_len;
		if (!(dr->flags & ISO9660_DR_FLAG_DIR)) len -= 2;

		if (!(dr->flags & ISO9660_DR_FLAG_DIR) && name[len] != ';') continue;
		if (name[len-1] == '.') len--;	// file with no extension
		
		if (idx == ent_no) {
			// Found the node we are interested in
			if (len >= DIR_MAX) len = DIR_MAX - 1;
			memcpy(d->name, name, len);
			d->name[len] = 0;

			dr_stat(dr, &d->st);

			return true;
		} else {
			idx++;
		}
	}
	return false;	// not found
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
