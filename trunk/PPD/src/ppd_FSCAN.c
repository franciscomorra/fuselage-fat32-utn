
#include <stdint.h>
#include <unistd.h>

#include "ppd_common.h"
#include "tad_queue.h"
#include "ppd_taker.h"
#include "ppd_qManager.h"
#include "ppd_FSCAN.h"

extern multiQueue_t*  multiQueue;
extern uint32_t Cylinder;
extern uint32_t headPosition;
extern uint32_t TrackJumpTime;

uint32_t FSCAN_getNext(queue_t* queue,queueNode_t** prevCandidate){
	uint32_t takerReturn;
	uint32_t delay = 0;

	takerReturn = TAKER_getNextNode(queue,prevCandidate,QMANAGER_selectCondition(multiQueue));
	if(takerReturn == 0){
		delay = FSCAN_moveHeadPos(multiQueue->direction);
		QMANAGER_toggleDirection(&multiQueue->direction);
		takerReturn = TAKER_getNextNode(queue,prevCandidate,QMANAGER_selectCondition(multiQueue));
	}

	return delay;
}


uint32_t FSCAN_moveHeadPos(flag_t direction){
	uint32_t delay;
	if(direction == UP)
		delay = FSCAN_moveToCylinder(Cylinder-1);			//Si esta en subida lo lleva al ultimo cilindro
	else
		delay = FSCAN_moveToCylinder(0);					//Si esta en bajada lo lleva al primer cilindro
	//sleep(delay/1000);
	return delay;
}


uint32_t FSCAN_moveToCylinder(uint32_t destCylinder){
	CHS_t* headPosCHS = COMMON_turnToCHS(headPosition);							//Obtiene el CHS del Head Position
	uint32_t delay = abs(headPosCHS->cylinder - destCylinder)*TrackJumpTime;	//Calcula el tiempo que tardara en llegar al cilindro
	uint32_t reachedSector = TAKER_reachedSector(destCylinder,headPosCHS);		//Calcula el sector en el que caera despues de moverse
	headPosCHS->cylinder = destCylinder;										//Modifica el cilindro para luego ser transformado a numero de sector
	headPosCHS->sector = reachedSector;											//Modifica el sector para luego ser transformado a numero de sector
	headPosition = TAKER_turnToSectorNum(headPosCHS);							//Devuelve el numero de sector que hay que asignarle al Head Position

	free(headPosCHS);
	return delay;																//Retorna el tiempo que tardo en moverse
}
