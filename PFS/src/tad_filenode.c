/*
 * tad_filenode.c
 *
 *  Created on: 30/09/2011
 *      Author: utn_so
 */
#include "tad_filenode.h"

void FILENODE_cleanList(fileNode_t* first) {
	fileNode_t* cur = first;
	fileNode_t* next;

	while (cur->next != 0x0) {
		next = cur->next;
		free(cur->long_file_name);
		free(cur);
		cur = next;
	}

	free(cur->long_file_name);
	free(cur);
}

fileNode_t* FILENODE_takeNode(fileNode_t **first)
{
	fileNode_t* tmp = *first;
	if (tmp != 0x0)	*first = (*first)->next;
	return tmp;
}
