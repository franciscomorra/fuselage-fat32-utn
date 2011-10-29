/*
 * sockets.h
 *
 *  Created on: 28/10/2011
 *      Author: utn_so
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

uint32_t Create_Sockets_INET(uint32_t*);

//recibe msj para saber el tipo y el FD asociado
uint32_t pfs_receive(char*,uint32_t);
#endif /* SOCKETS_H_ */
