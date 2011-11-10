/*
 * ppd_FSCAN.h
 *
 *  Created on: Nov 9, 2011
 *      Author: utn_so
 */

#ifndef PPD_FSCAN_H_
#define PPD_FSCAN_H_

#include <stdint.h>

#include "tad_queue.h"

uint32_t FSCAN_getNext(queue_t* queue,queueNode_t** prevCandidate);

uint32_t FSCAN_moveHeadPos(flag_t direction);

uint32_t FSCAN_moveToCylinder(uint32_t destCylinder);


#endif /* PPD_FSCAN_H_ */
