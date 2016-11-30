/*
 *	scheduler/scheduler.c
 *
 *	@author:	Emiliano Silvestri
 *	@email:		emisilve86@gmail.com
 */

#include <stddef.h>
#include "../arch/atomic.h"
#include "scheduler.h"
#include "../stm_threadpool.h"

#ifdef SCHED_MISS_COUNT
/* Number of times the priority queues exploration gives no result */
int									exploration_miss;
#endif

/* Platform or Transaction mode ? */
__thread unsigned short int			mode;
/* Even if Transaction mode, Preemptable or Not ? */
__thread unsigned short int			preemptable;
/* In case a Non-Preemptable block completes, is there a Pending Tick? */
__thread unsigned short int			standing_tick;
/* To indicate whether the first operation on shared data has been performed */
__thread unsigned short int			first_tx_operation;

/* Link to the pool which every thread refers to */
extern __thread worker_threadpool_t*	threadpool;

void schedule() {
	int prio;
	task_t* task;
	if (running_task == NULL) {
		if ((running_task = GetHighestPrioTask(pta)) == NULL) {
#ifdef SCHED_MISS_COUNT
			if (threadpool->can_increment)
				atomic_inc(&exploration_miss);
#endif
			return;
		}
#ifdef SINGLE_POOL
		thread_tx = running_task->state.stmtx;
#else
		if (running_task->state == NULL)	/* NEW or SUSPENDED Task ? */
			running_task->state = GetState();
		thread_tx = running_task->state->stmtx;
#endif
	} else {
#ifdef PREEMPTION
		task = running_task;
		prio = (task->susp_prio > -1) ? task->susp_prio : task->prio;
		if ((running_task = GetHigherPrioTask(pta, prio)) == NULL) {
#ifdef SCHED_MISS_COUNT
			if (threadpool->can_increment)
				atomic_inc(&exploration_miss);
#endif
			running_task = task;
			return;
		}
#ifdef SINGLE_POOL
		thread_tx = running_task->state.stmtx;
#else
		if (running_task->state == NULL)	/* NEW or SUSPENDED Task ? */
			running_task->state = GetState();
		thread_tx = running_task->state->stmtx;
#endif
#ifdef SCHED_POLICY
		if (task->num_susp+1 < MAX_SUSPENSIONS && prio < pta->num_prio-1) {
#ifdef ALWAYS_INC_SCHED_POLICY
			InsertSuspendedTaskWithPrio(pta, task, prio+1, ((first_tx_operation == FIRST_DONE) ? 1 : 0));
#else
			InsertSuspendedTaskWithPrio(pta, task, prio, ((first_tx_operation == FIRST_DONE) ? 1 : 0));
#endif
		} else {
			InsertSuspendedTaskWithPrio(pta, task, pta->num_prio-1, ((first_tx_operation == FIRST_DONE) ? 1 : 0));
		}
#else
		InsertSuspendedTask(pta, task, ((first_tx_operation == FIRST_DONE) ? 1 : 0));
#endif
#endif
	}
}
