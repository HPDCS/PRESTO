#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../datatypes/task.h"
#include "../datatypes/priority.h"

#ifdef BITMAP
static inline int B_CAS(volatile byte_t* ptr, byte_t oldVal, byte_t newVal) {
	unsigned long res = 0;
	__asm__ __volatile__(
		"lock cmpxchgb %1, %2;"
		"lahf;"
		"bt $14, %%ax;"
		"adc %0, %0"
		: "=r"(res)
		: "r"(newVal), "m"(*ptr), "a"(oldVal), "0"(res)
		: "memory"
	);
	return (int) res;
}
#endif

static inline int task_list_init(task_list_t* list) {
	list->head = NULL;
	list->tail = NULL;
	if (pthread_spin_init(&list->lock, PTHREAD_PROCESS_PRIVATE))
		return -1;
	list->count = 0;
	return 0;
}

static inline task_t* remove_new_task(struct prio_task_array* pta, int prio) {
#ifdef BITMAP
	int byte, bit;
	byte_t bitmap_part, bitmap_part_set;
#endif
	task_t* task;
check:
	if (pta->new[prio].count == 0)
		return NULL;
	if (pthread_spin_trylock(&pta->new[prio].lock))
		goto check;
	if (pta->new[prio].count == 0) {
		pthread_spin_unlock(&pta->new[prio].lock);
		return NULL;
	}
	task = pta->new[prio].head;
	if ((pta->new[prio].head = task->next) == NULL)
		pta->new[prio].tail = NULL;
	task->next = NULL;
	pta->new[prio].count--;
#ifdef BITMAP
	if (pta->new[prio].count == 0) {
		byte = GET_BYTE_NUM(prio);
		bit = GET_BIT_NUM(prio);
		do {
			bitmap_part = pta->n_bitmap[byte];
			bitmap_part_set = RESET_BIT(bitmap_part, bit);
		} while (!B_CAS(&pta->n_bitmap[byte], bitmap_part, bitmap_part_set));
	}
#endif
	pthread_spin_unlock(&pta->new[prio].lock);
	return task;
}

static inline task_t* remove_suspended_task(struct prio_task_array* pta, int prio) {
#ifdef BITMAP
	int byte, bit;
	byte_t bitmap_part, bitmap_part_set;
#endif
	task_t* task;
check:
	if (pta->suspended[prio].count == 0)
		return NULL;
	if (pthread_spin_trylock(&pta->suspended[prio].lock))
		goto check;
	if (pta->suspended[prio].count == 0) {
		pthread_spin_unlock(&pta->suspended[prio].lock);
		return NULL;
	}
	task = pta->suspended[prio].head;
	if ((pta->suspended[prio].head = task->next) == NULL)
		pta->suspended[prio].tail = NULL;
	task->next = NULL;
	pta->suspended[prio].count--;
#ifdef BITMAP
	if (pta->suspended[prio].count == 0) {
		byte = GET_BYTE_NUM(prio);
		bit = GET_BIT_NUM(prio);
		do {
			bitmap_part = pta->s_bitmap[byte];
			bitmap_part_set = RESET_BIT(bitmap_part, bit);
		} while (!B_CAS(&pta->s_bitmap[byte], bitmap_part, bitmap_part_set));
	}
#endif
	pthread_spin_unlock(&pta->suspended[prio].lock);
	return task;
}

static inline task_t* try_remove_new_task(struct prio_task_array* pta, int prio) {
#ifdef BITMAP
	int byte, bit;
	byte_t bitmap_part, bitmap_part_set;
#endif
	task_t* task;
	if (pta->new[prio].count == 0)
		return NULL;
	if (pthread_spin_trylock(&pta->new[prio].lock))
		return NULL;
	if (pta->new[prio].count == 0) {
		pthread_spin_unlock(&pta->new[prio].lock);
		return NULL;
	}
	task = pta->new[prio].head;
	if ((pta->new[prio].head = task->next) == NULL)
		pta->new[prio].tail = NULL;
	task->next = NULL;
	pta->new[prio].count--;
#ifdef BITMAP
	if (pta->new[prio].count == 0) {
		byte = GET_BYTE_NUM(prio);
		bit = GET_BIT_NUM(prio);
		do {
			bitmap_part = pta->n_bitmap[byte];
			bitmap_part_set = RESET_BIT(bitmap_part, bit);
		} while (!B_CAS(&pta->n_bitmap[byte], bitmap_part, bitmap_part_set));
	}
#endif
	pthread_spin_unlock(&pta->new[prio].lock);
	return task;
}

