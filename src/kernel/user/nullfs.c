#include <hashtbl.h>
#include <mutex.h>
#include <string.h>
#include <debug.h>

#include <nullfs.h>

// nullfs driver
static bool nullfs_fs_make(fs_handle_t *source, char* opts, fs_t *d);

// nullfs fs_t
static void nullfs_fs_shutdown(fs_ptr fs);

// nullfs directory node
static bool nullfs_d_open(fs_node_ptr n, int mode, fs_handle_t *s);
static bool nullfs_d_stat(fs_node_ptr n, stat_t *st);
static bool nullfs_d_walk(fs_node_ptr n, const char* file, struct fs_node *node_d);
static bool nullfs_d_delete(fs_node_ptr n, const char* file);
static bool nullfs_d_move(fs_node_ptr n, const char* old_name, fs_node_t *new_parent, const char *new_name);
static bool nullfs_d_create(fs_node_ptr n, const char* file, int type);
static void nullfs_d_dispose(fs_node_ptr n);

// nullfs directory handle
static bool nullfs_dh_readdir(fs_handle_ptr f, dirent_t *d);
static void nullfs_dh_close(fs_handle_ptr f);

// nullfs ram file node
static bool nullfs_f_open(fs_node_ptr n, int mode, fs_handle_t *s);
static bool nullfs_f_stat(fs_node_ptr n, stat_t *st);
static void nullfs_f_dispose(fs_node_ptr n);

// nullfs ram file handle
static size_t nullfs_fh_read(fs_handle_ptr f, size_t offset, size_t len, char* buf);
static size_t nullfs_fh_write(fs_handle_ptr f, size_t offset, size_t len, const char* buf);
static void nullfs_fh_close(fs_handle_ptr f);

// VTables that go with it
static fs_driver_ops_t nullfs_driver_ops = {
	.make = nullfs_fs_make,
	.detect = 0,
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
	.ioctl = 0,
};

static fs_handle_ops_t nullfs_dh_ops = {
	.readdir = nullfs_dh_readdir,
	.close = nullfs_dh_close,
	.read = 0,
	.write = 0,
};

static fs_node_ops_t nullfs_f_ops = {
	.open = nullfs_f_open,
	.stat = nullfs_f_stat,
	.dispose = nullfs_f_dispose,
	.walk = 0,
	.create = 0,
	.delete = 0,
	.move = 0,
	.ioctl =0,
};

static fs_handle_ops_t nullfs_fh_ops = {
	.read = nullfs_fh_read,
	.write = nullfs_fh_write,
	.close = nullfs_fh_close,
	.readdir = 0,
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

	struct nullfs_item *next;
} nullfs_item_t;

typedef struct {
	nullfs_item_t *items_list;
	hashtbl_t *items_idx;

	mutex_t lock;		// always locked when open (cannot create/delete/move)

	nullfs_t *fs;
} nullfs_dir_t;

typedef struct {
	nullfs_item_t *it;
	nullfs_dir_t *d;
} nullfs_dh_t;

typedef struct {
	char* data;
	size_t size;
	bool own_data;
	int ok_modes;
	
	mutex_t lock;
} nullfs_file_t;

// No nullfs_file_handle_t struct, we don't need it. The handle's data
// pointer is simply a pointer to the file node.

// ===================== //
// NULLFS IMPLEMENTATION //
// ===================== //

void register_nullfs_driver() {
	register_fs_driver("nullfs", &nullfs_driver_ops);
}

bool nullfs_fs_make(fs_handle_t *source, char* opts, fs_t *fs_s) {
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
	root->items_idx = create_hashtbl(str_key_eq_fun, str_hash_fun, 0, 0);
	if (root->items_idx == 0) {
		free(root);
		free(fs);
		return false;
	}

	fs_s->ops = &nullfs_ops;
	fs_s->data = fs;
	fs_s->root.ops = &nullfs_d_ops;
	fs_s->root.data = root;

	return true;
}

void nullfs_fs_shutdown(fs_ptr fs) {
	// TODO free all
}

