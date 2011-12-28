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


uint32_t ppd_read_boot_sector();

char* ppd_read_sectors(uint32_t* sectors_array, size_t sectors_array_len);

char* ppd_write_sectors(queue_t sectors_toWrite, size_t sectors_toWrite_len);

socketPool_t ppd_create_connection_pool(uint32_t max_conn,char* address,uint32_t port);

char* ppd_reconstruct_data_from_responses(char *sectors,uint32_t *indexes_array,size_t array_len);

socketInet_t* ppd_get_free_socket();

#endif /* PFS_COMM_H_ */
