#include <debug.h>
#include <vfs.h>
#include <string.h>

// ============================= //
// FILESYSTEM DRIVER REGISTERING //
// ============================= //

typedef struct fs_driver {
	const char* name;
	fs_driver_ops_t *ops;
	struct fs_driver *next;
} fs_driver_t;

fs_driver_t *drivers = 0;

void register_fs_driver(const char* name, fs_driver_ops_t *ops) {
	fs_driver_t *d = (fs_driver_t*)malloc(sizeof(fs_driver_t));
	ASSERT(d != 0); // should we fail in a more graceful manner ? TODO

	d->name = name;
	d->ops = ops;
	d->next = drivers;
	drivers = d;
}

// ================================== //
// CREATING AND DELETING FILE SYSTEMS //
// ================================== //

fs_t *make_fs(const char* drv_name, fs_handle_t *source, char* opts) {
	// Look for driver
	fs_driver_t *d = 0;
	for(fs_driver_t *i = drivers; i != 0; i = i->next) {
		if (drv_name != 0 && strcmp(i->name, drv_name) == 0) d = i;
		if (drv_name == 0 && source != 0 && i->ops->detect && i->ops->detect(source)) d = i;
		if (d != 0) break;
	}
	if (d == 0) return 0;		// driver not found

	// Open file system
	fs_t *fs = (fs_t*)malloc(sizeof(fs_t));
	if (fs == 0) return 0;

	fs->refs = 1;
	fs->root.refs = 1;		// root node is never disposed of (done by fs->shutdown)
	fs->root.fs = fs;
	fs->root.parent = 0;
	fs->root.children = 0;

	if (d->ops->make(source, opts, fs)) {
		return fs;
	} else {
		free(fs);
		return 0;
	}
}

bool fs_add_source(fs_t *fs, fs_handle_t *source) {
	return fs->ops->add_source && fs->ops->add_source(fs->data, source);
}

void ref_fs(fs_t *fs) {
	fs->refs++;
}

void unref_fs(fs_t *fs) {
	fs->refs--;
	if (fs->refs == 0) {
		// don't unref root node, don't call dispose on it
		// (done by fs->shutdown)
		fs->ops->shutdown(fs->data);
		free(fs);
	}
}

// ====================================================== //
// WALKING IN THE FILE SYSTEM CREATING AND DELETING NODES //

void ref_fs_node(fs_node_t *n) {
	n->refs++;
}

void unref_fs_node(fs_node_t *n) {
	n->refs--;
	if (n->refs == 0) {
		ASSERT(n != &n->fs->root);
		ASSERT(n->parent != 0);
		ASSERT(n->name != 0);

		hashtbl_remove(n->parent->children, n->name);

		if (n->ops->dispose) n->ops->dispose(n->data);

		unref_fs_node(n->parent);
		unref_fs(n->fs);

		free(n->name);
		free(n);
	}
}

fs_node_t* fs_walk_one(fs_node_t* from, const char* file) {
	if (from->children != 0) {
		fs_node_t *n = (fs_node_t*)hashtbl_find(from->children, file);
		if (n != 0) {
			ref_fs_node(n);
			return n;
		}
	}

	bool walk_ok = false, add_ok = false;

	fs_node_t *n = (fs_node_t*)malloc(sizeof(fs_node_t));
	if (n == 0) return 0;

	n->fs = from->fs;
	n->refs = 1;
	n->parent = from;
	n->children = 0;
	n->name = strdup(file);
	if (n->name == 0) goto error;
	
	walk_ok = from->ops->walk && from->ops->walk(from->data, file, n);
	if (!walk_ok) goto error;

	if (from->children == 0) {
		from->children = create_hashtbl(str_key_eq_fun, str_hash_fun, 0, 0);
		if (from->children == 0) goto error;
	}

	add_ok = hashtbl_add(from->children, n->name, n);
	if (!add_ok) goto error;

	ref_fs_node(n->parent);
	ref_fs(n->fs);

	return n;

error:
	if (walk_ok) n->ops->dispose(n->data);
	if (n->name != 0) free(n->name);
	free(n);
	return 0;
}

fs_node_t* fs_walk_path(fs_node_t* from, const char* path) {
	if (*path == '/') path++;

	fs_node_t *n = from;
	ref_fs_node(n);

	while (n && (*path)) {
		if (*path == '/') {
			// we don't want multiple slashes, so fail
			unref_fs_node(n);
			return 0;
		}

		const char* d = strchr(path, '/');
		if (d == 0) {
			fs_node_t *n2 = fs_walk_one(n, path);
			unref_fs_node(n);

			return n2;
		} else {
			size_t nlen = d - path;
			if (nlen >= DIR_MAX - 1) {
				unref_fs_node(n);
				return 0;	// sorry, path item too long
			}
			char name_buf[DIR_MAX];
			strncpy(name_buf, path, nlen);
			name_buf[nlen] = 0;

			fs_node_t *n2 = fs_walk_one(n, name_buf);
			unref_fs_node(n);
			n = n2;

			path = d + 1;
		}
	}
	return n;
}

