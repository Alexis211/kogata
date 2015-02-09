#include <hashtbl.h>
#include <string.h>

#include <nullfs.h>

static bool nullfs_i_make(fs_handle_t *source, char* opts, fs_t *d);

static bool nullfs_i_open(void* fs, const char* file, int mode, fs_handle_t *s);
static bool nullfs_i_delete(void* fs, const char* file);
static void nullfs_i_shutdown(void* fs);

static size_t nullfs_i_read(void* f, size_t offset, size_t len, char* buf);
static size_t nullfs_i_write(void* f, size_t offset, size_t len, const char* buf);
static void nullfs_i_close(void* f);

static fs_driver_ops_t nullfs_driver_ops = {
	.make = nullfs_i_make,
	.detect = 0,
};

static fs_ops_t nullfs_ops = {
	.open = nullfs_i_open,
	.delete = nullfs_i_delete,
	.rename = 0,
	.stat = 0,
	.ioctl = 0,
	.add_source = 0,
	.shutdown = nullfs_i_shutdown
};

static fs_handle_ops_t nullfs_h_ops = {
	.read = nullfs_i_read,
	.write = nullfs_i_write,
	.close = nullfs_i_close
};

// Internal nullfs structures

typedef struct {
	void* data;
	nullfs_node_ops_t *ops;
} nullfs_item_t;

typedef struct {
	nullfs_item_t *item;
	void* data;
} nullfs_handle_t;

typedef struct nullfs {
	hashtbl_t *items;
	bool can_delete;
	bool can_create;
} nullfs_t;

// Nullfs management

void register_nullfs_driver() {
	register_fs_driver("nullfs", &nullfs_driver_ops);
}

nullfs_t *as_nullfs(fs_t *it) {
	if (it->ops != &nullfs_ops) return 0;
	return (nullfs_t*)it->data;
}

bool nullfs_i_make(fs_handle_t *source, char* opts, fs_t *d) {
	nullfs_t *fs = (nullfs_t*)malloc(sizeof(nullfs_t));
	if (fs == 0) return false;

	fs->items = create_hashtbl(str_key_eq_fun, str_hash_fun, free, 0);
	if (fs->items == 0) {
		free(fs);
		return false;
	}

	fs->can_delete = (strchr(opts, 'd') != 0);
	fs->can_create = (strchr(opts, 'c') != 0);

	d->data = fs;
	d->ops = &nullfs_ops;

	return true;
}

bool nullfs_add(nullfs_t *f, const char* name, void* data, nullfs_node_ops_t *ops) {
	nullfs_item_t *i = (nullfs_item_t*)malloc(sizeof(nullfs_item_t));
	if (i == 0) return false;

	char* n = strdup(name);
	if (n == 0) {
		free(i);
		return false;
	}

	i->data = data;
	i->ops = ops;
	if (!hashtbl_add(f->items, n, i)) {
		free(n);
		free(i);
		return false;
	}

	return true;
}

static void nullfs_i_free_item(void* x) {
	nullfs_item_t *i = (nullfs_item_t*)x;
	if (i->ops->dispose) i->ops->dispose(i->data);
	free(i);
}

void nullfs_i_shutdown(void* fs) {
	nullfs_t *f = (nullfs_t*)fs;

	delete_hashtbl(f->items, nullfs_i_free_item);
	free(f);
}

// Nullfs operations

bool nullfs_i_open(void* fs, const char* file, int mode, fs_handle_t *s) {
	nullfs_t *f = (nullfs_t*)fs;

	nullfs_item_t *x = (nullfs_item_t*)(hashtbl_find(f->items, file));
	if (x == 0) return false;
	// TODO : if null and can_create, then create.

	nullfs_handle_t *h = (nullfs_handle_t*)malloc(sizeof(nullfs_handle_t));
	if (h == 0) return false;

	h->item = x;
	h->data = x->ops->open(x->data, mode, s);
	if (h->data == 0) {
		free(h);
		return false;
	}

	s->data = h;
	s->ops = &nullfs_h_ops;
	return true;
}

