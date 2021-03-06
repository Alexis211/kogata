#include <hashtbl.h>
#include <mutex.h>
#include <string.h>
#include <debug.h>

#include <nullfs.h>
#include <pager.h>

// nullfs driver
bool nullfs_fs_make(fs_handle_t *source, const char* opts, fs_t *d);

// nullfs fs_t
void nullfs_fs_shutdown(fs_ptr fs);

// nullfs directory node
bool nullfs_d_open(fs_node_t *n, int mode);
bool nullfs_d_stat(fs_node_t *n, stat_t *st);
bool nullfs_d_walk(fs_node_t *n, const char* file, struct fs_node *node_d);
bool nullfs_d_delete(fs_node_t *n, const char* file);
bool nullfs_d_move(fs_node_t *n, const char* old_name, fs_node_t *new_parent, const char *new_name);
bool nullfs_d_create(fs_node_t *n, const char* file, int type);
void nullfs_d_dispose(fs_node_t *n);
bool nullfs_d_readdir(fs_handle_t *f, size_t ent_no, dirent_t *d);
void nullfs_d_close(fs_handle_t *f);

// nullfs ram file node
bool nullfs_f_open(fs_node_t *n, int mode);
bool nullfs_f_stat(fs_node_t *n, stat_t *st);
void nullfs_f_dispose(fs_node_t *n);
size_t nullfs_f_read(fs_handle_t *f, size_t offset, size_t len, char* buf);
size_t nullfs_f_write(fs_handle_t *f, size_t offset, size_t len, const char* buf);
void nullfs_f_close(fs_handle_t *f);

// VTables that go with it
static fs_driver_ops_t nullfs_driver_ops = {
	.make = nullfs_fs_make,
};

static fs_ops_t nullfs_ops = {
	.shutdown = nullfs_fs_shutdown,
	.add_source = 0,
};

static fs_node_ops_t nullfs_d_ops = {
	.open = nullfs_d_open,
	.stat = nullfs_d_stat,
	.walk = nullfs_d_walk,
	.delete = nullfs_d_delete,
	.move = nullfs_d_move,
	.create = nullfs_d_create,
	.dispose = nullfs_d_dispose,
	.readdir = nullfs_d_readdir,
	.close = nullfs_d_close,
	.read = 0,
	.write = 0,
	.ioctl = 0,
	.poll = 0,
};

static fs_node_ops_t nullfs_f_ops = {
	.open = nullfs_f_open,
	.stat = nullfs_f_stat,
	.dispose = nullfs_f_dispose,
	.walk = 0,
	.create = 0,
	.delete = 0,
	.move = 0,
	.read = fs_read_from_pager,
	.write = fs_write_to_pager,
	.close = nullfs_f_close,
	.readdir = 0,
	.ioctl =0,
	.poll = 0,
};


// ====================== //
// NULLFS DATA STRUCTURES //
// ====================== //

typedef struct {
	bool can_create, can_move, can_delete;
} nullfs_t;

typedef struct nullfs_item {
	char* name;
	fs_node_ptr data;
	fs_node_ops_t *ops;
	pager_t *pager;

	struct nullfs_item *next;
} nullfs_item_t;

typedef struct {
	nullfs_item_t *items_list;
	hashtbl_t *items_idx;

	mutex_t lock;

	nullfs_t *fs;

	fs_node_t *vfs_node;
} nullfs_dir_t;

typedef struct {
	pager_t *pager;

	int ok_modes;
	
	mutex_t lock;		// locked on open

	fs_node_t *vfs_node;
} nullfs_file_t;

// No nullfs_file_handle_t struct, we don't need it. The handle's data
// pointer is simply a pointer to the file node.

// ===================== //
// NULLFS IMPLEMENTATION //
// ===================== //

void register_nullfs_driver() {
	register_fs_driver("nullfs", &nullfs_driver_ops);
}

bool nullfs_fs_make(fs_handle_t *source, const char* opts, fs_t *fs_s) {
	if (source != 0) return false;

	nullfs_t *fs = (nullfs_t*)malloc(sizeof(nullfs_t));
	if (fs == 0) return false;

	fs->can_create = (strchr(opts, 'c') != 0);
	fs->can_move = (strchr(opts, 'm') != 0);
	fs->can_delete = (strchr(opts, 'd') != 0);

	nullfs_dir_t *root = (nullfs_dir_t*)malloc(sizeof(nullfs_dir_t));
	if (root == 0) {
		free(fs);
		return false;
	}

	root->fs = fs;
	root->items_list = 0;
	root->lock = MUTEX_UNLOCKED;
	root->items_idx = create_hashtbl(str_key_eq_fun, str_hash_fun, 0);
	if (root->items_idx == 0) {
		free(root);
		free(fs);
		return false;
	}

	fs_s->ops = &nullfs_ops;
	fs_s->data = fs;
	fs_s->root->ops = &nullfs_d_ops;
	fs_s->root->data = root;

	return true;
}

