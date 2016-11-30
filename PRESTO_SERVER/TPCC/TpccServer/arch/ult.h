/**
*                       Copyright (C) 2008-2015 HPDCS Group
*                       http://www.dis.uniroma1.it/~hpdcs
*
* @file ult.h
* @brief The User-Level Thread module allows the creation/scheduling of a user-level thread
* 	in an architecture-dependent way.
* @author Alessandro Pellegrini
*/

#pragma once
//#pragma GCC poison setjmp longjmp

#include "jmp.h"
#include "../datatypes/task.h"

#define STACK_SIZE			2097152

/*
 * Save machine context for userspace context switch.
 * This is used only in initialization!
 */
#define	context_save(context) set_jmp(context)


/*
 * Restore machine context for userspace context switch.
 * This is used only in inizialitaion!
 */
#define	context_restore(context) long_jmp(context, 1)


/*
 * Swicth machine context for userspace context switch.
 * This is used to schedule a transaction or return control to platform context.
 */
#define	context_switch(context_old, context_new) \
			if (set_jmp(context_old) == 0) \
				long_jmp(context_new, (context_new)->rax)


/*
 * Allocate ULT stack.
 * This is used only in inizialitaion!
 */
void*	get_ult_stack(unsigned int lid, size_t size);

/*
 * Setup context and stack in such a way to start running at entry_point.
 */
void	context_create(exec_context_t* context, void (*entry_point)(void*), void* args, void* stack, size_t stack_size);


/* Execution context of a worker_thread out of any transaction context */
extern __thread exec_context_t			platform_context;
/* The task_t structure associated to the currently running transaction */
extern __thread task_t*					running_task;
