/*
 * ppd_common.h
 *
 *  Created on: 07/10/2011
 *      Author: utn_so
 */

#ifndef PPD_COMMON_H_
#define PPD_COMMON_H_

#include <semaphore.h>
#include "nipc.h"
#include "tad_queue.h"


//extern uint32_t Head;
//extern uint32_t Sector;


typedef struct CHS_t {
	uint32_t cylinder;
	uint32_t head;
	uint32_t sector;
} __attribute__((__packed__)) CHS_t;

typedef struct request_t {
	NIPC_type type;
	struct CHS_t* CHS;
	char len[2];
	uint32_t sender;
	char* payload;
} __attribute__((__packed__)) request_t;

typedef enum {
	QUEUE1_ACTIVE=0x00, QUEUE2_ACTIVE=0x01, SSTF=0x02, FSCAN=0x03
} flag_t;

typedef struct multiQueue_t {
	queue_t* queue1;
	queue_t* queue2;
	flag_t flag;
	sem_t queueElemSem;
} __attribute__((__packed__)) multiQueue_t;

// cambia de sectorNum a CHS para luego ser metido en la lista grande
CHS_t* COMMON_turnToCHS(uint32_t);

#endif /* PPD_COMMON_H_ */