static inline task_t* try_remove_suspended_task(struct prio_task_array* pta, int prio) {
#ifdef BITMAP
	int byte, bit;
	byte_t bitmap_part, bitmap_part_set;
#endif
	task_t* task;
	if (pta->suspended[prio].count == 0)
		return NULL;
	if (pthread_spin_trylock(&pta->suspended[prio].lock))
		return NULL;
	if (pta->suspended[prio].count == 0) {
		pthread_spin_unlock(&pta->suspended[prio].lock);
		return NULL;
	}
	task = pta->suspended[prio].head;
	if ((pta->suspended[prio].head = task->next) == NULL)
		pta->suspended[prio].tail = NULL;
	task->next = NULL;
	pta->suspended[prio].count--;
#ifdef BITMAP
	if (pta->suspended[prio].count == 0) {
		byte = GET_BYTE_NUM(prio);
		bit = GET_BIT_NUM(prio);
		do {
			bitmap_part = pta->s_bitmap[byte];
			bitmap_part_set = RESET_BIT(bitmap_part, bit);
		} while (!B_CAS(&pta->s_bitmap[byte], bitmap_part, bitmap_part_set));
	}
#endif
	pthread_spin_unlock(&pta->suspended[prio].lock);
	return task;
}

struct prio_task_array* GetPrioTaskArray(int num_prio) {
	int i;
#ifdef BITMAP
	int num_bytes;
#endif
	struct prio_task_array* pta;
	if (num_prio <= 0)
		goto error0;
	if ((pta = (struct prio_task_array*) malloc(sizeof(struct prio_task_array))) == NULL)
		goto error0;
	pta->num_prio = num_prio;
#ifdef BITMAP
	pta->num_bytes = (int) ((num_prio - 1) / 8) + 1;
	if ((pta->n_bitmap = (byte_t*) malloc(num_bytes * sizeof(byte_t))) == NULL)
		goto error1;
	memset(pta->n_bitmap, 0, num_bytes);if ((pta->s_bitmap = (byte_t*) malloc(num_bytes * sizeof(byte_t))) == NULL)
		goto error2;
	memset(pta->s_bitmap, 0, num_bytes);
#endif
	if ((pta->new = (task_list_t*) malloc(num_prio * sizeof(task_list_t))) == NULL)
		goto error3;
	if ((pta->suspended = (task_list_t*) malloc(num_prio * sizeof(task_list_t))) == NULL)
		goto error4;
	for (i=0; i<num_prio; i++) {
		if (task_list_init(&pta->new[i]) == -1)
			goto error5;
		if (task_list_init(&pta->suspended[i]) == -1)
			goto error5;
	}
	return pta;
error5:
	free(pta->suspended);
error4:
	free(pta->new);
error3:
#ifdef BITMAP
	free(pta->s_bitmap);
error2:
	free(pta->n_bitmap);
error1:
#endif
	free(pta);
error0:
	return NULL;
}

void FreePrioTaskArray(struct prio_task_array** ppta) {
	int i;
	struct prio_task_array* pta;
	if (ppta == NULL)
		return;
	if ((pta = (*ppta)) == NULL)
		return;
	(*ppta) = NULL;
	for (i=0; i<pta->num_prio; i++) {
		while (pta->new[i].count > 0) ;
		while (pthread_spin_destroy(&pta->new[i].lock) == EBUSY) ;
		while (pta->suspended[i].count > 0) ;
		while (pthread_spin_destroy(&pta->suspended[i].lock) == EBUSY) ;
	}
	free(pta->new);
	free(pta->suspended);
#ifdef BITMAP
	free(pta->n_bitmap);
	free(pta->s_bitmap);
#endif
	free(pta);
}