void nullfs_fs_shutdown(fs_ptr fs) {
	dbg_printf("Not implemented: nullfs_fs_shutdown. Memory is leaking.\n");
	// TODO free all
}

bool nullfs_add_node(fs_t *fs, const char* name, fs_node_ptr data, fs_node_ops_t *ops, pager_t *pager) {
	char file_name[DIR_MAX];

	fs_node_t *n = fs_walk_path_except_last(fs->root, name, file_name);
	if (n == 0) return false;
	if (n->ops != &nullfs_d_ops) return false;

	nullfs_dir_t *d = (nullfs_dir_t*)n->data;
	mutex_lock(&d->lock);

	nullfs_item_t *i = (nullfs_item_t*)malloc(sizeof(nullfs_item_t));
	if (i == 0) goto error;
	
	i->name = strdup(file_name);
	if (i->name == 0) goto error;

	i->data = data;
	i->ops = ops;
	i->pager = pager;

	bool add_ok = hashtbl_add(d->items_idx, i->name, i);
	if (!add_ok) goto error;

	i->next = d->items_list;
	d->items_list = i;

	mutex_unlock(&d->lock);
	return true;

error:
	if (i && i->name) free(i->name);
	if (i) free(i);
	mutex_unlock(&d->lock);
	return false;
}

bool nullfs_add_ram_file(fs_t *fs, const char* name, const char* data, size_t init_sz, int ok_modes) {
	pager_t *p = 0;

	nullfs_file_t *f = (nullfs_file_t*)malloc(sizeof(nullfs_file_t));
	if (f == 0) return false;
	
	p = new_swap_pager(init_sz, true);
	if (p == 0) goto error;

	f->ok_modes = ok_modes & (FM_MMAP | FM_TRUNC | FM_READ | FM_WRITE);
	f->lock = MUTEX_UNLOCKED;
	f->pager = p;

	bool add_ok = nullfs_add_node(fs, name, f, &nullfs_f_ops, p);
	if (!add_ok) goto error;

	pager_write(p, 0, init_sz, data);

	return true;

error:
	if (p) delete_pager(p);
	if (f) free(f);
	return false;
}

//   -- Directory node --

bool nullfs_d_open(fs_node_t *n, int mode) {
	if (mode != FM_READDIR) return false;

	return true;
}

bool nullfs_d_stat(fs_node_t *n, stat_t *st) {
	nullfs_dir_t* d = (nullfs_dir_t*)n->data;

	mutex_lock(&d->lock);

	st->type = FT_DIR;
	st->access = FM_READDIR
		| (d->fs->can_create ? FM_DCREATE : 0)
		| (d->fs->can_move ? FM_DMOVE : 0)
		| (d->fs->can_delete ? FM_DDELETE : 0);

	st->size = hashtbl_count(d->items_idx);

	mutex_unlock(&d->lock);

	return true;
}

bool nullfs_d_walk(fs_node_t *n, const char* file, struct fs_node *node_d) {
	nullfs_dir_t* d = (nullfs_dir_t*)n->data;

	mutex_lock(&d->lock);

	nullfs_item_t* x = (nullfs_item_t*)hashtbl_find(d->items_idx, file);
	if (x == 0) {
		mutex_unlock(&d->lock);
		return false;
	}

	node_d->ops = x->ops;
	node_d->data = x->data;
	node_d->pager = x->pager;

	mutex_unlock(&d->lock);

	return true;
}

bool nullfs_d_delete(fs_node_t *n, const char* file) {
	nullfs_dir_t* d = (nullfs_dir_t*)n->data;

	mutex_lock(&d->lock);

	if (!d->fs->can_delete) goto error;

	nullfs_item_t *i = hashtbl_find(d->items_idx, file);
	if (i == 0) goto error;

	if (i->ops == &nullfs_d_ops) {
		// if it is a subdirectory, check it is empty
		nullfs_dir_t* sd = (nullfs_dir_t*)i->data;
		if (!mutex_try_lock(&sd->lock)) goto error;	// in use
		if (sd->items_list != 0) goto error;		// cannot delete non-empty directory

		delete_hashtbl(sd->items_idx);
		free(sd);
	} else if (i->ops == &nullfs_f_ops) {
		nullfs_file_t* f = (nullfs_file_t*)i->data;
		if (!mutex_try_lock(&f->lock)) goto error;	// in use

		delete_pager(f->pager);
		free(f);
	} else {
		goto error;		// special nodes (devices, ...) may not be deleted
	}

	hashtbl_remove(d->items_idx, i->name);

	if (d->items_list == i) {
		d->items_list = i->next;
	} else {
		for (nullfs_item_t* it = d->items_list; it != 0; it++) {
			if (it->next == i) {
				it->next = i->next;
				break;
			}
		}
	}

	free(i->name);
	free(i);
	mutex_unlock(&d->lock);
	return true;

error:
	mutex_unlock(&d->lock);
	return false;
}

