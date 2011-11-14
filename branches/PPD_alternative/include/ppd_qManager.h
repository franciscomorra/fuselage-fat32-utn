/*
 * ppd_qManager.h
 *
 *  Created on: Nov 2, 2011
 *      Author: utn_so
 */

#ifndef PPD_QMANAGER_H_
#define PPD_QMANAGER_H_

#include <stdint.h>

#include "tad_queue.h"
#include "ppd_common.h"

queue_t* QMANAGER_selectPassiveQueue(multiQueue_t*);

queue_t* QMANAGER_selectActiveQueue(multiQueue_t*);

conditionFunction_t QMANAGER_selectCondition(flag_t direction);

uint32_t QMANAGER_toggleQFlag(flag_t*,queue_t*);

uint32_t QMANAGER_toggleDirection(flag_t* direction);

#endif /* PPD_QMANAGER_H_ */