int InsertNewTask(struct prio_task_array* pta, task_t* task) {
#ifdef BITMAP
	int byte, bit;
	byte_t bitmap_part, bitmap_part_set;
#endif
	if (pta == NULL)
		return -1;
	if (task == NULL)
		return -1;
	if (task->prio < 0 || task->prio >= pta->num_prio)
		return -1;
	pthread_spin_lock(&pta->new[task->prio].lock);
	if (pta->new[task->prio].head == NULL)
		pta->new[task->prio].head = task;
	else
		pta->new[task->prio].tail->next = task;
	pta->new[task->prio].tail = task;
	pta->new[task->prio].count++;
#ifdef BITMAP
	if (pta->new[task->prio].count == 1) {
		byte = GET_BYTE_NUM(task->prio);
		bit = GET_BIT_NUM(task->prio);
		do {
			bitmap_part = pta->n_bitmap[byte];
			bitmap_part_set = SET_BIT(bitmap_part, bit);
		} while (!B_CAS(&pta->n_bitmap[byte], bitmap_part, bitmap_part_set));
	}
#endif
#ifdef TXS_STATS
#ifdef DROP_CONT_TIME
	clock_gettime(CLOCK_MONOTONIC_RAW, &task->enqueue);
#endif
#endif
	pthread_spin_unlock(&pta->new[task->prio].lock);
	return 0;
}

int InsertSuspendedTask(struct prio_task_array* pta, task_t* task, int incr_susp) {
#ifdef BITMAP
	int byte, bit;
	byte_t bitmap_part, bitmap_part_set;
#endif
	if (pta == NULL)
		return -1;
	if (task == NULL)
		return -1;
	if (task->prio < 0 || task->prio >= pta->num_prio)
		return -1;
	task->susp_prio = task->prio;
	if (incr_susp)
		task->num_susp++;
	pthread_spin_lock(&pta->suspended[task->prio].lock);
	if (pta->suspended[task->prio].head == NULL)
		pta->suspended[task->prio].head = task;
	else
		pta->suspended[task->prio].tail->next = task;
	pta->suspended[task->prio].tail = task;
	pta->suspended[task->prio].count++;
#ifdef BITMAP
	if (pta->suspended[task->prio].count == 1) {
		byte = GET_BYTE_NUM(task->prio);
		bit = GET_BIT_NUM(task->prio);
		do {
			bitmap_part = pta->s_bitmap[byte];
			bitmap_part_set = SET_BIT(bitmap_part, bit);
		} while (!B_CAS(&pta->s_bitmap[byte], bitmap_part, bitmap_part_set));
	}
#endif
	pthread_spin_unlock(&pta->suspended[task->prio].lock);
	return 0;
}

int InsertSuspendedTaskWithPrio(struct prio_task_array* pta, task_t* task, int susp_prio, int incr_susp) {
#ifdef BITMAP
	int byte, bit;
	byte_t bitmap_part, bitmap_part_set;
#endif
	if (pta == NULL)
		return -1;
	if (task == NULL)
		return -1;
	if (susp_prio < 0)
		susp_prio = 0;
	else if (susp_prio >= pta->num_prio)
		susp_prio = pta->num_prio-1;
	task->susp_prio = susp_prio;
	if (incr_susp)
		task->num_susp++;
	pthread_spin_lock(&pta->suspended[susp_prio].lock);
	if (pta->suspended[susp_prio].head == NULL)
		pta->suspended[susp_prio].head = task;
	else
		pta->suspended[susp_prio].tail->next = task;
	pta->suspended[susp_prio].tail = task;
	pta->suspended[susp_prio].count++;
#ifdef BITMAP
	if (pta->suspended[susp_prio].count == 1) {
		byte = GET_BYTE_NUM(susp_prio);
		bit = GET_BIT_NUM(susp_prio);
		do {
			bitmap_part = pta->s_bitmap[byte];
			bitmap_part_set = SET_BIT(bitmap_part, bit);
		} while (!B_CAS(&pta->s_bitmap[byte], bitmap_part, bitmap_part_set));
	}
#endif
	pthread_spin_unlock(&pta->suspended[susp_prio].lock);
	return 0;
}

task_t* RemoveNewTask(struct prio_task_array* pta, int prio) {
	if (pta == NULL)
		return NULL;
	if (prio < 0 || prio >= pta->num_prio)
		return NULL;
	return remove_new_task(pta, prio);
}

task_t* RemoveSuspendedTask(struct prio_task_array* pta, int prio) {
	if (pta == NULL)
		return NULL;
	if (prio < 0 || prio >= pta->num_prio)
		return NULL;
	return remove_suspended_task(pta, prio);
}

task_t* TryRemoveNewTask(struct prio_task_array* pta, int prio) {
	if (pta == NULL)
		return NULL;
	if (prio < 0 || prio >= pta->num_prio)
		return NULL;
	return try_remove_new_task(pta, prio);
}

