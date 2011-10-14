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

typedef struct listNode_t
{
	void	*data;
	struct listNode_t *next;
} listNode_t ;

typedef struct listLine_t
{
	listNode_t* begin;
	listNode_t* end;

} listLine_t;

void LIST_addNode(listLine_t**,listNode_t**);
listNode_t* LIST_removeFromBegin(listLine_t**);
listNode_t* LIST_searchNode(listLine_t**,void*,size_t);
listNode_t* LIST_createNode(void*);
void LIST_destroyList(listLine_t**,uint32_t);
void LIST_freeByType(void**,uint32_t);
#endif /* TAD_LIST_H_ */
