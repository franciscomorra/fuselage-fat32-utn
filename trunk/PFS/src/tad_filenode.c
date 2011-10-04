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

	if (first != NULL)
	{
		while (cur->next != NULL) {
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
	if (tmp != NULL) *first = (*first)->next;
	return tmp;
}

void FILENODE_addNode(fileNode_t **first,fileNode_t **new_node)
{

			assert(*new_node != NULL);

			if (*first == NULL) {
				*first =  *new_node;
			} else if ((*first)->next != NULL){
				fileNode_t *cur = (*first);
				while (cur->next != NULL)
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
			assert(dirEntry != NULL);
			fileNode_t *new_file = malloc(sizeof(fileNode_t));
			new_file->long_file_name = malloc(strlen(filename)+1);
			memset(new_file->long_file_name, 0, strlen(filename) + 1); // Seteo a 0
			strcpy(new_file->long_file_name, filename);
			memcpy(&(new_file->dir_entry), dirEntry, sizeof(dirEntry_t));
			new_file->next = NULL;
			return new_file;
}

fileNode_t* FILENODE_searchNode(char* filename, fileNode_t *list)
{
	if ( list != NULL)
	{
		fileNode_t *cur = list;
		while (cur != NULL)
		{
			if (strcmp(cur->long_file_name,filename) == 0)
			{
				return cur;
			}
					cur = cur->next;
		}
		return NULL;
	 }
	return NULL;
}
