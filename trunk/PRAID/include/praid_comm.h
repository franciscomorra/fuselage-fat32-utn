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

uint32_t PRAID_pfs_receive(char*,uint32_t);
uint32_t PRAID_ppd_receive(char*,uint32_t);

//uint32_t  Create_Sockets_INET(uint32_t*);

//uint32_t send(uint32_t sockfd, const void *msg, uint32_t len, uint32_t flags);

//void fd_appendNode(queue_t*,void*);
void error_fd(uint32_t fd);
#endif /* PRAID_COMM_H_ */
