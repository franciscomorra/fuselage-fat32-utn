/*
 * ppd_common.h
 *
 *  Created on: 07/10/2011
 *      Author: utn_so
 */

#ifndef PPD_COMMON_H_
#define PPD_COMMON_H_

#include "nipc.h"

extern uint32_t Head;
extern uint32_t Sector;

typedef struct requestNode_t {
	NIPC_type type;
	uint32_t cylinder;
	uint32_t head;
	uint32_t sector;
	uint32_t len;
	char* payload;
	char* sender;
	struct requestNode_t* next;
} __attribute__((__packed__)) requestNode_t;

// cambia de sectorNum a CHS para luego ser metido en la lista grande
void COMMON_turnToCHS(uint32_t,requestNode_t*);

#endif /* PPD_COMMON_H_ */
