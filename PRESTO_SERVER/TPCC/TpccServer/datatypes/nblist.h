/*
 *	core/nblist.h
 *
 *	@author:	Emiliano Silvestri
 *	@email:		emisilve86@gmail.com
 */

#pragma once

/* Insert and Remove return values */
#define		SUCCESS		0
#define		EARGS		1
#define		ENOTMEM		2
#define		EFULL		3
#define		EEMPTY		4

/*
 * The two following structures are used to implement
 * a non-blocking circular array list where each node
 * maintains a pointer to the structure just inserted
 * or NULL when removed.
 */
typedef struct {
	volatile void*	ptr;
} nb_node_t;

typedef struct {
	unsigned int	size;
	unsigned int	head;
	unsigned int	tail;
	nb_node_t*		array;
} nb_list_t;

/* Prototypes for the functions declared in "connection.c" */
int		NBListInit(nb_list_t**, unsigned int);
int		NBListDestroy(nb_list_t*);
int		NBListAdd(nb_list_t*, void*);
int		NBListRemove(nb_list_t*, void**);
