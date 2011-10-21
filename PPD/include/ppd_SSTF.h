/*
 * ppd_RequestList.h
 *
 *  Created on: Oct 1, 2011
 *      Author: utn_so
 */

#ifndef PPD_REQUESTLIST_H_
#define PPD_REQUESTLIST_H_

#include "ppd_common.h"
#include "tad_queue.h"

// va colocando en la cabeza de la cola el nodo cuyo CHS este mas cerca del headPosition
void SSTF_getHead(queue_t*);

//saca el primer nodo de la cola y lo transforma de tipo queueNode a requestNode
requestNode_t* SSTF_takeRequest(queue_t*);

//agrega un nuevo requestNode_t a la lista segun el algoritmo SSTF
uint32_t SSTF_addRequest(requestNode_t* new);

//devuelve true si el nodo A esta mas cerca del nodo B que el C
uint32_t SSTF_near(requestNode_t* A,requestNode_t* B,requestNode_t* C);

//devuelve la cantidad de sectores que tiene que recorrer para llegar de fst a lst.
uint32_t SSTF_sectorDist(uint32_t fstSector, uint32_t lstSector);

//definida en el ppd_main para ser usada como thread
uint32_t SSTF_main(void);

#endif /* PPD_REQUESTLIST_H_ */
