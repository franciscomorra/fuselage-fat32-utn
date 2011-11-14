/*
 * ppd_taker.h
 *
 *  Created on: 07/10/2011
 *      Author: utn_so
 */


#ifndef PPD_TAKER_H_
#define PPD_TAKER_H_

#include "tad_queue.h"

// definida en el ppd_main por un tema de threads
void TAKER_main(uint32_t (*getNext)(queue_t*,queueNode_t**));

// atiende al pedido que el algoritmo saca de la cola
void TAKER_handleRequest(queue_t*,request_t*,uint32_t,uint32_t (*getNext)(queue_t*,queueNode_t**));

//saca el primer nodo de la cola y lo transforma de tipo queueNode a requestNode
request_t* TAKER_takeRequest(queue_t*,queueNode_t*,uint32_t*);

//Genera la informacion necesaria para imprimir datos del comando Trace
void TAKER_getTraceInfo(CHS_t* CHSrequest,uint32_t* distance,uint32_t* delay);

// cambia de CHS a numero de sector para poder enviarlo en el payload del nipcMsg_t
uint32_t TAKER_turnToSectorNum(CHS_t* CHSnode);

//devuelve el tiempo en ms que le lleva al disco llegar a cada sector
uint32_t TAKER_distanceTime(CHS_t*);

//calcula la distancia entre el sector alcanzado luego de llegar al cilindro y el sector al cual se buscaba llegar
uint32_t TAKER_sectReachedDistance(CHS_t* CHSrequest,CHS_t* CHSposition);

//devuelve la distancia que hay entre dos sectores logicos en el disco
uint32_t TAKER_sectorDist(uint32_t,uint32_t);

//actualiza la posicion del cabezal controlando que no cambie de cilindro
void TAKER_updateHPos(uint32_t sectorNum);

//devuelve 1 si el nodo A esta mas cerca del headPosition que el B
uint32_t TAKER_near(CHS_t* curr, CHS_t* headP,CHS_t* candidate,uint32_t (condition)(CHS_t,CHS_t));

uint32_t TAKER_getNextNode(queue_t* queue, queueNode_t** prevCandidate,uint32_t (condition)(CHS_t,CHS_t));

uint32_t TAKER_reachedSector(uint32_t cylinder,CHS_t* headPosCHS);

#endif /* PPD_TAKER_H_ */

