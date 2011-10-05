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

extern t_log* log_file;

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
