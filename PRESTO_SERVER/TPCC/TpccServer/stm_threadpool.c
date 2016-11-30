/*
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file threadpool.c
 * @brief Threadpool implementation file
 */

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>

#include <stdio.h>

#include <stm.h>
#include "../lib/tm.h"
#include "stm_threadpool.h"
#include "arch/atomic.h"
#include "arch/ult.h"
#include "datatypes/task.h"
#include "scheduler/scheduler.h"

#ifdef TXS_STATS
#include "stats/stats.h"
#endif

/* Link to the pool which every thread refers to */
__thread worker_threadpool_t*		threadpool;

worker_threadpool_t* worker_threadpool_create(struct prio_task_array* pta, int thread_count)
{
    worker_threadpool_t* pool;
    int i;

    /* TODO: Check for negative or otherwise very big input parameters */

    if ((pool = (worker_threadpool_t*) malloc(sizeof(worker_threadpool_t))) == NULL) {
        goto err;
    }

    /* Initialize */
#ifdef SCHED_MISS_COUNT
    pool->can_increment = 0;
#endif
    pool->shutdown = 0;
    pool->started = 0;
    pool->thread_count = thread_count;

    /* Allocate thread */
    pool->threads = (pthread_t*) malloc(sizeof(pthread_t) * thread_count);

    /* Link to the Prio List Array */
    pool->pta = pta;

#ifdef EXTRA_TICK
    if (preempt_init())
    	printf("ERROR: preempt_init() has failed!!!\n");
#endif

    for(i = 0; i < thread_count; i++) {
        if(pthread_create(&(pool->threads[i]), NULL, worker_threadpool_thread, (void*) pool) != 0) {
            worker_threadpool_destroy(pool);
            return NULL;
        } else {
        	atomic_inc(&pool->started);
        }
    }

    return pool;

 err:
    if(pool) {
        worker_threadpool_free(pool);
    }
    return NULL;
}

server_threadpool_t* server_threadpool_create(struct prio_task_array* pta, int num_servers)
{
	server_threadpool_t* pool;
	int i;

	/* TODO: Check for negative or otherwise very big input parameters */

	if ((pool = (server_threadpool_t*) malloc(sizeof(server_threadpool_t))) == NULL) {
		goto err;
	}

	/* Initialize */
	pool->can_start = 0;
	pool->started = 0;
	pool->thread_created = 0;
	pool->thread_count = num_servers;

	/* Allocate thread */
	if ((pool->threads = (server_thread_t*) malloc(sizeof(server_thread_t) * num_servers)) == NULL) {
		goto err;
	}

	for (i=0; i<num_servers; i++) {
		pool->threads[i].pool = (void*) pool;
		pool->threads[i].conn = -1;
		pool->threads[i].txs = 0;
#ifdef SINGLE_POOL
		pool->threads[i].gc_head = NULL;
#endif
	}

	/* Link to the Prio List Array */
	pool->pta = pta;

	return pool;

 err:
	if(pool) {
		server_threadpool_free(pool);
	}
	return NULL;
}

int worker_threadpool_destroy(worker_threadpool_t *pool)
{
    int i, err = 0;

    if(pool == NULL) {
        return threadpool_invalid;
    }

    do {
        /* Already shutting down */
        if(pool->shutdown) {
            err = threadpool_shutdown;
            break;
        }

        pool->shutdown = 1;

        /* Join all worker thread */
        for(i = 0; i < pool->thread_count; i++) {
            if(pthread_join(pool->threads[i], NULL) != 0) {
                err = threadpool_thread_failure;
            }
        }
    } while(0);

    /* Only if everything went well do we deallocate the pool */
    if(!err) {
        worker_threadpool_free(pool);
    }
    
    return err;
}

int server_threadpool_destroy(server_threadpool_t *pool)
{
    int i, err = 0;

    if(pool == NULL) {
        return threadpool_invalid;
    }

    for (i = 0; i < pool->thread_created; i++) {
		if (pthread_join(pool->threads[i].thread, NULL) != 0) {
			err = threadpool_thread_failure;
		}
	}

    /* Only if everything went well do we deallocate the pool */
    if (!err) {
        server_threadpool_free(pool);
    }

    return err;
}

int worker_threadpool_free(worker_threadpool_t* pool)
{
    if(pool == NULL || pool->started > 0) {
        return -1;
    }

    /* Did we manage to allocate ? */
    if(pool->threads) {
        free(pool->threads);
    }
    free(pool);
    return 0;
}

