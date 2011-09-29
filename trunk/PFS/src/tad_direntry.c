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

size_t DIRENTRY_getLongFileName(long_filename_entry lfn,char** ret_longfilename)
{
	size_t utf8_size;
	char* long_filename = malloc(26);
	*ret_longfilename = malloc(13);
	memcpy(long_filename,lfn.name_chars1,10);
	memcpy(long_filename+10,lfn.name_chars2,12);
	memcpy(long_filename+22,lfn.name_chars3,4);
	unicode_utf16_to_utf8_inbuffer((uint16_t*)long_filename,13,*ret_longfilename,&utf8_size);
	free(long_filename);
	return strlen(*ret_longfilename);
}


file_node* DIRENTRY_getFileList(char* cluster_data)
{
	//TODO HAY QE INVERTIR EL ORDEN EN QUE CONCATENA PORQUE LEE PRIMERO LA ULTIMA PARTE DEL NOMBRE
	long_filename_entry *lfn_entry = (long_filename_entry*) cluster_data;
	char* new_longfilename;
	char* longfilename_buf = malloc(255);
	memset(longfilename_buf,0,255);
	char* tmp_longfilename;
	size_t tmp_longfilename_size = 0;
	size_t new_longfilename_size = 0;


	while (lfn_entry != 0x00)
	{
		if (lfn_entry->sequence_no.number == 1)
		{
			tmp_longfilename_size = DIRENTRY_getLongFileName(*lfn_entry,&tmp_longfilename);
			new_longfilename_size += tmp_longfilename_size;
			memcpy(strchr(longfilename_buf,'\0'),tmp_longfilename,tmp_longfilename_size);

			new_longfilename = malloc(new_longfilename_size);
			memcpy(new_longfilename,longfilename_buf,new_longfilename_size);
			memset(longfilename_buf,0,255);

			free(tmp_longfilename);

			lfn_entry =  lfn_entry+(2);


			new_longfilename_size = 0;
		}
		else
		{
			tmp_longfilename_size = DIRENTRY_getLongFileName(*lfn_entry,&tmp_longfilename);
			new_longfilename_size += tmp_longfilename_size;
			memcpy(strchr(longfilename_buf,'\0'),tmp_longfilename,tmp_longfilename_size);

			lfn_entry++;
			free(tmp_longfilename);

		}
	}

}



