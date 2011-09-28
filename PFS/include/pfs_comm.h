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



//char* PFS_requestSectorsRead(uint32_t *sectors,size_t sectors_count);
char* PFS_requestSectorsOperation(NIPC_type request_type,uint32_t *sectors,size_t sectors_count);

//char* PFS_request(char* msg);
char* PFS_request(NIPC_msg msg);

#endif /* PFS_COMM_H_ */
