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
	ASSERT(d != 0); // should we fail in a more graceful manner ?

	d->name = name;
	d->ops = ops;
	d->next = drivers;
	drivers = d;
}

// ================================== //
// CREATING AND DELETINF FILE SYSTEMS //
// ================================== //

fs_t *make_fs(const char* drv_name, fs_handle_t *source, char* opts) {
	// Look for driver
	fs_driver_t *d = 0;
	for(fs_driver_t *i = drivers; i != 0; i = i->next) {
		if (drv_name != 0 && strcmp(i->name, drv_name) == 0) d = i;
		if (drv_name == 0 && source != 0 && i->ops->detect && i->ops->detect(source)) d = i;
		if (d != 0) break;
	}

	// Open file system
	fs_t *fs = (fs_t*)malloc(sizeof(fs_t));
	if (fs == 0) return 0;

	if (d->ops->make(source, opts, fs)) {
		fs->refs = 1;
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
		fs->ops->shutdown(fs->data);
		free(fs);
	}
}

// DOING THINGS IN FLESYSTEMS //

bool fs_delete(fs_t *fs, const char* file) {
	return fs->ops->delete && fs->ops->delete(fs->data, file);
}

bool fs_rename(fs_t *fs, const char* from, const char* to) {
	return fs->ops->rename && fs->ops->rename(fs->data, from, to);
}

bool fs_stat(fs_t *fs, const char* name, stat_t *st) {
	return fs->ops->stat && fs->ops->stat(fs->data, name, st);
}

int fs_ioctl(fs_t *fs, const char* file, int command, void* data) {
	return fs->ops->ioctl && fs->ops->ioctl(fs->data, file, command, data);
}

// =================== //
// OPERATIONS ON FILES //
// =================== //

fs_handle_t* fs_open(fs_t *fs, const char* file, int mode) {
	fs_handle_t *h = (fs_handle_t*)malloc(sizeof(fs_handle_t));
	if (h == 0) return 0;

	if (fs->ops->open(fs->data, file, mode, h)) {
		h->refs = 1;
		return h;
	} else {
		free(h);
		return 0;
	}
}

void ref_file(fs_handle_t *file) {
	file->refs++;
}

void unrefe_file(fs_handle_t *file) {
	file->refs--;
	if (file->refs == 0) {
		file->ops->close(file->data);
		free(file);
	}
}

size_t file_read(fs_handle_t *f, size_t offset, size_t len, char* buf) {
	if (!(f->mode && FM_READ)) return 0;

	return f->ops->read && f->ops->read(f->data, offset, len, buf);
}

size_t file_write(fs_handle_t *f, size_t offset, size_t len, const char* buf) {
	if (!(f->mode && FM_WRITE)) return 0;

	return f->ops->write && f->ops->write(f->data, offset, len, buf);
}

int file_get_mode(fs_handle_t *f) {
	return f->mode;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