bool nullfs_d_move(fs_node_t *n, const char* old_name, fs_node_t *new_parent, const char *new_name) {
	dbg_printf("Not implemented: move in nullfs. Failing potentially valid move request.\n");

	return false; //TODO
}

bool nullfs_d_create(fs_node_t *n, const char* file, int type) {
	nullfs_dir_t *d = (nullfs_dir_t*)n->data;
	nullfs_item_t *i = 0;

	if (type == FT_REGULAR) {
		mutex_lock(&d->lock);

		pager_t *p = 0;

		nullfs_file_t *f = (nullfs_file_t*)malloc(sizeof(nullfs_file_t));
		if (f == 0) goto f_error;

		p = new_swap_pager(0, true);
		if (p == 0) goto f_error;

		f->ok_modes = FM_READ | FM_WRITE | FM_TRUNC | FM_APPEND;
		f->pager = p;
		f->lock = MUTEX_UNLOCKED;

		i = (nullfs_item_t*)malloc(sizeof(nullfs_item_t));
		if (i == 0) goto f_error;

		i->name = strdup(file);
		if (i->name == 0) goto f_error;

		i->ops = &nullfs_f_ops;
		i->data = f;
		i->pager = p;

		bool add_ok = hashtbl_add(d->items_idx, i->name, i);
		if (!add_ok) goto f_error;

		i->next = d->items_list;
		d->items_list = i;

		mutex_unlock(&d->lock);
		return true;

	f_error:
		if (i != 0 && i->name != 0) free(i->name);
		if (i != 0) free(i);
		if (f != 0) free(f);
		if (p != 0) delete_pager(p);
		mutex_unlock(&d->lock);
		return false;
	} else if (type == FT_DIR) {
		mutex_lock(&d->lock);

		nullfs_dir_t *x = (nullfs_dir_t*)malloc(sizeof(nullfs_dir_t));
		if (x == 0) goto d_error;

		x->items_idx = create_hashtbl(str_key_eq_fun, str_hash_fun, 0);
		if (x->items_idx == 0) goto d_error;

		x->items_list = 0;
		x->lock = MUTEX_UNLOCKED;
		x->fs = d->fs;

		i = (nullfs_item_t*)malloc(sizeof(nullfs_item_t));
		if (i == 0) goto d_error;

		i->name = strdup(file);
		if (i->name == 0) goto d_error;

		i->ops = &nullfs_d_ops;
		i->data = x;

		bool add_ok = hashtbl_add(d->items_idx, i->name, i);
		if (!add_ok) goto d_error;

		i->next = d->items_list;
		d->items_list = i;

		mutex_unlock(&d->lock);
		return true;

	d_error:
		if (i != 0 && i->name != 0) free(i->name);
		if (i != 0) free(i);
		if (x != 0 && x->items_idx != 0) delete_hashtbl(x->items_idx);
		if (x != 0) free(x);
		mutex_unlock(&d->lock);
		return false;
	} else {
		return false;
	}
}

void nullfs_d_dispose(fs_node_t *n) {
	// nothing to do
}

bool nullfs_d_readdir(fs_handle_t *f, size_t ent_no, dirent_t *d) {
	// Very nonefficient !!

	nullfs_dir_t *h = (nullfs_dir_t*)f->node->data;

	mutex_lock(&h->lock);

	bool ok = false;
	size_t idx = 0;
	for (nullfs_item_t *i = h->items_list; i != 0; i = i->next) {
		if (idx == ent_no) {
			ok = true;

			strncpy(d->name, i->name, DIR_MAX);
			d->name[DIR_MAX-1] = 0;

			// dirty hack so that we can stat (TODO do this better)
			fs_node_t n;
			memset(&n, 0, sizeof(fs_node_t));
			n.fs = f->node->fs;
			n.ops = i->ops;
			n.data = i->data;
			n.pager = i->pager;
			n.ops->stat(&n, &d->st);

			break;
		}
		idx++;
	}

	mutex_unlock(&h->lock);

	return ok;
}

void nullfs_d_close(fs_handle_t *f) {
	// Nothing to do
}

//   -- File node --

bool nullfs_f_open(fs_node_t *n, int mode) {
	nullfs_file_t *f = (nullfs_file_t*)n->data;

	if (mode & ~f->ok_modes) return false;

	if (mode & FM_TRUNC) {
		// truncate file
		mutex_lock(&f->lock);

		pager_resize(f->pager, 0);

		mutex_unlock(&f->lock);
	}

	return true;
}

bool nullfs_f_stat(fs_node_t *n, stat_t *st) {
	nullfs_file_t *f = (nullfs_file_t*)n->data;
	mutex_lock(&f->lock);

	st->type = FT_REGULAR;
	st->access = f->ok_modes;
	st->size = f->pager->size;

	mutex_unlock(&f->lock);
	return true;
}

void nullfs_f_dispose(fs_node_t *n) {
	// nothing to do
}

//   -- File handle --

void nullfs_f_close(fs_handle_t *h) {
	// nothing to do
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
