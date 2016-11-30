/*
 *	scheduler/scheduler.h
 *
 *	@author:	Emiliano Silvestri
 *	@email:		emisilve86@gmail.com
 */

#pragma once

#include "../arch/ult.h"
#include "../datatypes/state.h"
#include "../datatypes/task.h"
#include "../datatypes/priority.h"

#define PLATFORM_MODE		0x0001
#define TRANSACTION_MODE	0x0000

#define NOT_PREEMPTABLE		0x0001
#define IS_PREEMPTABLE		0x0000

#define TICK_UP				0x0001
#define TICK_DOWN			0x0000

#define FIRST_DONE			0x0001
#define FIRST_NOT_DONE		0x0000

#define switch_to_platform_mode()		do { \
											if (running_task != NULL) { \
												mode = PLATFORM_MODE; \
											} \
										} while(0)

#define switch_to_transaction_mode()	do { \
											if (running_task != NULL) { \
												mode = TRANSACTION_MODE; \
											} \
										} while(0)

#define make_transaction_preemptable()		do { \
												if (mode == TRANSACTION_MODE) { \
													preemptable = IS_PREEMPTABLE; \
												} \
											} while(0)

#define make_transaction_not_preemptable()	do { \
												if (mode == TRANSACTION_MODE) { \
													preemptable = NOT_PREEMPTABLE; \
												} \
											} while(0)

void	schedule(void);

int		preempt_init(void);
int		enable_preemption(void);
int		disable_preemption(void);

extern struct prio_task_array*	pta;

/* Platform or Transaction mode ? */
extern __thread unsigned short int		mode;
/* Even if Transaction mode, Preemptable or Not ? */
extern __thread unsigned short int		preemptable;
/* In case a Non-Preemptable block completes, is there a Pending Tick ? */
extern __thread unsigned short int		standing_tick;
/* To indicate whether the first operation on shared data has been performed */
extern __thread unsigned short int		first_tx_operation;
