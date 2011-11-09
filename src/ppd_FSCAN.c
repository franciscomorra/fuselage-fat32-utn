#include <stdint.h>

#include "ppd_common.h"
#include "tad_queue.h"
#include "ppd_taker.h"
#include "ppd_qManager.h"

extern multiQueue_t*  multiQueue;

uint32_t FSCAN_getNext(queue_t* queue,queueNode_t** prevCandidate){
	uint32_t takerReturn;
	conditionFunction_t condition;

	condition = QMANAGER_selectCondition(multiQueue);
	takerReturn = TAKER_getNextNode(queue,prevCandidate,condition);
	if(takerReturn == 0){
		QMANAGER_toggleDirection(&multiQueue->direction);
		condition = QMANAGER_selectCondition(multiQueue);
		takerReturn = TAKER_getNextNode(queue,prevCandidate,condition);
	}

	return takerReturn;
}
