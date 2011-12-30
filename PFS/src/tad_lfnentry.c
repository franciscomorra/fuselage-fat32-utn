/*
 * tad_lfnentry.c
 *
 *  Created on: 30/09/2011
 *      Author: utn_so
 */
#include "tad_lfnentry.h"
#include <stdlib.h>
#include <string.h>
#include "utils.h"

size_t LFNENTRY_getString(lfnEntry_t lfn,char* ret_longfilename) {
	size_t utf8_size;
	char long_filename[26];
	memset(ret_longfilename,0,13);
	//*ret_longfilename = malloc(13);
	memcpy(long_filename, lfn.name_chars1, 10); // Junto las tres partes en un buffer primero
	memcpy(long_filename + 10, lfn.name_chars2, 12); // y despues las voy a pasar a UTF8
	memcpy(long_filename + 22, lfn.name_chars3, 4);
	unicode_utf16_to_utf8_inbuffer((uint16_t*) long_filename, 13,ret_longfilename, &utf8_size);
	//free(long_filename);
	return strlen(ret_longfilename);
}

lfnEntry_t LFNENTRY_create(char* filename)
{
	lfnEntry_t new_lfn;

	memset(&new_lfn,0,sizeof(lfnEntry_t));
	*((uint16_t*) new_lfn.first_cluster) = 0;
	new_lfn.attr = 0x0F;
	new_lfn.reserved = 0x00;
	new_lfn.sequence_no.deleted = false;
	new_lfn.sequence_no.last = true;
	new_lfn.sequence_no.number = 1;
	LFNENTRY_setNameChars(&new_lfn,filename);
	return new_lfn;
}

void LFNENTRY_setNameChars(lfnEntry_t *entry,char* filename)
{
	size_t utf16_size;
	char* str_from = malloc(13);
	char* str_to = malloc(26);
	memset(str_from,0,13);
	strcpy(str_from,filename);

	unicode_utf8_to_utf16_inbuffer(str_from, 13,(uint16_t*) str_to, &utf16_size);
	memcpy(entry->name_chars1,str_to,10);
	memcpy(entry->name_chars2,str_to+10,12);
	memcpy(entry->name_chars3,str_to+22,4);
	free(str_from);
	free(str_to);
	return;
}
