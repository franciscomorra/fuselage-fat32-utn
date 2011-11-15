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
#include "tad_sockets.h"

//recive mensajes y se fija de que tipo son
uint32_t COMM_handleReceive(char*,uint32_t);

uint32_t COMM_connect(uint32_t*);

//crea un string de la forma nipcMsg_t con un tamaño de payload de payload_bytes_len en cero
char* COMM_createCharMessage(NIPC_type type,uint32_t payload_bytes_len);

//acepta conexion proveniente del socket de escucha de la consola.
//devuelve la estructura correspondiente al socket que transfiere datos
socketUnix_t COMM_ConsoleAccept(socketUnix_t consoleListen);

//manda el mensaje de Handshake al Raid y espera su respuesta para luego loggear error si es que lo hay.
void COMM_RaidHandshake(socketInet_t inetListen,uint32_t diskID);


#endif /* PPD_COMM_H_ */
