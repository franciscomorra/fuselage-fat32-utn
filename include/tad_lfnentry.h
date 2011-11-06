/*
 * tad_lfnentry.h
 *
 *  Created on: 30/09/2011
 *      Author: utn_so
 */

#ifndef TAD_LFNENTRY_H_
#define TAD_LFNENTRY_H_

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "tad_queue.h"

typedef struct lfn_sequence_number
{
	uint32_t number 	:6; //lo dimos vuelta porque los bits last y del son los de mas peso ( LO DI VUELTA DE NUEVO, ACUERDENSE LO DE LITTLE ENDIAN!!! Y SOBRE TODO PRUEBEN SIEMPRE)
	bool last 			:1;
	bool deleted 	 	:1;
}__attribute__((__packed__)) lfn_sequence_number;

typedef struct  LFN_ENTRY
{
	lfn_sequence_number sequence_no;
	char name_chars1[10];
	char attr;
	char reserved;
	char checksum;
	char name_chars2[12];
	char first_cluster[2];
	char name_chars3[4];

} __attribute__((__packed__)) lfnEntry_t;

size_t LFNENTRY_getString(lfnEntry_t lfn,char* ret_longfilename);

char* LFNENTRY_getLFN(queue_t lfn_entries);

lfnEntry_t LFNENTRY_create(char* filename);

void LFNENTRY_setNameChars(lfnEntry_t *entry,char* filename);
#endif /* TAD_LFNENTRY_H_ */
