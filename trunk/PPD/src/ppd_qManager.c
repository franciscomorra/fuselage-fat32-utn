#include <stdlib.h>
#include "tad_queue.h"
#include "ppd_qManager.h"


queue_t* QMANAGER_selectPassiveQueue(multiQueue_t* multiQueue){
	if(multiQueue->qflag == QUEUE2_ACTIVE || multiQueue->qflag == SSTF)
		return multiQueue->queue1;
	else
		return multiQueue->queue2;
}

queue_t* QMANAGER_selectActiveQueue(multiQueue_t* multiQueue){
	switch(multiQueue->qflag){
		case SSTF:
			return multiQueue->queue1;

		case QUEUE1_ACTIVE:
			if(QMANAGER_toggleQFlag(&multiQueue->qflag, multiQueue->queue1)==1)
				return multiQueue->queue2;
			else
				return multiQueue->queue1;

		case QUEUE2_ACTIVE:
			if(QMANAGER_toggleQFlag(&multiQueue->qflag, multiQueue->queue2)==1)
				return multiQueue->queue1;
			else
				return multiQueue->queue2;
	}
}

conditionFunction_t QMANAGER_selectCondition(flag_t direction){
	switch(direction){

		case UP: {
			return &COMMON_greaterThan;
			break;
		}
		case DOWN: {
			return &COMMON_lessThan;
			break;
		}
		case SSTF: {
			return &COMMON_identity;
			break;
		}
	}
}


uint32_t QMANAGER_toggleQFlag(flag_t* flag,queue_t* queue){
	if(queue->begin == NULL){
		if(*flag == QUEUE1_ACTIVE)
			*flag = QUEUE2_ACTIVE;
		else
			*flag = QUEUE1_ACTIVE;
		return 1;
	} else
		return 0;
}

uint32_t QMANAGER_toggleDirection(flag_t* direction){
	if(*direction == UP)
		*direction = DOWN;
	else
		*direction = UP;
	return 1;
}

uint32_t QMANAGER_freeRequests(queue_t* queue){
	queueNode_t* aux = queue->begin;

	while (aux != NULL){
		free(((request_t*)aux->data)->CHS);
		free(((request_t*)aux->data)->payload);
		aux = aux->next;
	}

	return 1;
}


