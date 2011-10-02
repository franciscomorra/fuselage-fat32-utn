/*
 * ppd_RequestList.h
 *
 *  Created on: Oct 1, 2011
 *      Author: utn_so
 */

#ifndef PPD_REQUESTLIST_H_
#define PPD_REQUESTLIST_H_

typedef struct requestNode_t {
	uint32_t cylinder;
	uint32_t head;
	uint32_t sector;
	char* payload;
	struct requestNode_t* next;
} __attribute__((__packed__)) requestNode_t;


uint32_t REQUEST_add(uint32_t* sectorNum);

requestNode_t* REQUEST_turnToCHS(uint32_t* sectorNum);

uint32_t near(requestNode_t* A,requestNode_t* B,requestNode_t* C);

#endif /* PPD_REQUESTLIST_H_ */
