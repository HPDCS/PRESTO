#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "arch/atomic.h"
#include "../arch/ult.h"
#include "../datatypes/state.h"
#include "../datatypes/task.h"

#ifdef SINGLE_POOL
extern void worker_runMethod(void*) __attribute__ ((noreturn));
#endif

static short unsigned int init = 0;
static int num_free_tasks;
static struct task_pool pool;

static inline int init_task(task_t* task, task_t* next) {
	task->conn = -1;
	task->prio = -1;
	task->next = NULL;
	memset(task->args, 0, sizeof(task->args));
#ifdef SINGLE_POOL
	memset(&task->state.context, 0, sizeof(exec_context_t));
	if ((task->state.stack = get_ult_stack(0, STACK_SIZE)) == NULL)
		return -1;
	context_create(&task->state.context, worker_runMethod, NULL, task->state.stack, STACK_SIZE);
	stm_init_thread();
	task->state.stmtx = thread_tx;
	thread_tx = NULL;
#else
	task->state = NULL;
#endif
	task->num_susp = 0;
	task->susp_prio = -1;
#ifndef NBLIST_TASK
	task->next_free = next;
#endif
#ifdef SINGLE_POOL
	task->free_gc = 0;
	task->next_gc = NULL;
#endif
#ifdef TXS_STATS
	task->valid = 0;
	task->aborts = 0;
	task->commits = 0;
#endif
	task->txid = -1;
	return 0;
}

static inline void fini_task(task_t* task) {
#ifdef SINGLE_POOL
	free(task->state.stack);
	thread_tx = task->state.stmtx;
	stm_exit_thread();
	task->state.stmtx = NULL;
#else
	if (task->state != NULL)
		FreeState(task->state);
#endif
}

int TaskPoolInit(int pool_size) {
	int i, j;
	if (init)
		goto error0;
	if (pool_size <= 0)
		goto error0;
	pool.size = pool_size;
	if ((pool.array = (task_t*) malloc(pool_size * sizeof(task_t))) == NULL)
		goto error0;
#ifdef NBLIST_TASK
	if (NBListInit(&pool.list, pool_size))
		goto error1;
#else
	if (pthread_spin_init(&pool.lock, PTHREAD_PROCESS_PRIVATE))
		goto error1;
	pool.head = &pool.array[0];
	pool.tail = &pool.array[pool.size-1];
#endif
	for (i=0; i<pool_size; i++) {
		if (init_task(&pool.array[i], ((i == pool_size-1) ? NULL : &pool.array[i+1])) == -1)
			goto error2;
#ifdef NBLIST_TASK
		if (NBListAdd(pool.list, (void*) &pool.array[i])) {
			i++;
			goto error3;
		}
#endif
	}
	num_free_tasks = pool_size;
	init = 1;
	return 0;
#ifdef NBLIST_TASK
error3:
	NBListDestroy(pool.list);
#endif
error2:
	for (j=0; j<i; j++)
		fini_task(&pool.array[j]);
error1:
	free(pool.array);
error0:
	return -1;
}

void TaskPoolDestroy() {
	int i;
	if (!init)
		return;
#ifdef NBLIST_TASK
	NBListDestroy(pool.list);
#else
	pthread_spin_destroy(&pool.lock);
#endif
	for (i=0; i<pool.size; i++) {
		fini_task(&pool.array[i]);
	}
	free(pool.array);
	init = 0;
}

task_t* GetTask() {
	task_t* task;
	if (!init)
		return NULL;
	task = NULL;
#ifdef NBLIST_TASK
	if (NBListRemove(pool.list, (void**) &task))
		return NULL;
#else
	while (task == NULL) {
		while (pool.head == NULL)
			return NULL;
		if (pthread_spin_trylock(&pool.lock))
			continue;
		if (pool.head != NULL) {
			task = pool.head;
			pool.head = task->next_free;
			if (pool.head == NULL)
				pool.tail = NULL;
		}
		pthread_spin_unlock(&pool.lock);
	}
	task->next_free = NULL;
#endif
	task->num_susp = 0;
	task->susp_prio = -1;
	atomic_dec(&num_free_tasks);
#ifdef SINGLE_POOL
	task->free_gc = 0;
	task->next_gc = NULL;
#endif
#ifdef TXS_STATS
	task->valid = 0;
	task->aborts = 0;
	task->commits = 0;
#endif
	return task;
}

void FreeTask(task_t* task) {
	if (task == NULL)
		return;
	if (!init)
		return;
	task->conn = -1;
	task->prio = -1;
	task->next = NULL;
#ifndef SINGLE_POOL
	if (task->state != NULL)
		FreeState(task->state);
	task->state = NULL;
#endif
	task->num_susp = 0;
	task->susp_prio = -1;
#ifdef SINGLE_POOL
	task->free_gc = 0;
	task->next_gc = NULL;
#endif
#ifdef TXS_STATS
	task->valid = 0;
	task->aborts = 0;
	task->commits = 0;
#endif
#ifdef NBLIST_TASK
	while (NBListAdd(pool.list, (void*) task)) ;
#else
	pthread_spin_lock(&pool.lock);
	if (pool.tail != NULL) {
		pool.tail->next_free = task;
		pool.tail = task;
	} else {
		pool.head = pool.tail = task;
	}
	pthread_spin_unlock(&pool.lock);
#endif
	task->txid = -1;
	atomic_inc(&num_free_tasks);
}

int GetNumFreeTasks() {
	return num_free_tasks;
}