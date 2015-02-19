#include <worker.h>
#include <btree.h>
#include <mutex.h>
#include <malloc.h>

static uint64_t time = 0;
static uint64_t next_task_time = UINT64_MAX;

static thread_t **workers = 0;
static int nworkers = 0;

static btree_t *tasks = 0;
STATIC_MUTEX(tasks_mutex);

static void worker_thread(void*);

typedef struct {
	uint64_t time;
	entry_t fun;
	void* data;
} worker_task_t;

int uint64_key_cmp_fun(const void* a, const void* b) {
	uint64_t *x = (uint64_t*)a, *y = (uint64_t*)b;
	return (*x == *y ? 0 : (*x > *y ? 1 : -1));
}

void start_workers(int n) {
	ASSERT(n > 0);
	workers = (thread_t**)malloc(n * sizeof(thread_t*));
	ASSERT(workers != 0);

	tasks = create_btree(uint64_key_cmp_fun, 0);
	ASSERT(tasks != 0);

	nworkers = n;
	for (int i = 0; i < n; i++) {
		workers[i] = new_thread(worker_thread, 0);
	}
}

void worker_thread(void* x) {
	uint64_t zero64 = 0;

	while (true) {
		mutex_lock(&tasks_mutex);
		worker_task_t *t = btree_upper(tasks, &zero64);
		next_task_time = (t == 0 ? UINT64_MAX : t->time);
		if (t != 0 && t->time <= time) {
			btree_remove_v(tasks, &t->time, t);
		} else {
			t = 0;
		}
		mutex_unlock(&tasks_mutex);
		
		if (t != 0) {
			// do task :-)
			t->fun(t->data);
			free(t);
		} else {
			pause();
		}
	}
}

bool worker_push_in(int usecs, entry_t fun, void* data) {
	worker_task_t *t = (worker_task_t*)malloc(sizeof(worker_task_t));
	if (t == 0) return false;

	t->time = time + usecs;
	t->fun = fun;
	t->data = data;

	mutex_lock(&tasks_mutex);
	btree_add(tasks, &t->time, t);
	mutex_unlock(&tasks_mutex);
	
	if (t->time < next_task_time) next_task_time = t->time;

	return true;
}

bool worker_push(entry_t fun, void* data) {
	return worker_push_in(0, fun, data);
}

void worker_notify_time(int usecs) {
	time += usecs;
	if (next_task_time <= time) {
		for (int i = 0; i < nworkers; i++) {
			if (resume_thread(workers[i], false)) break;
		}
	}
}

uint64_t worker_get_time() {
	return time;
}

/* vim: set ts=4 sw=4 tw=0 noet :*/
