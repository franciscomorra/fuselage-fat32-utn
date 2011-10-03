/*
 * tad_filenode.c
 *
 *  Created on: 30/09/2011
 *      Author: utn_so
 */
#include "tad_filenode.h"
#include <assert.h>
#include "log.h"

void FILENODE_cleanList(fileNode_t* first) {
	fileNode_t* cur = first;
	fileNode_t* next;

	if (first != 0x0)
	{
		while (cur->next != 0x0) {
			next = cur->next;
			free(cur->long_file_name);
			free(cur);
			cur = next;
		}
		free(cur->long_file_name);
		free(cur);
	}


}

fileNode_t* FILENODE_takeNode(fileNode_t **first)
{
	fileNode_t* tmp = *first;
	if (tmp != 0x0) *first = (*first)->next;
	return tmp;
}

void FILENODE_addNode(fileNode_t **first,fileNode_t **new_node)
{

			assert(*new_node != 0x0);

			if (*first == 0x0) {
				*first =  *new_node;
			} else if ((*first)->next != 0x0){
				fileNode_t *cur = (*first);
				while (cur->next != 0x0)
				{
					cur = cur->next;
				}
				cur->next = *new_node;
			}
			else
			{
				(*first)->next = *new_node;
			}

}

fileNode_t* FILENODE_createNode(char* filename,dirEntry_t *dirEntry)
{
			assert(dirEntry != 0x0);
			fileNode_t *new_file = malloc(sizeof(fileNode_t));
			new_file->long_file_name = malloc(strlen(filename)+1);
			memset(new_file->long_file_name, 0, strlen(filename) + 1); // Seteo a 0
			strcpy(new_file->long_file_name, filename);
			memcpy(&(new_file->dir_entry), dirEntry, sizeof(dirEntry_t));
			new_file->next = 0x0;
			return new_file;
}
