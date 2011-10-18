/*
 * ppd_RequestList.h
 *
 *  Created on: Oct 1, 2011
 *      Author: utn_so
 */

#ifndef PPD_REQUESTLIST_H_
#define PPD_REQUESTLIST_H_

#include "ppd_common.h"

uint32_t SSTF_addRequest(requestNode_t* new);

uint32_t SSTF_near(requestNode_t* A,requestNode_t* B,requestNode_t* C);

uint32_t SSTF_sectorDist(uint32_t fstSector, uint32_t lstSector);

uint32_t SSTF_main();

#endif /* PPD_REQUESTLIST_H_ */
