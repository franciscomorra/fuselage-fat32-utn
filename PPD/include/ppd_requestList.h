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
} __attribute__((__packed__)) requestNode_t;


uint32_t REQUEST_add(char* sector);

requestNode_t* REQUEST_getCHS(char* sectorNum);

#endif /* PPD_REQUESTLIST_H_ */
