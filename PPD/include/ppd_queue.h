/*
 * ppd_queue.h
 *
 *  Created on: 17/10/2011
 *      Author: utn_so
 */

#ifndef PPD_QUEUE_H_
#define PPD_QUEUE_H_

#include <semaphore.h>
#include "nipc.h"
#include "ppd_common.h"

//LO SAQUE POR AHORA YA QUE TENEMOS EL tad_queue EN EL COMMONS

/*
typedef struct {
	NIPC_node* head;
	NIPC_node* tail;
	sem_t sem;
}	 queue_t;

//cambia de nipcMsg_t a NIPC_nodo para meterlo en la cola
uint32_t QUEUE_add(nipcMsg_t msg, queue_t* buffer);

// saca un nodo cualquiera de la cola si es que el first de la lista es nil
//si no es asi quita el nodo con el sector num mas cerca del head position
//luego lo cambia de tipo a requestNode_t para ser agregado a la lista grande
requestNode_t* QUEUE_take(queue_t* buffer);

// funcion que utiliza QUEUE_take para sacar el nodo con el numero de sector mas cerca del head position
uint32_t QUEUE_getHead(queue_t* queue);

*/
#endif /* PPD_QUEUE_H_ */
