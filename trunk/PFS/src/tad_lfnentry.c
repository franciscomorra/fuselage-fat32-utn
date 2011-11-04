/*
 * tad_lfnentry.c
 *
 *  Created on: 30/09/2011
 *      Author: utn_so
 */
#include "tad_lfnentry.h"
#include <stdlib.h>
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


char* LFNENTRY_getLFN(queue_t lfn_entries)
{
	char *longfilename_buf = malloc(255);
	memset(longfilename_buf,0,255);

	char *tmp_longfilename_part = malloc(13);


	uint32_t tmp_longfilename_part_size = 0;
	uint32_t new_longfilename_size = 0;

	queueNode_t *lfn_node = (queueNode_t*) lfn_entries.begin;
	lfnEntry_t* lfn;
	while (lfn_node != NULL)
	{
		lfn = (lfnEntry_t*) lfn_node->data;
		tmp_longfilename_part_size = LFNENTRY_getString(*lfn, tmp_longfilename_part);
		new_longfilename_size += tmp_longfilename_part_size;

		shiftbytes_right(longfilename_buf, 255, tmp_longfilename_part_size);
		memcpy(longfilename_buf, tmp_longfilename_part,	tmp_longfilename_part_size);

		lfn_node = lfn_node->next;
	}

	char *ret_longfilename = malloc(new_longfilename_size+1);
	memset(ret_longfilename,0,new_longfilename_size+1);
	memcpy(ret_longfilename,longfilename_buf,new_longfilename_size);
	free(longfilename_buf);
	free(tmp_longfilename_part);
	return ret_longfilename;

}

lfnEntry_t LFNENTRY_create(char* filename)
{
	lfnEntry_t new_lfn;
	size_t utf16_size;
	memset(&new_lfn,0,sizeof(lfnEntry_t));
	*((uint16_t*) new_lfn.first_cluster) = 0;
	new_lfn.attr = 0x0F;
	new_lfn.reserved = 0x00;
	new_lfn.sequence_no.deleted = false;
	new_lfn.sequence_no.last = true;
	new_lfn.sequence_no.number = 1;
	char* str_from = malloc(13);
	char* str_to = malloc(26);
	memset(str_from,0,13);
	strcpy(str_from,filename);

	unicode_utf8_to_utf16_inbuffer(str_from, 13,(uint16_t*) str_to, &utf16_size);
	memcpy(new_lfn.name_chars1,str_to,10);
	memcpy(new_lfn.name_chars2,str_to+10,12);
	memcpy(new_lfn.name_chars3,str_to+12,4);
	/*unicode_utf8_to_utf16_inbuffer(str_buf+5, 6,(uint16_t*) new_lfn.name_chars2, &utf16_size);
	unicode_utf8_to_utf16_inbuffer(str_buf+11, 2,(uint16_t*) new_lfn.name_chars3, &utf16_size);*/

	return new_lfn;
}
