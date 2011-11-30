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
#include "tad_queue.h"

typedef struct socketPool_t
{
	uint32_t size;
	socketInet_t *sockets;
	sem_t free_sockets;

} socketPool_t;


char* PPDINTERFACE_readSectors(uint32_t* sectors_array, size_t sectors_array_len);

char* PPDINTERFACE_writeSectors(queue_t sectors_toWrite, size_t sectors_toWrite_len);

socketPool_t create_connections_pool(uint32_t max_conn,char* address,uint32_t port);

char* splitAndSort(char *sectors,uint32_t *indexes_array,size_t array_len);

int32_t sendMsgToPPD(socketInet_t socket,char *msg);

char* createRequest(NIPC_type msg_type,uint32_t payload_len,char* payload);

socketInet_t* PPDINTERFACE_getFreeSocket();

#endif /* PFS_COMM_H_ */
