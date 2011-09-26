/*
 * dir_entry.c
 *
 *  Created on: 16/09/2011
 *      Author: utn_so
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dir_entry.h"
#include "utils.h"


uint32_t DIRENTRY_getClusterNumber(directory_entry *entry)
{
	unsigned char cluster[4] = {entry->high_cluster[1],entry->high_cluster[0],entry->low_cluster[1],entry->low_cluster[0]};
	int i=0;
	uint32_t arrToInt=0;
	for(i=0;i<4;i++)
	arrToInt =(arrToInt<<8) | cluster[i];
	return arrToInt;
}

char* DIRENTRY_getLongFileName(long_filename_entry lfn)
{
	uint16_t* src_utf16;

	src_utf16 =	strcat(lfn.name_chars3,strcat(lfn.name_chars1,lfn.name_chars2));

	return unicode_utf16_to_utf8(const uint16_t *src_utf16, const size_t src_utf16size, size_t *dest_utf8size)
}

file_node* DIRENTRY_getFileList(char* cluster_data)
{
	char *tmp;
	tmp = cluster_data + 32;
	long_filename_entry *lfn_entry = 0x0;
	char * lfn = malloc(500);
	while (*tmp != 0x00){
			memcpy(lfn_entry,tmp,32);
			memcpy(lfn,DIRENTRY_getLongFileName(*lfn_entry),32);
			tmp += 32;
	}
	return 0;
}



