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
uint32_t SSTF_getHead(queue_t*);

//agrega un nuevo requestNode_t a la lista segun el algoritmo SSTF
uint32_t SSTF_addRequest(requestNode_t* new);

//devuelve true si el nodo A esta mas cerca del nodo B que el C
uint32_t SSTF_near(CHS_t* A,CHS_t* C);

//definida en el ppd_main para ser usada como thread
uint32_t SSTF_main(void);

#endif /* PPD_REQUESTLIST_H_ */
