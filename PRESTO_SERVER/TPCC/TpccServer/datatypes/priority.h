#pragma once

#include <pthread.h>
#include "task.h"

#ifdef BITMAP
typedef unsigned char byte_t;

#define GET_BYTE_NUM(pos)		(int) (pos / 8)
#define GET_BIT_NUM(pos)		(int) (pos % 8)
#define GET_POS_NUM(byte, bit)	(int) (byte * 8) + bit

#define BYTE_TO_INT(byte)		(int) byte
#define ONE_SHIFT(bit)			(1 << bit)

#define IS_EMPTY(byte)			((int) byte) == 0
#define IS_FULL(byte)			((int) byte) == 255

#define CHECK_BIT(byte, bit)	(byte_t) (((int) byte) & (1 << bit))
#define SET_BIT(byte, bit)		(byte_t) (((int) byte) | (1 << bit))
#define RESET_BIT(byte, bit)	(byte_t) (((int) byte) & ~(1 << bit))
#endif

typedef struct task_list {
	task_t*				head;
	task_t*				tail;
	pthread_spinlock_t	lock;
	int					count;
} task_list_t;

struct prio_task_array {
	int					num_prio;
#ifdef BITMAP
	int					num_bytes;
	byte_t*				n_bitmap;
	byte_t*				s_bitmap;
#endif
	task_list_t*		new;
	task_list_t*		suspended;
};

struct prio_task_array*		GetPrioTaskArray(int);
void						FreePrioTaskArray(struct prio_task_array**);
int							InsertNewTask(struct prio_task_array*, task_t*);
int							InsertSuspendedTask(struct prio_task_array*, task_t*, int);
int							InsertSuspendedTaskWithPrio(struct prio_task_array*, task_t*, int, int);
task_t*						RemoveNewTask(struct prio_task_array*, int);
task_t*						RemoveSuspendedTask(struct prio_task_array*, int);
task_t*						TryRemoveNewTask(struct prio_task_array*, int);
task_t*						TryRemoveSuspendedTask(struct prio_task_array*, int);
task_t*						GetHigherPrioTask(struct prio_task_array*, int);
task_t*						GetHighestPrioTask(struct prio_task_array*);
