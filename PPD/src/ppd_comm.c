
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <semaphore.h>
#include "nipc.h"
#include "ppd_SSTF.h"
#include "ppd_comm.h"
#include "tad_queue.h"


extern queue_t* queue;
extern sem_t queueElemSem;
extern sem_t mainMutex;

uint32_t ppd_send(nipcMsg_t msg)
{

	return 1;
}

uint32_t ppd_receive(nipcMsg_t msgIn )
{
	/* Se obtienen los distintos campos del mensaje IPC*/

	uint32_t type = 0;
	if (msgIn.type == HANDSHAKE)
	{

	} else {
		uint32_t a;
		memcpy(&a,msgIn.payload,4);
		requestNode_t* request = COMMON_turnToCHS(a);
		queueNode_t* queueNode;

		request->type = msgIn.type;

		memcpy(&a,msgIn.len,2);
		request->len = a-4;

		request->payload = malloc(request->len);
		memcpy(request->payload,msgIn.payload + 4,request->len);
		//TODO sender

		queueNode = QUEUE_createNode(request);
		sem_wait(&mainMutex);
		QUEUE_appendNode(queue,queueNode);
		sem_post(&mainMutex);
		//agregar tambien el mutex de la consola
		sem_post(&queueElemSem);
	}
	return 0;
}