task_t* TryRemoveSuspendedTask(struct prio_task_array* pta, int prio) {
	if (pta == NULL)
		return NULL;
	if (prio < 0 || prio >= pta->num_prio)
		return NULL;
	return try_remove_suspended_task(pta, prio);
}

task_t* GetHigherPrioTask(struct prio_task_array* pta, int prio) {
#ifdef BITMAP
	int b, t, byte, bit, b_shift;
#else
	int i;
#endif
	task_t* task;
	if (pta == NULL)
		return NULL;
	if (prio < 0 || prio >= pta->num_prio)
		return NULL;
	task = NULL;
#ifdef BITMAP
	byte = GET_BYTE_NUM(prio);
	bit = GET_BIT_NUM(prio);
	for (b=pta->num_bytes-1; b>byte; b--) {
		if (IS_EMPTY(pta->s_bitmap[b]) && IS_EMPTY(pta->n_bitmap[b]))
			continue;
		for (t=7; t>=0; t--) {
			if (CHECK_BIT(pta->s_bitmap[b],t))
				if ((task = try_remove_suspended_task(pta, GET_POS_NUM(b,t))) != NULL)
					return task;
			if (CHECK_BIT(pta->n_bitmap[b],t))
				if ((task = try_remove_new_task(pta, GET_POS_NUM(b,t))) != NULL)
					return task;
		}
	}
	b_shift = ONE_SHIFT(bit+1);
	if (BYTE_TO_INT(pta->s_bitmap[byte]) < b_shift &&
			BYTE_TO_INT(pta->n_bitmap[byte]) < b_shift)
		return NULL;
	for (t=7; t>bit; t--) {
		if (CHECK_BIT(pta->s_bitmap[byte],t))
			if ((task = try_remove_suspended_task(pta, GET_POS_NUM(byte,t))) != NULL)
				return task;
		if (CHECK_BIT(pta->n_bitmap[byte],t))
			if ((task = try_remove_new_task(pta, GET_POS_NUM(byte,t))) != NULL)
				return task;
	}
#else
	for (i=pta->num_prio-1; i>prio; i--) {
#ifdef TRY_REMOVE
		if ((task = try_remove_suspended_task(pta, i)) != NULL)
			break;
		if ((task = try_remove_new_task(pta, i)) != NULL)
			break;
#else
		if ((task = remove_suspended_task(pta, i)) != NULL)
			break;
		if ((task = remove_new_task(pta, i)) != NULL)
			break;
#endif
	}
#endif
	return task;
}

task_t* GetHighestPrioTask(struct prio_task_array* pta) {
#ifdef BITMAP
	int b, t;
#else
	int i;
#endif
	task_t* task;
	if (pta == NULL)
		return NULL;
	task = NULL;

#ifdef TXS_STATS
#ifdef DROP_CONT_TIME
	struct timespec before_start;
	clock_gettime(CLOCK_MONOTONIC_RAW, &before_start);
#endif
#endif

#ifdef BITMAP
	for (b=pta->num_bytes-1; b>=0; b--) {
		if (IS_EMPTY(pta->s_bitmap[b]) && IS_EMPTY(pta->n_bitmap[b]))
			continue;
		for (t=7; t>=0; t--) {
			if (CHECK_BIT(pta->s_bitmap[b],t))
				if ((task = try_remove_suspended_task(pta, GET_POS_NUM(b,t))) != NULL)
					goto end;
			if (CHECK_BIT(pta->n_bitmap[b],t))
				if ((task = try_remove_new_task(pta, GET_POS_NUM(b,t))) != NULL)
					goto end;
		}
	}
#else
	for (i=pta->num_prio-1; i>=0; i--) {
#ifdef TRY_REMOVE
		if ((task = try_remove_suspended_task(pta, i)) != NULL)
			break;
		if ((task = try_remove_new_task(pta, i)) != NULL)
			break;
#else
		if ((task = remove_suspended_task(pta, i)) != NULL)
			break;
		if ((task = remove_new_task(pta, i)) != NULL)
			break;
#endif
	}
#endif
end:

#ifdef TXS_STATS
#ifdef DROP_CONT_TIME
	if (task != NULL) {
		task->before_start = before_start;
	}
#endif
#endif
	
	return task;
}
