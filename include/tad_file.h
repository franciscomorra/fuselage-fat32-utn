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
	queue_t lfn_entries;
	dirEntry_t* dir_entry;
} fat32file_t;

typedef struct fat32file_2_t
{
	char *long_file_name;
	bool deleted	:1;
	uint32_t cluster;
	uint32_t offset;
	lfnEntry_t lfn_entry;
	dirEntry_t dir_entry;
} fat32file_2_t;

fat32file_t* FILE_createStruct(char* filename,dirEntry_t *dirEntry);

void FILE_free(fat32file_2_t *file);

void FILE_freeQueue(queue_t* file_queue);

void FILE_splitNameFromPath(const char *path,char **ret_filename,char **ret_path_to_filename);

#endif /* TAD_FILE_H_ */
