/*
 * tad_file.h
 *
 *  Created on: 04/10/2011
 *      Author: utn_so
 */

#ifndef TAD_FILE_H_
#define TAD_FILE_H_

#include "tad_direntry.h"
#include "tad_lfnentry.h"
#include "tad_queue.h"
#include <stdbool.h>


typedef struct fat32file_t
{
	char *long_file_name;
	bool deleted	:1;
	bool has_lfn;
	uint32_t cluster;
	uint32_t offset;
	lfnEntry_t lfn_entry;
	dirEntry_t dir_entry;
} fat32file_t;


typedef struct opened_file_t
{
	char* path;
	fat32file_t file_entry;
	queue_t cache;
	size_t cache_size;
	uint32_t open_count;
	pthread_mutex_t write_mutex;

} opened_file_t;



void FILE_free(fat32file_t *file);

void FILE_freeQueue(queue_t* file_queue);

void FILE_splitNameFromPath(const char *path,char **ret_filename,char **ret_path_to_filename);

opened_file_t* OFILE_get(const char* path);

opened_file_t* OFILE_add_new(char* path,fat32file_t file_entry);

#endif /* TAD_FILE_H_ */
