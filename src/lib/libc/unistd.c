#include <string.h>
#include <unistd.h>

#include <kogata/syscall.h>


char cwd_buf[256];

char* getcwd(char* buf, size_t buf_len) {
	if (buf_len > strlen(cwd_buf)) {
		strcpy(buf, cwd_buf);
		return buf;
	} else {
		return 0;
	}
}

int chdir(const char* path) {
	char cwd_buf2[256];
	strcpy(cwd_buf2, cwd_buf);

	if (!pathncat(cwd_buf2, path, 256)) return -1;

	stat_t st;
	if (!sc_stat(cwd_buf2, &st)) return -1;
	if (!(st.type & FT_DIR)) return -1;

	strcpy(cwd_buf, cwd_buf2);
	return 0;
}

char* pathncat(char* buf, const char* add, size_t buf_len) {
	if (strchr(add, ':')) {
		if (strlen(add) < buf_len) {
			strcpy(buf, add);
			return buf;
		} else {
			return 0;
		}
	} else {
		char* sep_init = strchr(buf, ':');
		if (add[0] == '/') {
			if (strlen(add) + (sep_init + 1 - buf) < buf_len) {
				strcpy(sep_init + 1, add);
				return buf;
			} else {
				return 0;
			}
		} else {
			//TODO: simplify '..'
			char* end = buf + strlen(buf) - 1;
			if (*end != '/') end++;
			if (end + 1 - buf + strlen(add) < buf_len) {
				*end = '/';
				strcpy(end + 1, add);
				return buf;
			} else {
				return 0;
			}
		}
		return 0;
	}
}


/* vim: set ts=4 sw=4 tw=0 noet :*/
