#include <string.h>

#include <ipc.h>
#include <mutex.h>
#include <thread.h>

static size_t channel_read(fs_node_ptr c, size_t offset, size_t len, char* buf);
static size_t channel_write(fs_node_ptr c, size_t offset, size_t len, const char* buf);
static bool channel_stat(fs_node_ptr c, stat_t *st);
static void channel_close(fs_node_ptr c);

static fs_node_ops_t channel_ops = {
	.read = channel_read,
	.write = channel_write,
	.close = channel_close,
	.open = 0,
	.readdir = 0,
	.ioctl = 0,
	.stat = channel_stat,
	.walk = 0,
	.create = 0,
	.move = 0,
	.delete = 0,
	.dispose = 0,
};

typedef struct channel {
	struct channel *other_side;

	mutex_t lock;

	char buf[CHANNEL_BUFFER_SIZE];
	size_t buf_use_begin, buf_used;		// circular buffer
} channel_t;

fs_handle_pair_t make_channel() {
	fs_handle_pair_t ret = { .a = 0, .b = 0 };
	channel_t *a = 0, *b = 0;

	a = (channel_t*)malloc(sizeof(channel_t));
	if (a == 0) goto error;

	b = (channel_t*)malloc(sizeof(channel_t));
	if (b == 0) goto error;

	ret.a = (fs_handle_t*)malloc(sizeof(fs_handle_t));
	if (ret.a == 0) goto error;

	ret.b = (fs_handle_t*)malloc(sizeof(fs_handle_t));
	if (ret.b == 0) goto error;

	a->other_side = b;
	b->other_side = a;
	a->lock = b->lock = MUTEX_UNLOCKED;
	a->buf_use_begin = a->buf_used = 0;
	b->buf_use_begin = b->buf_used = 0;

	ret.a->fs = ret.b->fs = 0;
	ret.a->node = ret.b->node = 0;
	ret.a->ops = ret.b->ops = &channel_ops;
	ret.a->data = a;
	ret.b->data = b;
	ret.a->refs = ret.b->refs = 1;
	ret.a->mode = ret.b->mode = FM_READ | FM_WRITE;

	return ret;

error:
	if (a) free(a);
	if (b) free(b);
	if (ret.a) free(ret.a);
	if (ret.b) free(ret.b);
	ret.a = ret.b = 0;
	return ret;
}

size_t channel_read(fs_node_ptr ch, size_t offset, size_t len, char* buf) {
	channel_t *c = (channel_t*)ch;
	
	mutex_lock(&c->lock);

	if (c->buf_used < len) len = c->buf_used;

	if (len) {
		size_t len0 = len, len1 = 0;

		if (c->buf_use_begin + len > CHANNEL_BUFFER_SIZE) {
			len0 = CHANNEL_BUFFER_SIZE - c->buf_use_begin;
			len1 = len - len0;
		}
		memcpy(buf, c->buf + c->buf_use_begin, len0);
		if (len1) memcpy(buf + len0, c->buf, len1);

		c->buf_use_begin = (c->buf_use_begin + len) % CHANNEL_BUFFER_SIZE;
		c->buf_used -= len;
		
		if (c->buf_used == 0) c->buf_use_begin = 0;
	}

	mutex_unlock(&c->lock);

	return len;
}

size_t channel_write(fs_node_ptr ch, size_t offset, size_t len, const char* buf) {
	channel_t *tc = (channel_t*)ch;
	channel_t *c = tc->other_side;
	
	if (c == 0) return 0;

	while (!mutex_try_lock(&c->lock)) {
		yield();
		if (tc->other_side == 0) return 0;
	}

	if (c->buf_used + len > CHANNEL_BUFFER_SIZE) len = CHANNEL_BUFFER_SIZE - c->buf_used;

	if (len) {
		size_t len0 = len, len1 = 0;

		if (c->buf_use_begin + c->buf_used + len > CHANNEL_BUFFER_SIZE) {
			len0 = CHANNEL_BUFFER_SIZE - c->buf_use_begin - c->buf_used;
			len1 = len - len0;
		}
		memcpy(c->buf + c->buf_use_begin + c->buf_used, buf, len0);
		if (len1) memcpy(c->buf, buf + len0, len1);

		c->buf_used += len;
	}

	mutex_unlock(&c->lock);

	if (len) resume_on(c);	// notify processes that may be waiting for data

	return len;
}

bool channel_stat(fs_node_ptr ch, stat_t *st) {
	channel_t *c = (channel_t*)ch;

	mutex_lock(&c->lock);

	st->type = FT_CHANNEL;
	st->size = c->buf_used;
	st->access = FM_READ | FM_WRITE;

	mutex_unlock(&c->lock);

	return true;
}

void channel_close(fs_node_ptr ch) {
	channel_t *c = (channel_t*)ch;

	mutex_lock(&c->lock);

	c->other_side->other_side = 0;
	free(c);
}


/* vim: set ts=4 sw=4 tw=0 noet :*/
