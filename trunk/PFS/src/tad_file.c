/*
 * tad_file.c
 *
 *  Created on: 04/10/2011
 *      Author: utn_so
 */

#include "tad_file.h"
#include "pfs_fat32.h"
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

extern queue_t opened_files;
extern uint32_t cache_size_inBytes;

void FILE_free(fat32file_t *file)
{
	free(file->long_file_name);
	//free(file);
}


void FILE_freeQueue(queue_t* file_queue)
{
	queueNode_t *cur_file_node;
	fat32file_t *cur_file;
	while((cur_file_node = QUEUE_takeNode(file_queue)) != NULL)
	{
		cur_file = (fat32file_t*) cur_file_node->data;
		free(cur_file->long_file_name);
		free(cur_file);
		free(cur_file_node);
	}
}

void FILE_splitNameFromPath(const char *path,char **ret_filename,char **ret_path_to_filename)
{

	char *aux = path + (strlen(path) - 1);
	uint32_t char_count = 0;

	while (*aux != '/')
	{
		char_count++;
		aux--;
	}

	size_t filename_size = char_count;
	size_t path_size = strlen(path) - char_count;

	*ret_filename = malloc(filename_size+1);
	memset(*ret_filename,0,filename_size+1);
	memcpy(*ret_filename,path+path_size,filename_size);


	//Reservo la cantidad necesaria calculada +1 por el caracter '\0'
	*ret_path_to_filename = malloc(path_size+1);
	memset(*ret_path_to_filename,0,path_size+1);
	if (path_size != 1)
	{
		memcpy(*ret_path_to_filename,path,path_size-1);
	}
	else
	{
		memset(*ret_path_to_filename,'/',1);
	}
	return;
}

opened_file_t* OFILE_get(const char* path)
{

	queueNode_t *opened_file_node = opened_files.begin;
	while (opened_file_node != NULL)
	{
		opened_file_t *opened_file = (opened_file_t*) opened_file_node->data;

		if (strcmp(opened_file->path,path) == 0)
		{
			return opened_file;
		}
		opened_file_node = opened_file_node->next;
	}

	fat32file_t *file_entry = fat32_getFileEntry(path);
	opened_file_t *new = OFILE_add_new(path,*file_entry);
	FILE_free(file_entry);
	return new;
}

opened_file_t* OFILE_add_new(char* path,fat32file_t file_entry)
{
	opened_file_t *new_opened_file = malloc(sizeof(opened_file_t));
	new_opened_file->path = malloc(strlen(path));
	strcpy(new_opened_file->path,path);
	new_opened_file->file_entry = file_entry;
	new_opened_file->cache.begin = new_opened_file->cache.end	= NULL;
	new_opened_file->cache_size = cache_size_inBytes;
	pthread_mutex_init(&new_opened_file->write_mutex,NULL);
	new_opened_file->open_count = 0;
	QUEUE_appendNode(&opened_files,new_opened_file);
	return new_opened_file;
}
