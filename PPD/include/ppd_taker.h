/*
 * ppd_taker.h
 *
 *  Created on: 07/10/2011
 *      Author: utn_so
 */

#ifndef PPD_TAKER_H_
#define PPD_TAKER_H_

uint32_t TAKER_main(); //definido en el ppd_main por un tema de threads

nipcMsg_t TAKER_getRequest(requestNode_t* first);

uint32_t TAKER_turnToSectorNum(requestNode_t* CHSnode);

#endif /* PPD_TAKER_H_ */

