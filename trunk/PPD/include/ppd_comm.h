/*
 * ppd_comm.h
 *
 *  Created on: 06/09/2011
 *      Author: utn_so
 */

#ifndef PPD_COMM_H_
#define PPD_COMM_H_

#include <stdint.h>
#include "nipc.h"

//envia mensajes por sockets
uint32_t COMM_send(char*,uint32_t);

//recive mensajes y se fija de que tipo son
uint32_t COMM_handleReceive(char*,uint32_t);

uint32_t COMM_connect(uint32_t*);

//crea un string de la forma nipcMsg_t con un tamaño de payload de payload_bytes_len en cero
char* COMM_createCharMessage(NIPC_type type,uint32_t payload_bytes_len);


uint32_t COMM_recieve(uint32_t currFD);

#endif /* PPD_COMM_H_ */
