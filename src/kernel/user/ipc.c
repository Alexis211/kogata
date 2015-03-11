#include <string.h>

#include <ipc.h>
#include <mutex.h>
#include <thread.h>

#include <prng.h>
#include <worker.h>

#include <hashtbl.h>

size_t channel_read(fs_handle_t *c, size_t offset, size_t len, char* buf);
size_t channel_write(fs_handle_t *c, size_t offset, size_t len, const char* buf);
int channel_poll(fs_handle_t *c, void** out_wait_obj);
bool channel_open(fs_node_t *c, int mode);
bool channel_stat(fs_node_t *c, stat_t *st);
void channel_close(fs_handle_t *c);
void channel_dispose(fs_node_t *c);

fs_node_ops_t channel_ops = {
	.read = channel_read,
	.write = channel_write,
	.close = channel_close,
	.poll = channel_poll,
	.open = channel_open,
	.readdir = 0,
	.ioctl = 0,
	.stat = channel_stat,
	.walk = 0,
	.create = 0,
	.move = 0,
	.delete = 0,
	.dispose = channel_dispose,
};

typedef struct channel {
	struct channel *other_side;

	mutex_t lock;

	char buf[CHANNEL_BUFFER_SIZE];
	size_t buf_use_begin, buf_used;		// circular buffer
} channel_t;

fs_handle_pair_t make_channel(bool blocking) {
	fs_node_t *na = 0, *nb = 0;
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

	na = (fs_node_t*)malloc(sizeof(fs_node_t));
	if (na == 0) goto error;

	nb = (fs_node_t*)malloc(sizeof(fs_node_t));
	if (nb == 0) goto error;

	a->other_side = b;
	b->other_side = a;
	a->lock = b->lock = MUTEX_UNLOCKED;
	a->buf_use_begin = a->buf_used = 0;
	b->buf_use_begin = b->buf_used = 0;

	memset(na, 0, sizeof(fs_node_t));
	memset(nb, 0, sizeof(fs_node_t));
	na->refs = nb->refs = 1;
	na->ops = nb->ops = &channel_ops;
	na->data = a;
	nb->data = b;

	ret.a->fs = ret.b->fs = 0;
	ret.a->node = na;
	ret.b->node = nb;
	ret.a->refs = ret.b->refs = 1;
	ret.a->mode = ret.b->mode = FM_READ | FM_WRITE | (blocking ? FM_BLOCKING : 0);

	return ret;

error:
	if (a) free(a);
	if (b) free(b);
	if (ret.a) free(ret.a);
	if (ret.b) free(ret.b);
	if (na) free(na);
	if (nb) free(nb);
	ret.a = ret.b = 0;
	return ret;
}

size_t channel_read(fs_handle_t *h, size_t offset, size_t req_len, char* orig_buf) {
	channel_t *c = (channel_t*)h->node->data;

	size_t ret = 0;
	
	do {
		size_t len = req_len - ret;
		char *buf = orig_buf + ret;

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

		ret += len;

		mutex_unlock(&c->lock);
	} while ((h->mode & FM_BLOCKING) && ret < req_len);

	return ret;
}

