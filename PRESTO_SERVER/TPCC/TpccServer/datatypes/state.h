#pragma once

#include <stm.h>

#if !defined(SINGLE_POOL) && !defined(NBLIST_STATE)
#include <pthread.h>
#endif

#include "../arch/jmp.h"

#if !defined(SINGLE_POOL) && defined(NBLIST_STATE)
#include "nblist.h"
#endif

/* NBLIST_STATE must be defined to be used */

typedef struct state {
	exec_context_t		context;
	void*				stack;
	stm_tx_t*			stmtx;
#if !defined(SINGLE_POOL) && !defined(NBLIST_STATE)
	struct state*		next;
#endif
} state_t;

#ifndef SINGLE_POOL
struct state_pool {
	int					size;
	state_t*			array;
#ifdef NBLIST_STATE
	nb_list_t*			list;
#else
	state_t*			head;
	state_t*			tail;
	pthread_spinlock_t	lock;
#endif
};
#endif

#ifndef SINGLE_POOL
int			StatePoolInit(int);
void		StatePoolDestroy(void);
state_t*	GetState(void);
void		FreeState(state_t*);
#endif
