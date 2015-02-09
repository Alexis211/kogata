#include <nullfs.h>

bool nullfs_i_make(fs_handle_t *source, char* opts, fs_t *d);

fs_driver_ops_t nullfs_driver_ops = {
	.make = nullfs_i_make,
	.detect = 0,
};

fs_ops_t nullfs_ops = {
	0 	//TODO
};

void register_nullfs_driver() {
	register_fs_driver("nullfs", &nullfs_driver_ops);
}

nullfs_t *as_nullfs(fs_t *it) {
	if (it->ops != &nullfs_ops) return 0;
	return (nullfs_t*)it->data;
}

bool nullfs_i_make(fs_handle_t *source, char* opts, fs_t *d) {
	// TODO
	return false;
}

bool nullfs_add(nullfs_t *f, const char* name, void* data, nullfs_node_ops_t *ops) {
	// TODO
	return false;
}

bool nullfs_add_ram_file(nullfs_t *f, const char* name, void* data, size_t init_sz, int ok_modes) {
	// TODO
	return false;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