int server_threadpool_free(server_threadpool_t* pool)
{
    if(pool == NULL || pool->started > 0) {
        return -1;
    }

    /* Did we manage to allocate ? */
    if(pool->threads) {
        free(pool->threads);
    }
    free(pool);
    return 0;
}

static void* worker_threadpool_thread(void* worker_threadpool)
{
    threadpool = (worker_threadpool_t*) worker_threadpool;
    task_t* task;

    running_task = NULL;
    mode = PLATFORM_MODE;
    preemptable = NOT_PREEMPTABLE;
    standing_tick = TICK_DOWN;
    first_tx_operation = FIRST_NOT_DONE;

#ifdef EXTRA_TICK
    if (enable_preemption())
    	printf("ERROR: enable_preemption() has failed!!!\n");
#endif

    for(;;) {

    	schedule();

		if (running_task == NULL) {
			if (threadpool->shutdown) {
				break;
			} else {
				continue;
			}
		}

#ifdef SINGLE_POOL
		context_switch(&platform_context, &running_task->state.context);
#else
		context_switch(&platform_context, &running_task->state->context);
#endif

		thread_tx = NULL;
		task = running_task;
		running_task = NULL;

#ifdef SINGLE_POOL
#ifdef TXS_STATS
		task->valid = 1;
#endif
		task->free_gc = 1;
#else
#ifdef TXS_STATS
		StatsUpdate(task);
#endif
		FreeTask(task); /* Also call FreeState(task->state) */
#endif
    }

#ifdef EXTRA_TICK
    if (disable_preemption())
    	printf("ERROR: disable_preemption() has failed!!!\n");
#endif

    atomic_dec(&threadpool->started);

    pthread_exit(NULL);
    return(NULL);
}

#ifdef SINGLE_POOL
void gc_insert(server_thread_t* thread, task_t* task) {
	task->next_gc = thread->gc_head;
	thread->gc_head = task;
}

void gc_clean(server_thread_t* thread) {
	task_t* task = thread->gc_head;
	task_t* task_prev = NULL;
	while (task != NULL) {
		if (task->free_gc) {
#ifdef TXS_STATS
			if (task->valid)
				StatsUpdate(task);
#endif
			if (task_prev == NULL) {
				thread->gc_head = task->next_gc;
				FreeTask(task);
				task = thread->gc_head;
			} else {
				task_prev->next_gc = task->next_gc;
				FreeTask(task);
				task = task_prev->next_gc;
			}
		} else {
			task_prev = task;
			task = task_prev->next_gc;
		}
	}
}
#endif

static void* server_threadpool_thread(void* server_thread)
{
#ifdef SINGLE_POOL
	int i = 1;
#endif

    server_thread_t* thread = (server_thread_t*) server_thread;
    server_threadpool_t* pool = (server_threadpool_t*) thread->pool;

    while (!pool->can_start) ;

    while (thread->txs) {

    	server_runMethod(pool->pta, thread);

    	thread->txs--;

#ifdef SINGLE_POOL
    	if (i >= GC_PERIOD) {
    		gc_clean(thread);
    		i = 1;
    	} else {
    		i++;
    	}
#endif

    }

#ifdef TXS_STATS
    DisableUpdate();
#endif

    close(thread->conn);

#ifdef SINGLE_POOL
    while(thread->gc_head != NULL)
    	gc_clean(thread);
#endif

    atomic_dec(&pool->started);

    pthread_exit(NULL);
    return(NULL);
}

int server_threadpool_add(server_threadpool_t* pool, int connaddr, int transactions)
{
    int err = 0;

    if(pool == NULL || connaddr < 0) {
        err = threadpool_invalid;
    }

    if (pool->thread_created < pool->thread_count) {
    	pool->threads[pool->thread_created].conn = connaddr;
        pool->threads[pool->thread_created].txs = transactions;
    	if (pthread_create(&(pool->threads[pool->thread_created].thread), NULL, server_threadpool_thread, (void*) &pool->threads[pool->thread_created]) != 0) {
    		pool->threads[pool->thread_created].conn = -1;
    		err = threadpool_invalid;
		} else {
			atomic_inc(&pool->started);
			pool->thread_created++;
		}
    } else {
    	err = threadpool_invalid;
    }

    return err;
}
