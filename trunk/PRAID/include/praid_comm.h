/*
 * praid_comm.h
 *
 *  Created on: 26/09/2011
 *      Author: utn_so
 */

#ifndef PRAID_COMM_H_
#define PRAID_COMM_H_


#include "nipc.h"
#include "praid_queue.h"

uint32_t pfs_receive(char*,uint32_t);


uint32_t  Create_Sockets_INET(uint32_t*);

#endif /* PRAID_COMM_H_ */
