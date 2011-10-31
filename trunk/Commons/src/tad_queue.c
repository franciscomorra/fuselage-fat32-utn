/*
 * tad_list.c
 *
 *  Created on: 04/10/2011
 *      Author: utn_so
 */
#include "tad_queue.h"
//#include "tad_file.h"
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include "log.h"

//extern t_log* log_file;
void QUEUE_initialize(queue_t* line)
{
	memset(line,0,sizeof(queue_t));
	//line = malloc(sizeof(queue_t));
	//(line)->begin = NULL;
	//(line)->end = NULL;
	return;
}
void QUEUE_appendNode(queue_t *line, void *data)
{

	assert(data!=NULL && line != NULL);
	queueNode_t *new_node = malloc(sizeof(queueNode_t));
	new_node->data=data;
	new_node->next = NULL;

	if (line->begin == NULL)
	{
		line->begin = line->end = new_node;

	}
	else
	{
		line->end->next = new_node;
		line->end = new_node;
	}
}

queueNode_t* QUEUE_takeNode(queue_t *line)
{
	queueNode_t* tmp = line->begin;
	if (tmp != NULL) line->begin = tmp->next;
	if (line->begin == NULL) line->end = line->begin;
	return tmp;
}

void QUEUE_destroyQueue(queue_t *line,uint32_t var_type)
{
	queueNode_t* cur = (line)->begin;

	if (cur != NULL)
	{
		while (cur != NULL)
		{
			QUEUE_freeNode(cur,var_type);
			cur = cur->next;
		}
	}
	free(line);
}

void QUEUE_freeNode(queueNode_t *node,uint32_t var_type)
{
	assert(node != NULL);
	free(node->data);
	free(node);
}

queueNode_t* QUEUE_searchNode(queue_t *line,void *data,size_t dataLength)
{
	if ((line)->begin != NULL)
	{
		queueNode_t *cur = (line)->begin;
		while (cur != NULL)
		{
				if (memcmp(cur->data,data,dataLength) == 0)
				{
					return cur;
				}
				cur = cur->next;
		}
		return NULL;
	 }
	return NULL;
}

void QUEUE_freeByType(void* pointer,uint32_t var_type)
{
	queueNode_t *toDestroy = (queueNode_t*) pointer;

	switch(var_type)
	{
		case FAT32FILE_T:;
			/*fat32file_t *casted_pointer = (fat32file_t*) *pointer;
			free(casted_pointer->long_file_name);
			free(casted_pointer);*/
		break;

		default:

		break;
	}

	free(toDestroy->data);
	free(pointer);

}

uint32_t QUEUE_length(queue_t *line)
{
	uint32_t counter = 0;
	queueNode_t *cur = (line)->begin;
	while (cur != NULL)
	{
		++counter;
		cur = cur->next;
	}
	return counter;
}


void QUEUE_cleanQueue(queue_t *line,uint32_t var_type)
{
	queueNode_t* cur = (line)->begin;

	if (line->begin != NULL)
	{
		while (line->begin != NULL)
		{
			cur = QUEUE_takeNode(line);
			// free(cur->data) me tira segmentation fault ayuda !!;
			free(cur);
		}
	}
}



