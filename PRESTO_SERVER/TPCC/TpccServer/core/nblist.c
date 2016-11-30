/*
 *	core/nblist.c
 *
 *	@author:	Emiliano Silvestri
 *	@email:		emisilve86@gmail.com
 */

#include <stdlib.h>
#include "../arch/atomic.h"
#include "../datatypes/nblist.h"

int NBListInit(nb_list_t** nbl, unsigned int s) {
	/*
	 * 'NBListInit' allocates and initializes all the data
	 * structures needed to handle the non-blocking circular
	 * array list
	 *
	 * @param
	 * list: points to the 'nb_list_t' structure we want to initialize
	 * s: the array list size we are going to allocate
	 *
	 * @ret
	 * SUCCESS if no error occurred, E*** otherwise
	 */
	if (nbl == NULL)
		return EARGS;
	if (s < 1)
		return EARGS;
	if (((*nbl) = malloc(sizeof(nb_list_t))) == NULL)
		return ENOTMEM;
	(*nbl)->size = s;
	(*nbl)->head = 0;
	(*nbl)->tail = 0;
	if (((*nbl)->array = malloc(s * sizeof(nb_node_t))) == NULL)
		return ENOTMEM;
	int i;
	for (i=0; i<s; i++)
		(*nbl)->array[i].ptr = NULL;
	return SUCCESS;
}

int NBListDestroy(nb_list_t* nbl) {
	/*
	 * 'NBListDestroy' deallocates the non-blocking circular array
	 * list
	 *
	 * @param
	 * list: points to the 'nb_list_t' structure we want to destroy
	 *
	 * @ret
	 * SUCCESS if no error occurred, E*** otherwise
	 */
	if (nbl == NULL)
		return EARGS;
	nbl->size = 0;
	nbl->head = 0;
	nbl->tail = 0;
	free(nbl->array);
	free(nbl);
	return SUCCESS;
}

int NBListAdd(nb_list_t* list, void* p) {
	/*
	 * 'NBListAdd' tries to insert at the tail of the list the address
	 * of the structure passed as argument
	 *
	 * @param
	 * list: points to the 'nb_list_t' structure
	 * p: the structure address we want to insert
	 *
	 * @ret
	 * SUCCESS if no error occurred, EFULL if no empty nodes are available,
	 * E*** otherwise
	 */
	if (list == NULL)
		return EARGS;
	if (p == NULL)
		return EARGS;
	unsigned int pos;
	while (1) {
		if ((pos = (list->tail + 1) % list->size) == list->head)
			return EFULL;
		if (P_CAS(&list->array[pos].ptr, NULL, p)) {
			list->tail = pos;
			if (list->array[(pos = list->head)].ptr == NULL)
				I_CAS(&list->head, pos, ((pos + 1) % list->size));
			return SUCCESS;
		}
	}
}

int NBListRemove(nb_list_t* list, void** p) {
	/*
	 * 'NBListRemove' tries to remove from the head of the list the
	 * address of the structure inserted at that position
	 *
	 * @param
	 * list: points to the 'nb_list_t' structure
	 * p: the address of the 'void*' variable we want to fill
	 *
	 * @ret
	 * SUCCESS if no error occurred, EEMPTY if no nodes has been filled,
	 * E*** otherwise
	 */
	if (list == NULL)
		return EARGS;
	if (p == NULL)
		return EARGS;
	unsigned int pos;
	void* rmv;
	while (1) {
		if ((rmv = (void*) list->array[(pos = list->head)].ptr) == NULL) {
			if (pos == list->tail)
				return EEMPTY;
			else
				continue;
		}
		if (P_CAS(&list->array[pos].ptr, rmv, NULL)) {
			(*p) = rmv;
			if (pos != list->tail)
				I_CAS(&list->head, pos, ((pos + 1) % list->size));
			return SUCCESS;
		}
	}
}
