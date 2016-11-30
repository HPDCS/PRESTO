#ifndef SINGLE_POOL

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../arch/ult.h"
#include "../datatypes/state.h"

extern void worker_runMethod(void*) __attribute__ ((noreturn));

static short unsigned int init = 0;
static struct state_pool pool;

static inline int init_state(state_t* state, state_t* next, int pos) {
	memset(&state->context, 0, sizeof(exec_context_t));
	if ((state->stack = get_ult_stack(pos, STACK_SIZE)) == NULL)
		return -1;
	context_create(&state->context, worker_runMethod, NULL, state->stack, STACK_SIZE);
	stm_init_thread();
	state->stmtx = thread_tx;
	thread_tx = NULL;
#ifndef NBLIST_STATE
	state->next = next;
#endif
	return 0;
}

static inline void fini_state(state_t* state) {
	free(state->stack);
	thread_tx = state->stmtx;
	stm_exit_thread();
	state->stmtx = NULL;
}

int StatePoolInit(int max_state) {
	int i, j;
	if (init)
		goto error0;
	if (max_state <= 0)
		goto error0;
	pool.size = max_state;
	if ((pool.array = (state_t*) malloc(max_state * sizeof(state_t))) == NULL)
		goto error0;
#ifdef NBLIST_STATE
	if (NBListInit(&pool.list, max_state))
		goto error1;
#else
	if (pthread_spin_init(&pool.lock, PTHREAD_PROCESS_PRIVATE))
		goto error1;
	pool.head = &pool.array[0];
	pool.tail = &pool.array[pool.size-1];
#endif
	for (i=0; i<max_state; i++) {
		if (init_state(&pool.array[i], ((i == max_state-1) ? NULL : &pool.array[i+1]), i) == -1)
			goto error2;
#ifdef NBLIST_STATE
		if (NBListAdd(pool.list, (void*) &pool.array[i])) {
			i++;
			goto error3;
		}
#endif
	}
	init = 1;
	return 0;
#ifdef NBLIST_STATE
error3:
	NBListDestroy(pool.list);
#endif
error2:
	for (j=0; j<i; j++)
		fini_state(&pool.array[j]);
error1:
	free(pool.array);
error0:
	return -1;
}

void StatePoolDestroy() {
	int i;
	if (!init)
		return;
#ifdef NBLIST_STATE
	NBListDestroy(pool.list);
#else
	pthread_spin_destroy(&pool.lock);
#endif
	for (i=0; i<pool.size; i++) {
		fini_state(&pool.array[i]);
	}
	free(pool.array);
	init = 0;
}

state_t* GetState() {
	state_t* state;
	if (!init)
		return NULL;
	state = NULL;
#ifdef NBLIST_STATE
	while (NBListRemove(pool.list, (void**) &state)) ;
#else
	while (state == NULL) {
		while (pool.head == NULL) ;
		if (pthread_spin_trylock(&pool.lock))
			continue;
		if (pool.head != NULL) {
			state = pool.head;
			pool.head = state->next;
			if (pool.head == NULL)
				pool.tail = NULL;
		}
		pthread_spin_unlock(&pool.lock);
	}
	state->next = NULL;
#endif
	return state;
}

void FreeState(state_t* state) {
	if (state == NULL)
		return;
	if (!init)
		return;
#ifdef NBLIST_STATE
	while (NBListAdd(pool.list, (void*) state)) ;
#else
	pthread_spin_lock(&pool.lock);
	if (pool.tail != NULL) {
		pool.tail->next = state;
		pool.tail = state;
	} else {
		pool.head = pool.tail = state;
	}
	pthread_spin_unlock(&pool.lock);
#endif
}

#endif
