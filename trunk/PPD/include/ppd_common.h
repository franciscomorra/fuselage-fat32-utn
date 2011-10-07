/*
 * ppd_common.h
 *
 *  Created on: 07/10/2011
 *      Author: utn_so
 */

#ifndef PPD_COMMON_H_
#define PPD_COMMON_H_

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



#endif /* PPD_COMMON_H_ */
