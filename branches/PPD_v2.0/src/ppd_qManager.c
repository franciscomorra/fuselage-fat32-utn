#include <stdlib.h>
#include "tad_queue.h"
#include "ppd_qManager.h"


queue_t* QMANAGER_selectPassiveQueue(multiQueue_t* multiQueue){
	if(multiQueue->flag == QUEUE2_ACTIVE || multiQueue->flag == SSTF)
		return multiQueue->queue1;
	else
		return multiQueue->queue2;
}

queue_t* QMANAGER_selectActiveQueue(multiQueue_t* multiQueue){
	switch(multiQueue->flag){
		case SSTF:
			return multiQueue->queue1;

		case QUEUE1_ACTIVE:
			if(QMANAGER_toggleFlag(&multiQueue->flag, multiQueue->queue1)==1)
				return multiQueue->queue2;
			else
				return multiQueue->queue1;

		case QUEUE2_ACTIVE:
			if(QMANAGER_toggleFlag(&multiQueue->flag, multiQueue->queue2)==1)
				return multiQueue->queue1;
			else
				return multiQueue->queue2;
	}
}

uint32_t QMANAGER_toggleFlag(flag_t* flag,queue_t* queue){
	if(queue->begin == NULL){
		if(*flag == QUEUE1_ACTIVE)
			*flag = QUEUE2_ACTIVE;
		else
			*flag = QUEUE1_ACTIVE;
		return 1;
	} else
		return 0;
}
