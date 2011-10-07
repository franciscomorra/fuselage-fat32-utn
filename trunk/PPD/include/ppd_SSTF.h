/*
 * ppd_RequestList.h
 *
 *  Created on: Oct 1, 2011
 *      Author: utn_so
 */

#ifndef PPD_REQUESTLIST_H_
#define PPD_REQUESTLIST_H_

#include "ppd_common.h"

uint32_t SSTF_addRequest(uint32_t* sectorNum);

requestNode_t* SSTF_turnToCHS(uint32_t* sectorNum);

uint32_t SSTF_near(requestNode_t* A,requestNode_t* B,requestNode_t* C);

uint32_t SSTF_sectorDist(uint32_t fstSector, uint32_t lstSector);

#endif /* PPD_REQUESTLIST_H_ */