size_t channel_write(fs_handle_t *h, size_t offset, size_t req_len, const char* orig_buf) {
	channel_t *tc = (channel_t*)h->node->data;
	channel_t *c = tc->other_side;
	
	if (c == 0) return 0;

	size_t ret = 0;

	do {
		size_t len = req_len - ret;
		const char* buf = orig_buf + ret;

		while (!mutex_try_lock(&c->lock)) {
			yield();
			if (tc->other_side == 0) break;
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
		ret += len;
	} while ((h->mode & FM_BLOCKING) && ret < req_len);

	return ret;
}

int channel_poll(fs_handle_t *h, void** out_wait_obj) {
	channel_t *c = (channel_t*)h->node->data;

	int ret = 0;

	if (c->other_side == 0) ret |= SEL_ERROR;
	if (c->other_side && c->other_side->buf_used < CHANNEL_BUFFER_SIZE) ret |= SEL_WRITE;
	if (c->buf_used > 0) ret |= SEL_READ;

	if (out_wait_obj) *out_wait_obj = c;

	return ret;
}

bool channel_open(fs_node_t *ch, int mode) {
	int ok_modes = FM_READ | FM_WRITE | FM_BLOCKING;

	if (mode & ~ok_modes) return false;
	return true;
}

bool channel_stat(fs_node_t *ch, stat_t *st) {
	channel_t *c = (channel_t*)ch->data;

	mutex_lock(&c->lock);

	st->type = FT_CHANNEL;
	st->size = c->buf_used;
	st->access = FM_READ | FM_WRITE;

	mutex_unlock(&c->lock);

	return true;
}

void channel_close(fs_handle_t *ch) {
	// do nothing
}

void channel_dispose(fs_node_t *n) {
	channel_t *c = (channel_t*)n->data;

	mutex_lock(&c->lock);

	if (c->other_side) {
		resume_on(c->other_side);
		c->other_side->other_side = 0;
	}

	free(c);
}

//  ---- ------ ------
//  ---- Shared memory
//  ---- ------ ------

bool shm_open(fs_node_t *n, int mode);
void shm_close(fs_handle_t *h);
bool shm_stat(fs_node_t *n, stat_t *st);
void shm_dispose(fs_node_t *n);

fs_node_ops_t shm_ops = {
	.open = shm_open,
	.read = fs_read_from_pager,
	.write = fs_write_to_pager,
	.readdir = 0,
	.poll = 0,
	.close = shm_close,
	.stat = shm_stat,
	.ioctl = 0,
	.walk = 0,
	.delete = 0,
	.move = 0,
	.create = 0,
	.dispose = shm_dispose,
};

fs_handle_t* make_shm(size_t size) {
	fs_node_t *n = 0;
	fs_handle_t *h = 0;

	n = (fs_node_t*)malloc(sizeof(fs_node_t));
	if (n == 0) goto error;
	memset(n, 0, sizeof(fs_node_t));

	h = (fs_handle_t*)malloc(sizeof(fs_handle_t));
	if (h == 0) goto error;

	n->pager = new_swap_pager(size, false);
	if (n->pager == 0) goto error;

	n->refs = 1;
	n->lock = MUTEX_UNLOCKED;
	n->ops = &shm_ops;
	n->data = 0;

	h->fs = 0;
	h->node = n;
	h->refs = 1;
	h->mode = FM_READ | FM_WRITE | FM_MMAP;

	return h;

error:
	if (n && n->pager) delete_pager(n->pager);
	if (n) free(n);
	if (h) free(h);
	return 0;
}

bool shm_open(fs_node_t *n, int mode) {
	int ok_modes = FM_READ | FM_WRITE | FM_MMAP;
	if (mode & ~ok_modes) return false;

	return true;
}

void shm_close(fs_handle_t *h) {
	// nothing to do
}

bool shm_stat(fs_node_t *n, stat_t *st) {
	st->size = n->pager->size;
	st->type = FT_REGULAR;
	st->access = FM_READ | FM_WRITE | FM_MMAP;

	return true;
}

void shm_dispose(fs_node_t *n) {
	delete_pager(n->pager);
}

//  ---- ------
//  ---- Tokens
//  ---- ------

static hashtbl_t *token_table = 0;
STATIC_MUTEX(token_table_mutex);

typedef struct {
	token_t tok;
	fs_handle_t *h;
	uint64_t time;
} token_table_entry_t;

static token_table_entry_t *expired_token = 0;

void token_expiration_check(void* x) {
	mutex_lock(&token_table_mutex);

	do {
		expired_token = 0;

		void find_expired_token(void* k, void* x) {
			token_table_entry_t *e = (token_table_entry_t*)x;
			if (e->time + TOKEN_LIFETIME < get_kernel_time()) {
				expired_token = e;
			}
		}
		hashtbl_iter(token_table, find_expired_token);

		if (expired_token) {
			hashtbl_remove(token_table, &expired_token->tok);
			unref_file(expired_token->h);
			free(expired_token);
		}
	} while (expired_token != 0);

	mutex_unlock(&token_table_mutex);

	while (!worker_push_in(1000000, token_expiration_check, 0)) yield();
}

bool gen_token_for(fs_handle_t *h, token_t *tok) {
	bool ok = false;

	token_table_entry_t *e = 0;

	mutex_lock(&token_table_mutex);

	if (token_table == 0) {
		token_table = create_hashtbl(token_eq_fun, token_hash_fun, 0);
		if (token_table == 0) goto end;

		while (!worker_push_in(1000000, token_expiration_check, 0)) yield();
	}

	e = (token_table_entry_t*)malloc(sizeof(token_t));
	if (!e) goto end;

	prng_bytes((uint8_t*)e->tok.bytes, TOKEN_LENGTH);
	memcpy(tok->bytes, e->tok.bytes, TOKEN_LENGTH);
	e->h = h;
	e->time = get_kernel_time();

	ok = hashtbl_add(token_table, &e->tok, e);
	if (!ok) goto end;

	ref_file(h);
	ok = true;

end:
	if (!ok && e) free(e);
	mutex_unlock(&token_table_mutex);
	return ok;
}

fs_handle_t* use_token(token_t* tok) {
	fs_handle_t *ret = 0;

	mutex_lock(&token_table_mutex);

	token_table_entry_t *e = hashtbl_find(token_table, tok);
	if (e != 0) {
		ret = e->h;

		hashtbl_remove(token_table, tok);
		free(e);
	}

	mutex_unlock(&token_table_mutex);
	return ret;

}

hash_t token_hash_fun(const void* v) {
	token_t *t = (token_t*)v;
	hash_t h = 707;
	for (int i = 0; i < TOKEN_LENGTH; i++) {
		h = h * 101 + t->bytes[i];
	}
	return h;
}

bool token_eq_fun(const void* a, const void* b) {
	token_t *ta = (token_t*)a, *tb = (token_t*)b;
	for (int i = 0; i < TOKEN_LENGTH; i++) {
		if (ta->bytes[i] != tb->bytes[i]) return false;
	}
	return true;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
