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

		n->ops->dispose(n->data);
		unref_fs_node(n->parent);
		unref_fs(n->fs);
		free(n);
	}
}

fs_node_t* fs_walk_one(fs_node_t* from, const char* file) {
	fs_node_t *n = (fs_node_t*)malloc(sizeof(fs_node_t));
	if (n == 0) return 0;

	n->fs = from->fs;
	n->refs = 1;
	n->parent = from;

	if (from->ops->walk && from->ops->walk(from->data, file, n)) {
		ref_fs_node(n->parent);
		ref_fs(n->fs);
		return n;
	} else {
		free(n);
		return 0;
	}
}

fs_node_t* fs_walk_path(fs_node_t* from, const char* path) {
	fs_node_t *n = from;
	ref_fs_node(n);

	while (n && (*path)) {
		const char* d = strchr(path, '/');
		if (d == path) {
			path++;
		} else if (d == 0) {
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
	// TODO : parse path, walk
	// This function does NOT walk into the last component of the path (it may not even exist),
	// instead it isolates it in the buffer last_file
	// This buffer is expected to be of size DIR_MAX (defined in fs.h)
	return 0;
}

// ========================== //
// DOING THINGS IN FLESYSTEMS //

bool fs_create(fs_t *fs, const char* file, int type) {
	char name[DIR_MAX];
	fs_node_t *n = fs_walk_path_except_last(&fs->root, file, name);
	if (n == 0) return false;

	bool ret = n->ops->create && n->ops->create(n->data, name, type);
	unref_fs_node(n);
	return ret;
}

bool fs_delete(fs_t *fs, const char* file) {
	fs_node_t* n = fs_walk_path(&fs->root, file);
	if (n == 0) return false;

	bool ret = n->ops->delete && n->ops->delete(n->data);
	unref_fs_node(n);
	return ret;
}

bool fs_move(fs_t *fs, const char* from, const char* to) {
	fs_node_t *n = fs_walk_path(&fs->root, from);
	if (n == 0) return false;

	char new_name[DIR_MAX];
	fs_node_t *new_parent = fs_walk_path_except_last(&fs->root, to, new_name);
	if (new_parent == 0) return false;

	return n->ops->move && n->ops->move(n->data, new_parent, new_name);
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
	h->n = n;

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
		unref_fs_node(file->n);
		unref_fs(file->fs);
		free(file);
	}
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

int file_get_mode(fs_handle_t *f) {
	return f->mode;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
