/*
 * tad_filenode.c
 *
 *  Created on: 30/09/2011
 *      Author: utn_so
 */
#include "tad_filenode.h"

void FILENODE_cleanList(FILE_NODE* first) {
	FILE_NODE* cur = first;
	FILE_NODE* next;

	while (cur->next != 0x0) {
		next = cur->next;
		free(cur->long_file_name);
		free(cur);
		cur = next;
	}

	free(cur->long_file_name);
	free(cur);
}

FILE_NODE* FILENODE_takeNode(FILE_NODE **first)
{
	FILE_NODE* tmp = *first;
	if (tmp != 0x0)	*first = (*first)->next;
	return tmp;
}
