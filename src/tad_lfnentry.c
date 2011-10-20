/*
 * tad_lfnentry.c
 *
 *  Created on: 30/09/2011
 *      Author: utn_so
 */
#include "tad_lfnentry.h"
#include <stdlib.h>
#include "utils.h"

size_t LFNENTRY_getLongFileName(lfnEntry_t lfn,char* ret_longfilename) {
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
