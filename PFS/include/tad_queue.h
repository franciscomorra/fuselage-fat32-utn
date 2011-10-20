/*
 * tad_list.h
 *
 *  Created on: 04/10/2011
 *      Author: utn_so
 */

#ifndef TAD_LIST_H_
#define TAD_LIST_H_

#include <stdint.h>
#include <stdlib.h>

#define DIRENTRY_T 1
#define FAT32FILE_T 2
#define UINT32_T 3

typedef struct queueNode_t
{
	void	*data;
	struct queueNode_t *next;
} queueNode_t ;

typedef struct queue_t
{
	queueNode_t* begin;
	queueNode_t* end;

} queue_t;

void QUEUE_initialize(queue_t* );
void QUEUE_addNode(queue_t*,queueNode_t*);
queueNode_t* QUEUE_removeFromBegin(queue_t*);
queueNode_t* QUEUE_searchNode(queue_t*,void*,size_t);
queueNode_t* QUEUE_createNode(void*);
void QUEUE_destroy(queue_t*,uint32_t);
void QUEUE_destroyNode(queueNode_t *,uint32_t );
void QUEUE_freeByType(void**,uint32_t);
uint32_t QUEUE_length(queue_t *);
#endif /* TAD_LIST_H_ */
