#include <debug.h>
#include <vfs.h>
#include <string.h>

// ============================= //
// FILESYSTEM DRIVER REGISTERING //
// ============================= //

static fs_ops_t no_fs_ops = {
	.shutdown = 0,
	.add_source = 0,
};

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

fs_t *make_fs(const char* drv_name, fs_handle_t *source, const char* opts) {
	// Open file system
	fs_t *fs = (fs_t*)malloc(sizeof(fs_t));
	if (fs == 0) goto fail;

	fs->root = (fs_node_t*)malloc(sizeof(fs_node_t));
	if (fs->root == 0) goto fail;

	fs->refs = 1;
	fs->from_fs = 0;
	fs->ok_modes = FM_ALL_MODES;
	fs->root->refs = 1;
	fs->root->fs = fs;
	fs->root->parent = 0;
	fs->root->children = 0;

	// Look for driver
	for(fs_driver_t *i = drivers; i != 0; i = i->next) {
		if ((drv_name != 0 && strcmp(i->name, drv_name) == 0) || (drv_name == 0 && source != 0)) {
			if (i->ops->make(source, opts, fs)) {
				return fs;
			} else {
				goto fail;
			}
		}
	}


fail:
	if (fs && fs->root) free(fs->root);
	if (fs) free(fs);
	return 0;
}

fs_t *fs_subfs(fs_t *fs, const char* root, int ok_modes) {
	fs_node_t* new_root = fs_walk_path(fs->root, root);
	if (new_root == 0) return 0;

	fs_t *subfs = (fs_t*)malloc(sizeof(fs_t));
	if (subfs == 0) return 0;

	subfs->refs = 1;
	subfs->from_fs = fs;
	subfs->ok_modes = ok_modes & fs->ok_modes;

	subfs->ops = &no_fs_ops;
	subfs->data = 0;

	subfs->root = new_root;

	return subfs;
}

bool fs_add_source(fs_t *fs, fs_handle_t *source, const char* opts) {
	return fs->ops->add_source && fs->ops->add_source(fs->data, source, opts);
}

void ref_fs(fs_t *fs) {
	fs->refs++;
}

void unref_fs(fs_t *fs) {
	fs->refs--;
	if (fs->refs == 0) {
		if (fs->from_fs != 0) {
			unref_fs_node(fs->root);
		} else {
			ASSERT(fs->root->refs == 1);
			if (fs->root->ops->dispose) fs->root->ops->dispose(fs->root->data);
		}
		if (fs->ops->shutdown) fs->ops->shutdown(fs->data);
		free(fs);
	}
}

// ====================================================== //
// WALKING IN THE FILE SYSTEM CREATING AND DELETING NODES //

void ref_fs_node(fs_node_t *n) {
	mutex_lock(&n->lock);
	n->refs++;
	mutex_unlock(&n->lock);
}

void unref_fs_node(fs_node_t *n) {
	mutex_lock(&n->lock);
	n->refs--;
	if (n->refs == 0) {
		ASSERT (n != n->fs->root);

		ASSERT(n->parent != 0);
		ASSERT(n->name != 0);

		mutex_lock(&n->parent->lock);
		hashtbl_remove(n->parent->children, n->name);
		if (n->ops->dispose) n->ops->dispose(n->data);
		mutex_unlock(&n->parent->lock);

		unref_fs_node(n->parent);
		unref_fs(n->fs);

		if (n->children != 0) {
			ASSERT(hashtbl_count(n->children) == 0);
			delete_hashtbl(n->children);
		}
		free(n->name);
		free(n);
	} else {
		mutex_unlock(&n->lock);
	}
}

fs_node_t* fs_walk_one(fs_node_t* from, const char* file) {
	mutex_lock(&from->lock);

	if (from->children != 0) {
		fs_node_t *n = (fs_node_t*)hashtbl_find(from->children, file);
		if (n != 0) {
			ref_fs_node(n);
			mutex_unlock(&from->lock);
			return n;
		}
	}

	bool walk_ok = false, add_ok = false;

	fs_node_t *n = (fs_node_t*)malloc(sizeof(fs_node_t));
	if (n == 0) goto error;

	n->fs = from->fs;
	n->refs = 1;
	n->lock = MUTEX_UNLOCKED;
	n->parent = from;
	n->children = 0;
	n->name = strdup(file);
	if (n->name == 0) goto error;
	
	walk_ok = from->ops->walk && from->ops->walk(from->data, file, n);
	if (!walk_ok) goto error;

	if (from->children == 0) {
		from->children = create_hashtbl(str_key_eq_fun, str_hash_fun, 0);
		if (from->children == 0) goto error;
	}

	add_ok = hashtbl_add(from->children, n->name, n);
	if (!add_ok) goto error;

	mutex_unlock(&from->lock);

	ref_fs_node(n->parent);
	ref_fs(n->fs);

	return n;

error:
	if (walk_ok) n->ops->dispose(n->data);
	if (n != 0 && n->name != 0) free(n->name);
	if (n != 0) free(n);
	mutex_unlock(&from->lock);
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
	if (!(fs->ok_modes & FM_DCREATE)) return false;

	char name[DIR_MAX];
	fs_node_t *n = fs_walk_path_except_last(fs->root, file, name);
	if (n == 0) return false;

	mutex_lock(&n->lock);
	bool ret = n->ops->create && n->ops->create(n->data, name, type);
	mutex_unlock(&n->lock);

	unref_fs_node(n);
	return ret;
}

