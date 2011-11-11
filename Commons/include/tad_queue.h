/*
 * tad_list.h
 *
 *  Created on: 04/10/2011
 *      Author: utn_so
 */

#ifndef TAD_QUEUE_H_
#define TAD_QUEUE_H_

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
void QUEUE_appendNode(queue_t*,void*);
queueNode_t* QUEUE_takeNode(queue_t*);
queueNode_t* QUEUE_searchNode(queue_t*,void*,size_t);
queueNode_t* QUEUE_createNode(void*);
void QUEUE_destroyQueue(queue_t*,uint32_t);
void QUEUE_freeNode(queueNode_t *);
void QUEUE_freeByType(void*,uint32_t);
uint32_t QUEUE_length(queue_t *);
void QUEUE_cleanQueue(queue_t*,uint32_t);
#endif /* TAD_QUEUE_H_ */
