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
	char* str1 = malloc(50);
	char* str2 = malloc(sizeof(char)*50);
	char* str3 = malloc(sizeof(char)*50);

		str1 = unicode_utf16_to_utf8(lfn.name_chars1,10,5); // aca rompe todo (L)
		str2 = unicode_utf16_to_utf8(lfn.name_chars2,12,6); // santi te queremos
		str3 = unicode_utf16_to_utf8(lfn.name_chars3,4,2);

		return strcat(str3,strcat(str1,str2));
}

file_node* DIRENTRY_getFileList(char* cluster_data)
{
	char *tmp;
	tmp = cluster_data + 32;
	long_filename_entry *lfn_entry;
	char * lfn = malloc(50);
			memcpy(lfn_entry,tmp,32);
			memcpy(lfn,DIRENTRY_getLongFileName(*lfn_entry),32);
			printf("%s",lfn);
	return 0;
}



