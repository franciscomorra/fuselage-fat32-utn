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
#include "tad_direntry.h"
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

/*char* DIRENTRY_getLongFileName(long_filename_entry lfn)
{

	char* str2 = malloc(sizeof(char)*50);
	char* str3 = malloc(sizeof(char)*50);
	size_t utf8_size;

	char* str1 = unicode_utf16_to_utf8((uint16_t*) &lfn.name_chars1,10,&utf8_size); // TODO
	str2 = unicode_utf16_to_utf8(&lfn.name_chars2,12,&utf8_size); // santi te queremos
	str3 = unicode_utf16_to_utf8(&lfn.name_chars3,4,&utf8_size);

		return strcat(str3,strcat(str1,str2));
}*/

char* DIRENTRY_getLongFileName(long_filename_entry lfn)
{
	char* str1 = malloc(sizeof(char)*50);
	char* str2 = malloc(sizeof(char)*50);
	char* str3 = malloc(sizeof(char)*50);
	size_t utf8_size;


	unicode_utf16_to_utf8_inbuffer((uint16_t*) &lfn.name_chars1,5,str1,&utf8_size); // TODO
	unicode_utf16_to_utf8_inbuffer((uint16_t*) &lfn.name_chars2,6,str2,&utf8_size); // averiguar por que anda con la mitad de los bytes originales
	unicode_utf16_to_utf8_inbuffer((uint16_t*) &lfn.name_chars3,2,str3,&utf8_size); // al armar un nombre corto te tira basura cuando lo mostras

	return strcat(strcat(str1,str2),str3);
}


file_node* DIRENTRY_getFileList(char* cluster_data)
{
	char *tmp;
	tmp = cluster_data;
	long_filename_entry *lfn_entry;
	char * lfn = malloc(50);
			memcpy(lfn_entry,tmp,32);
			memcpy(lfn,DIRENTRY_getLongFileName(*lfn_entry),32);
			printf("%s",lfn);
	return 0;
}