fs_node_t* fs_walk_path_except_last(fs_node_t* from, const char* path, char* last_file_buf) {
	// This function does NOT walk into the last component of the path (it may not even exist),
	// instead it isolates it in the buffer last_file
	// This buffer is expected to be of size DIR_MAX (defined in fs.h)
	if (*path == '/') path++;
	if (*path == 0) return 0;		// no last component !

	fs_node_t *n = from;
	ref_fs_node(n);

	while (n && (*path)) {
		if (*path == '/') {
			// we don't want multiple slashes, so fail
			unref_fs_node(n);
			return 0;
		}

		const char* d = strchr(path, '/');
		if (d == 0) {
			if (strlen(path) >= DIR_MAX - 1) {
				unref_fs_node(n);
				return 0;	// sorry, path item too long
			}
			strcpy(last_file_buf, path);
			return n;
		} else {
			size_t nlen = d - path;
			if (nlen >= DIR_MAX - 1) {
				unref_fs_node(n);
				return 0;	// sorry, path item too long
			}
			strncpy(last_file_buf, path, nlen);
			last_file_buf[nlen] = 0;

			if (*(d+1) == 0) {
				// trailing slash !
				return n;
			} else {
				fs_node_t *n2 = fs_walk_one(n, last_file_buf);
				unref_fs_node(n);
				n = n2;

				path = d + 1;
			}
		}
	}
	return n;
}

// =========================== //
// DOING THINGS IN FILESYSTEMS //

bool fs_create(fs_t *fs, const char* file, int type) {
	char name[DIR_MAX];
	fs_node_t *n = fs_walk_path_except_last(&fs->root, file, name);
	if (n == 0) return false;

	bool ret = n->ops->create && n->ops->create(n->data, name, type);
	unref_fs_node(n);
	return ret;
}

bool fs_unlink(fs_t *fs, const char* file) {
	char name[DIR_MAX];

	fs_node_t* n = fs_walk_path_except_last(&fs->root, file, name);
	if (n == 0) return false;

	if (n->children != 0) {
		fs_node_t* x = (fs_node_t*)hashtbl_find(n->children, name);
		if (x != 0) return false;
	}

	bool ret = n->ops->unlink && n->ops->unlink(n->data, name);
	unref_fs_node(n);
	return ret;
}

bool fs_move(fs_t *fs, const char* from, const char* to) {
	char old_name[DIR_MAX];
	fs_node_t *old_parent = fs_walk_path_except_last(&fs->root, from, old_name);
	if (old_parent == 0) return false;

	char new_name[DIR_MAX];
	fs_node_t *new_parent = fs_walk_path_except_last(&fs->root, to, new_name);
	if (new_parent == 0) {
		unref_fs_node(old_parent);
		return false;
	}

	bool ret = old_parent->ops->move && old_parent->ops->move(old_parent->data, old_name, new_parent, new_name);

	unref_fs_node(old_parent);
	unref_fs_node(new_parent);
	return ret;
}

bool fs_stat(fs_t *fs, const char* file, stat_t *st) {
	fs_node_t* n = fs_walk_path(&fs->root, file);
	if (n == 0) return false;

	bool ret = n->ops->stat && n->ops->stat(n->data, st);
	unref_fs_node(n);
	return ret;
}

int fs_ioctl(fs_t *fs, const char* file, int command, void* data) {
	fs_node_t* n = fs_walk_path(&fs->root, file);
	if (n == 0) return false;

	int ret = (n->ops->ioctl ? n->ops->ioctl(n->data, command, data) : -1);
	unref_fs_node(n);
	return ret;
}

// =================== //
// OPERATIONS ON FILES //
// =================== //

fs_handle_t* fs_open(fs_t *fs, const char* file, int mode) {
	fs_node_t *n = fs_walk_path(&fs->root, file);
	if (n == 0) return false;

	fs_handle_t *h = (fs_handle_t*)malloc(sizeof(fs_handle_t));
	if (h == 0) {
		unref_fs_node(n);
		return 0;
	}

	h->refs = 1;
	h->fs = fs;
	h->node = n;

	if (n->ops->open(n->data, mode, h)) {
		// our reference to node n is transferred to the file handle
		ref_fs(fs);
		return h;
	} else {
		unref_fs_node(n);
		free(h);
		return 0;
	}
}

void ref_file(fs_handle_t *file) {
	file->refs++;
}

void unref_file(fs_handle_t *file) {
	file->refs--;
	if (file->refs == 0) {
		file->ops->close(file->data);
		unref_fs_node(file->node);
		unref_fs(file->fs);
		free(file);
	}
}

int file_get_mode(fs_handle_t *f) {
	return f->mode;
}

size_t file_read(fs_handle_t *f, size_t offset, size_t len, char* buf) {
	if (!(f->mode && FM_READ)) return 0;

	if (f->ops->read == 0) return 0;
	return f->ops->read(f->data, offset, len, buf);
}

size_t file_write(fs_handle_t *f, size_t offset, size_t len, const char* buf) {
	if (!(f->mode && FM_WRITE)) return 0;

	if (f->ops->write == 0) return 0;
	return f->ops->write(f->data, offset, len, buf);
}

bool file_stat(fs_handle_t *f, stat_t *st) {
	return f->node->ops->stat && f->node->ops->stat(f->node->data, st);
}

bool file_readdir(fs_handle_t *f, dirent_t *d) {
	return f->ops->readdir && f->ops->readdir(f->data, d);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