bool nullfs_i_delete(void* fs, const char* file) {
	nullfs_t *f = (nullfs_t*)fs;

	if (!f->can_delete) return false;

	nullfs_item_t *x = (nullfs_item_t*)(hashtbl_find(f->items, file));
	if (x == 0) return false;

	hashtbl_remove(f->items, file);
	nullfs_i_free_item(x);
	return true;
}

size_t nullfs_i_read(void* f, size_t offset, size_t len, char* buf) {
	nullfs_handle_t *h = (nullfs_handle_t*)f;
	if (!h->item->ops->read) return 0;
	return h->item->ops->read(h->data, offset, len, buf);
}

size_t nullfs_i_write(void* f, size_t offset, size_t len, const char* buf) {
	nullfs_handle_t *h = (nullfs_handle_t*)f;
	if (!h->item->ops->write) return 0;
	return h->item->ops->write(h->data, offset, len, buf);
}

void nullfs_i_close(void* f) {
	nullfs_handle_t *h = (nullfs_handle_t*)f;
	if (h->item->ops->close) h->item->ops->close(h->data);
	free(h);
}

// ====================================================== //
// THE FUNCTIONS FOR HAVING RAM FILES (nullfs as ramdisk) //
// ====================================================== //

static void* nullfs_i_ram_open(void* f, int mode, fs_handle_t *h);
static size_t nullfs_i_ram_read(void* f, size_t offset, size_t len, char* buf);
static size_t nullfs_i_ram_write(void* f, size_t offset, size_t len, const char* buf);
static void nullfs_i_ram_dispose(void* f);

static nullfs_node_ops_t nullfs_ram_ops = {
	.open = nullfs_i_ram_open,
	.read = nullfs_i_ram_read,
	.write = nullfs_i_ram_write,
	.close = 0,
	.dispose = nullfs_i_ram_dispose
};

typedef struct {
	void* data;
	bool data_owned;
	size_t size;
	int ok_modes;
} nullfs_ram_file_t;

bool nullfs_add_ram_file(nullfs_t *f, const char* name, void* data, size_t init_sz, bool copy, int ok_modes) {
	nullfs_ram_file_t *x = (nullfs_ram_file_t*)malloc(sizeof(nullfs_ram_file_t));
	if (x == 0) return false;

	if (copy) {
		x->data = malloc(init_sz);
		if (x->data == 0) {
			free(x);
			return false;
		}
		memcpy(x->data, data, init_sz);
		x->data_owned = true;
	} else {
		x->data = data;
		x->data_owned = false;
	}
	x->size = init_sz;
	x->ok_modes = ok_modes;
	
	if (!nullfs_add(f, name, x, &nullfs_ram_ops)) {
		if (x->data_owned) free(x->data);
		free(x);
		return false;
	}
	return true;
}

void* nullfs_i_ram_open(void* fi, int mode, fs_handle_t *h) {
	nullfs_ram_file_t *f = (nullfs_ram_file_t*)fi;

	if (mode & ~f->ok_modes) {
		return 0;
	}

	h->mode = mode;
	return fi;
}

size_t nullfs_i_ram_read(void* fi, size_t offset, size_t len, char* buf) {
	nullfs_ram_file_t *f = (nullfs_ram_file_t*)fi;

	if (offset >= f->size) return 0;
	if (offset + len > f->size) len = f->size - offset;

	memcpy(buf, f->data + offset, len);
	return len;
}

size_t nullfs_i_ram_write(void* fi, size_t offset, size_t len, const char* buf) {
	nullfs_ram_file_t *f = (nullfs_ram_file_t*)fi;

	if (offset + len > f->size) {
		// resize buffer (zero out new portion)
		void* new_buffer = malloc(offset + len);
		if (new_buffer == 0) return 0;

		memcpy(new_buffer, f->data, f->size);
		if (offset > f->size)
			memset(new_buffer + f->size, 0, offset - f->size);

		if (f->data_owned) free(f->data);
		f->data = new_buffer;
		f->data_owned = true;
		f->size = offset + len;
	}

	memcpy(f->data + offset, buf, len);
	return len;
}

void nullfs_i_ram_dispose(void* fi) {
	nullfs_ram_file_t *f = (nullfs_ram_file_t*)fi;
	
	if (f->data_owned) free(f->data);
	free(f);
}



/* vim: set ts=4 sw=4 tw=0 noet :*/
