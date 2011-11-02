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

typedef struct CHS_t {
	uint32_t cylinder;
	uint32_t head;
	uint32_t sector;
} __attribute__((__packed__)) CHS_t;

typedef struct requestNode_t {
	NIPC_type type;
	struct CHS_t* CHS;
	char len[2];
	uint32_t sender;
	char* payload;
	struct requestNode_t* next;
} __attribute__((__packed__)) requestNode_t;

// cambia de sectorNum a CHS para luego ser metido en la lista grande
CHS_t* COMMON_turnToCHS(uint32_t);

#endif /* PPD_COMMON_H_ */