bool nullfs_add_node(fs_t *fs, const char* name, fs_node_ptr data, fs_node_ops_t *ops) {
	char file_name[DIR_MAX];

	fs_node_t *n = fs_walk_path_except_last(&fs->root, name, file_name);
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

bool nullfs_add_ram_file(fs_t *fs, const char* name, char* data, size_t init_sz, bool copy, int ok_modes) {
	nullfs_file_t *f = (nullfs_file_t*)malloc(sizeof(nullfs_file_t));
	if (f == 0) return false;

	f->size = init_sz;
	if (copy) {
		f->data = malloc(init_sz);
		memcpy(f->data, data, init_sz);
		f->own_data = true;
	} else {
		f->data = data;
		f->own_data = false;
	}
	f->ok_modes = ok_modes;
	f->lock = MUTEX_UNLOCKED;

	bool add_ok = nullfs_add_node(fs, name, f, &nullfs_f_ops);
	if (!add_ok) {
		if (f->own_data) free(f->data);
		free(f);
		return false;
	}

	return true;
}

//   -- Directory node --

bool nullfs_d_open(fs_node_ptr n, int mode, fs_handle_t *s) {
	if (mode != FM_READDIR) return false;

	nullfs_dir_t* d = (nullfs_dir_t*)n;

	bool got_lock = mutex_try_lock(&d->lock);
	if (!got_lock) return false;

	nullfs_dh_t *h = (nullfs_dh_t*)malloc(sizeof(nullfs_dh_t));
	if (h == 0) {
		mutex_unlock(&d->lock);
		return false;
	}

	h->it = d->items_list;
	h->d = d;

	s->data = h;
	s->ops = &nullfs_dh_ops;
	s->mode = FM_READDIR;

	return true;
}

bool nullfs_d_stat(fs_node_ptr n, stat_t *st) {
	nullfs_dir_t* d = (nullfs_dir_t*)n;

	mutex_lock(&d->lock);

	st->type = FT_DIR;
	st->access = FM_READDIR
		| (d->fs->can_create ? FM_DCREATE : 0)
		| (d->fs->can_move ? FM_DMOVE : 0)
		| (d->fs->can_delete ? FM_DUNLINK : 0);

	st->size = 0;
	for (nullfs_item_t *i = d->items_list; i != 0; i = i->next)
		st->size++;

	mutex_unlock(&d->lock);

	return true;
}

bool nullfs_d_walk(fs_node_ptr n, const char* file, struct fs_node *node_d) {
	nullfs_dir_t* d = (nullfs_dir_t*)n;

	mutex_lock(&d->lock);

	nullfs_item_t* x = (nullfs_item_t*)hashtbl_find(d->items_idx, file);
	if (x == 0) {
		mutex_unlock(&d->lock);
		return false;
	}

	node_d->ops = x->ops;
	node_d->data = x->data;

	mutex_unlock(&d->lock);

	return true;
}

bool nullfs_d_delete(fs_node_ptr n, const char* file) {
	return false; //TODO
}

bool nullfs_d_move(fs_node_ptr n, const char* old_name, fs_node_t *new_parent, const char *new_name) {
	return false; //TODO
}

bool nullfs_d_create(fs_node_ptr n, const char* file, int type) {
	return false; //TODO
}

void nullfs_d_dispose(fs_node_ptr n) {
	//TODO
}


//   -- Directory handle --

bool nullfs_dh_readdir(fs_handle_ptr f, dirent_t *d) {
	nullfs_dh_t *h = (nullfs_dh_t*)f;

	if (h->it == 0) {
		return false;
	} else {
		strncpy(d->name, h->it->name, DIR_MAX);
		d->name[DIR_MAX-1] = 0;	// make sur it's null-terminated
		if (h->it->ops->stat) {
			h->it->ops->stat(h->it->data, &d->st);
		} else {
			// no stat operation : should we do something else ?
			memset(&d->st, 0, sizeof(stat_t));
		}
		h->it = h->it->next;

		return true;
	}
}

void nullfs_dh_close(fs_handle_ptr f) {
	nullfs_dh_t *h = (nullfs_dh_t*)f;

	mutex_unlock(&h->d->lock);

	free(h);
}

//   -- File node --

bool nullfs_f_open(fs_node_ptr n, int mode, fs_handle_t *s) {
	nullfs_file_t *f = (nullfs_file_t*)n;

	if (mode & ~f->ok_modes) return false;

	mutex_lock(&f->lock);

	s->mode = mode;
	s->data = f;
	s->ops = &nullfs_fh_ops;

	return true;
}

bool nullfs_f_stat(fs_node_ptr n, stat_t *st) {
	return false; //TODO
}

void nullfs_f_dispose(fs_node_ptr n) {
	// TODO
}

//   -- File handle --

static size_t nullfs_fh_read(fs_handle_ptr f, size_t offset, size_t len, char* buf) {
	return 0; //TODO
}

static size_t nullfs_fh_write(fs_handle_ptr f, size_t offset, size_t len, const char* buf) {
	return 0; //TODO
}

static void nullfs_fh_close(fs_handle_ptr h) {
	nullfs_file_t *f = (nullfs_file_t*)h;

	mutex_unlock(&f->lock);
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
