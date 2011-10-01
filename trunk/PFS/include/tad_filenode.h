/*
 * tad_filenode.h
 *
 *  Created on: 30/09/2011
 *      Author: utn_so
 */

#ifndef TAD_FILENODE_H_
#define TAD_FILENODE_H_
#include "tad_direntry.h"

typedef struct FILE_NODE
{
	char *long_file_name;
	DIR_ENTRY dir_entry;
	struct FILE_NODE * next;
}__attribute__((__packed__)) FILE_NODE;

FILE_NODE* FILENODE_takeNode(FILE_NODE **first);

void FILENODE_cleanList(FILE_NODE* first);

#endif /* TAD_FILENODE_H_ */
