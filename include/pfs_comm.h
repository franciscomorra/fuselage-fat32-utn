/*
 * pfs_comm.h
 *
 *  Created on: 06/09/2011
 *      Author: utn_so
 */

#ifndef PFS_COMM_H_
#define PFS_COMM_H_
#include <stdint.h>
#include <stdlib.h>
#include "nipc.h"
#include "tad_sockets.h"
#include <semaphore.h>

typedef struct socketPool_t
{
	uint32_t size;
	socketInet_t *sockets;
	sem_t free_sockets;

} socketPool_t;


//char* PFS_requestSectorsRead(uint32_t *sectors,size_t sectors_count);
char* PFS_requestSectorsOperation(NIPC_type request_type,uint32_t *sectors,size_t sectors_count);

//char* PFS_request(char* msg);
char* PFS_request(nipcMsg_t msg);

socketPool_t create_connections_pool(uint32_t max_conn,char* address,uint32_t port);

char* splitAndSort(char *sectors,uint32_t *indexes_array,size_t array_len);

int32_t sendMsgToPPD(socketInet_t socket,nipcMsg_t *msg);

#endif /* PFS_COMM_H_ */
