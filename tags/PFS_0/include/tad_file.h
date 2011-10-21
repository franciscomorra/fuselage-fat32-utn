/*
 * tad_file.h
 *
 *  Created on: 04/10/2011
 *      Author: utn_so
 */

#ifndef TAD_FILE_H_
#define TAD_FILE_H_

#include "tad_direntry.h"

typedef struct fat32file_t
{
	char *long_file_name;
	uint32_t clusterofEntry;
	size_t numberOfEntries;
	uint32_t index_firstEntry;
	dirEntry_t dir_entry;
} fat32file_t;

fat32file_t* FILE_createStruct(char* filename,dirEntry_t *dirEntry);

#endif /* TAD_FILE_H_ */
