/*
 * tad_list.c
 *
 *  Created on: 04/10/2011
 *      Author: utn_so
 */
#include "tad_list.h"
#include "tad_file.h"

#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include "log.h"

extern t_log* log_file;

void LIST_addNode(listNode_t **first,listNode_t **new_node)
{
	assert(*new_node != NULL);

			if (*first == NULL) {
				*first =  *new_node;
			}
			/*else if ((*first)->next != NULL)
			{

				listNode_t *cur = (*first);

				while (cur->next != NULL)
				{

					cur = cur->next;

				}

				cur->next = *new_node;
			}*/
			else
			{
				(*new_node)->next = *first;
				*first = *new_node;
				//(*first)->next = *new_node;
			}

}

listNode_t* LIST_removeNode(listNode_t **first)
{
	listNode_t* tmp = *first;
	if (tmp != NULL) *first = (*first)->next;
	return tmp;
}

void LIST_destroyList(listNode_t **first,uint32_t var_type)
{
	listNode_t* cur = *first;

	if (*first != NULL)
	{
		while (cur != NULL)
		{
			log_debug(log_file,"PFS","LIST_destroyList() -> LIST_freeByType(0x%x)",cur);

			LIST_freeByType(&cur,var_type);
			cur = cur->next;
		}
	}
}

void LIST_destroyNode(listNode_t **node,uint32_t var_type)
{
	assert(*node != NULL);
	log_debug(log_file,"PFS","LIST_destroyNode() -> LIST_freeByType(0x%x)",node);
	LIST_freeByType(node,var_type);
}

listNode_t* LIST_searchNode(listNode_t	**first,void *data,size_t dataLength)
{
	if (*first != NULL)
	{
		listNode_t *cur = *first;
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

listNode_t* LIST_createNode(void* data)
{
	assert(data!=NULL);
	listNode_t *new_node = malloc(sizeof(listNode_t));
	new_node->data=data;
	new_node->next = NULL;

	return new_node;
}

void LIST_freeByType(void** pointer,uint32_t var_type)
{

	switch(var_type)
	{
		case FAT32FILE_T:;
			fat32file_t *casted_pointer = (fat32file_t*) *pointer;
			free(casted_pointer->long_file_name);
			free(casted_pointer);
		break;

		default:
			free(*pointer);
		break;
	}


}

uint32_t LIST_listSize(listNode_t **first)
{
	uint32_t counter = 1;
	listNode_t *cur = *first;
	while (cur->next != NULL)
	{
		cur = cur->next;
		++counter;
	}
	return ++counter;
}
