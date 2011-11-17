#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ppd_SSTF.h"
#include "ppd_common.h"
#include "ppd_taker.h"		//funcion getTimeSleep()
#include "tad_queue.h"
#include "ppd_qManager.h"

extern multiQueue_t* multiQueue;

uint32_t SSTF_getNext(queue_t* queue, queueNode_t** prevCandidate,uint32_t initialPosition){

	TAKER_getNextNode(queue,initialPosition,prevCandidate,QMANAGER_selectCondition(multiQueue->direction));

	return 0;

}

uint32_t SSTF_near(CHS_t* new,CHS_t* queueHead){

	if(TAKER_distanceTime(new)< TAKER_distanceTime(queueHead))
		return 1;

	return 0;

}
