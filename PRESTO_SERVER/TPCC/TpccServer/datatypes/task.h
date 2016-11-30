#pragma once

#ifdef SINGLE_POOL
#include <stm.h>
#endif

#ifdef NBLIST_TASK
#include "nblist.h"
#else
#include <pthread.h>
#endif

#include "state.h"

/* NBLIST_TASK must be defined to be used */
/* SINGLE_POOL must be defined to be used */

typedef struct task {
	int					conn;
	int					prio;
	struct task*		next;
	char				args[256];
#ifdef SINGLE_POOL
	state_t				state;
#else
	state_t*			state;
#endif
	int					num_susp;
	int					susp_prio;
#ifndef NBLIST_TASK
	struct task*		next_free;
#endif
#ifdef SINGLE_POOL
	int					free_gc;
	struct task*		next_gc;
#endif
#ifdef TXS_STATS
	int					valid;
	int					aborts;
	int					commits;
	struct timespec		enqueue;
#ifdef DROP_CONT_TIME
	struct timespec		before_start;
#endif
	struct timespec		start;
	struct timespec		end;
#endif
	int					txid;
} task_t;

struct task_pool {
	int					size;
	task_t*				array;
#ifdef NBLIST_TASK
	nb_list_t*			list;
#else
	task_t*				head;
	task_t*				tail;
	pthread_spinlock_t	lock;
#endif
};

#ifdef TXS_STATS
#define incr_task_aborts()		do { \
									if (running_task != NULL) { \
										running_task->aborts++; \
									} \
								} while(0)

#define incr_task_commits()		do { \
									if (running_task != NULL) { \
										running_task->commits++; \
									} \
								} while(0)
#endif

int			TaskPoolInit(int);
void		TaskPoolDestroy(void);
task_t*		GetTask(void);
void		FreeTask(task_t*);
int			GetNumFreeTasks(void);