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
void TAKER_main();

// atiende al pedido que el algoritmo saca de la cola
void TAKER_handleRequest(queue_t*,requestNode_t*);

// cambia de CHS a numero de sector para poder enviarlo en el payload del nipcMsg_t
uint32_t TAKER_turnToSectorNum(requestNode_t* CHSnode);

//devuelve el tiempo en ms que le lleva al disco llegar a cada sector
uint32_t TAKER_getSleepTime(requestNode_t*);

//devuelve la distancia que hay entre dos sectores logicos en el disco
uint32_t TAKER_sectorDist(uint32_t,uint32_t);

uint32_t TAKER_getReachedDistance(requestNode_t* request,requestNode_t* CHSposition);

#endif /* PPD_TAKER_H_ */

