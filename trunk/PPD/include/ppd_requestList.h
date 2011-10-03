/*
 * ppd_RequestList.h
 *
 *  Created on: Oct 1, 2011
 *      Author: utn_so
 */

#ifndef PPD_REQUESTLIST_H_
#define PPD_REQUESTLIST_H_

#include "nipc.h"

typedef struct requestNode_t {
	NIPC_type type;
	uint32_t cylinder;
	uint32_t head;
	uint32_t sector;
	char* payload;
	char* sender;
	struct requestNode_t* next;
} __attribute__((__packed__)) requestNode_t;


uint32_t REQUEST_add(uint32_t* sectorNum);

requestNode_t* REQUEST_turnToCHS(uint32_t* sectorNum);

uint32_t REQUEST_near(requestNode_t* A,requestNode_t* B,requestNode_t* C);

uint32_t REQUEST_sectorDist(uint32_t fstSector, uint32_t lstSector);

#endif /* PPD_REQUESTLIST_H_ */
