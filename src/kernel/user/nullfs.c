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

nullfs_t *make_nullfs(char* options) {
	fs_t *it = make_fs("nullfs", 0, options);
	if (it == 0) return 0;
	if (it->ops != &nullfs_ops) return 0;
	return (nullfs_t*)it->data;
}

bool nullfs_i_make(fs_handle_t *source, char* opts, fs_t *d) {
	// TODO
	return false;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
