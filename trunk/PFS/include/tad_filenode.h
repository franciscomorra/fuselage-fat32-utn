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
	dirEntry_t dir_entry;
	struct FILE_NODE * next;
}__attribute__((__packed__)) fileNode_t;

fileNode_t* FILENODE_takeNode(fileNode_t **first);

void FILENODE_cleanList(fileNode_t* first);

#endif /* TAD_FILENODE_H_ */