bool fs_delete(fs_t *fs, const char* file) {
	if (!(fs->ok_modes & FM_DDELETE)) return false;

	char name[DIR_MAX];

	fs_node_t* n = fs_walk_path_except_last(fs->root, file, name);
	if (n == 0) return false;

	if (n->children != 0) {
		fs_node_t* x = (fs_node_t*)hashtbl_find(n->children, name);
		if (x != 0) return false;
	}

	mutex_lock(&n->lock);
	bool ret = n->ops->delete && n->ops->delete(n->data, name);
	mutex_unlock(&n->lock);

	unref_fs_node(n);
	return ret;
}

bool fs_move(fs_t *fs, const char* from, const char* to) {
	if (!(fs->ok_modes & FM_DMOVE)) return false;

	char old_name[DIR_MAX];
	fs_node_t *old_parent = fs_walk_path_except_last(fs->root, from, old_name);
	if (old_parent == 0) return false;

	char new_name[DIR_MAX];
	fs_node_t *new_parent = fs_walk_path_except_last(fs->root, to, new_name);
	if (new_parent == 0) {
		unref_fs_node(old_parent);
		return false;
	}

	bool ret = false;
	if (!old_parent->ops->move) goto end;

	mutex_lock(&old_parent->lock);
	mutex_lock(&new_parent->lock);

	fs_node_t *the_node = (old_parent->children != 0 ?
		(fs_node_t*)hashtbl_find(old_parent->children, old_name) : 0);

	if (the_node) {
		mutex_lock(&the_node->lock);

		char* new_name_dup = strdup(new_name);
		if (new_name_dup == 0) goto unlock_end;	// we failed

		bool add_ok = hashtbl_add(new_parent->children, new_name_dup, the_node);
		if (!add_ok) {
			free(new_name_dup);
			goto unlock_end;
		}
			
		ret = old_parent->ops->move(old_parent->data, old_name, new_parent, new_name);

		if (ret) {
			// adjust node parameters
			hashtbl_remove(old_parent->children, old_name);
			free(the_node->name);
			the_node->name = new_name_dup;
			the_node->parent = new_parent;
		} else {
			hashtbl_remove(new_parent->children, new_name_dup);
			free(new_name_dup);
		}

	unlock_end:
		mutex_unlock(&the_node->lock);
	} else {
		ret = old_parent->ops->move(old_parent->data, old_name, new_parent, new_name);
	}

	mutex_unlock(&old_parent->lock);
	mutex_unlock(&new_parent->lock);

end:
	unref_fs_node(old_parent);
	unref_fs_node(new_parent);
	return ret;
}

bool fs_stat(fs_t *fs, const char* file, stat_t *st) {
	fs_node_t* n = fs_walk_path(fs->root, file);
	if (n == 0) return false;

	mutex_lock(&n->lock);
	bool ret = n->ops->stat && n->ops->stat(n->data, st);
	mutex_unlock(&n->lock);

	unref_fs_node(n);
	return ret;
}

// =================== //
// OPERATIONS ON FILES //
// =================== //

fs_handle_t* fs_open(fs_t *fs, const char* file, int mode) {
	if (mode & ~fs->ok_modes) return 0;

	fs_node_t *n = fs_walk_path(fs->root, file);
	if (n == 0 && (mode & FM_CREATE)) {
		if (fs_create(fs, file, FT_REGULAR)) {
			n = fs_walk_path(fs->root, file);
		}
	}
	if (n == 0) return 0;

	mutex_lock(&n->lock);

	mode &= ~FM_CREATE;

	fs_handle_t *h = (fs_handle_t*)malloc(sizeof(fs_handle_t));
	if (h == 0) goto error;

	bool open_ok = n->ops->open(n->data, mode);
	if (!open_ok) goto error;

	h->refs = 1;
	h->fs = fs;
	h->node = n;
	h->mode = mode;
	h->ops = n->ops;
	h->data = n->data;

	// our reference to node n is transferred to the file handle
	mutex_unlock(&n->lock);
	ref_fs(fs);
	return h;

error:
	mutex_unlock(&n->lock);
	unref_fs_node(n);
	if (h != 0) free(h);
	return 0;
}

void ref_file(fs_handle_t *file) {
	file->refs++;
}

void unref_file(fs_handle_t *file) {
	file->refs--;
	if (file->refs == 0) {
		file->ops->close(file);
		if (file->node) unref_fs_node(file->node);
		if (file->fs) unref_fs(file->fs);
		free(file);
	}
}

int file_get_mode(fs_handle_t *f) {
	return f->mode;
}

size_t file_read(fs_handle_t *f, size_t offset, size_t len, char* buf) {
	if (!(f->mode & FM_READ)) return 0;

	if (f->ops->read == 0) return 0;

	return f->ops->read(f, offset, len, buf);
}

size_t file_write(fs_handle_t *f, size_t offset, size_t len, const char* buf) {
	if (!(f->mode & FM_WRITE)) return 0;

	if (f->ops->write == 0) return 0;

	return f->ops->write(f, offset, len, buf);
}

bool file_stat(fs_handle_t *f, stat_t *st) {
	return f->ops->stat && f->ops->stat(f->data, st);
}

int file_ioctl(fs_handle_t *f, int command, void* data) {
	if (!(f->mode & FM_IOCTL)) return -1;

	if (f->ops->ioctl == 0) return -1;

	return f->ops->ioctl(f->data, command, data);
}

bool file_readdir(fs_handle_t *f, size_t ent_no, dirent_t *d) {
	if (!(f->mode & FM_READDIR)) return 0;

	return f->ops->readdir && f->ops->readdir(f, ent_no, d);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
