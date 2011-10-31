/*
 * tad_file.c
 *
 *  Created on: 04/10/2011
 *      Author: utn_so
 */

#include "tad_file.h"
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include "log.h"


fat32file_t* FILE_createStruct(char* filename,dirEntry_t *dirEntry)
{
			assert(dirEntry != NULL);
			fat32file_t *new_file = malloc(sizeof(fat32file_t));
			new_file->long_file_name = malloc(strlen(filename)+1);
			memset(new_file->long_file_name, 0, strlen(filename) + 1); // Seteo a 0
			strcpy(new_file->long_file_name, filename);
			memcpy(&(new_file->dir_entry), dirEntry, sizeof(dirEntry_t));
			return new_file;
}

fat32file_t* FILE_createStruct2(char* filename,dirEntry_t *dirEntry)
{
			assert(dirEntry != NULL);
			fat32file_t *new_file = malloc(sizeof(fat32file_t));
			new_file->long_file_name = malloc(strlen(filename)+1);
			memset(new_file->long_file_name, 0, strlen(filename) + 1); // Seteo a 0
			strcpy(new_file->long_file_name, filename);
			new_file->dir_entry = dirEntry;
			return new_file;
}


void FILE_free(fat32file_t *file)
{
	free(file->long_file_name);
	queueNode_t *lfn_node;
	while ((lfn_node = QUEUE_takeNode(&file->lfn_entries)) != NULL)
	{
		free(lfn_node);
	}
	free(file);
}


void FILE_freeQueue(queue_t* file_queue)
{
	queueNode_t *cur_file_node;
	fat32file_t *cur_file;
	while((cur_file_node = QUEUE_takeNode(file_queue)) != NULL)
	{
		cur_file = (fat32file_t*) cur_file_node->data;
		FILE_free(cur_file);
		free(cur_file_node);
	}
}

