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

enum {
	HANDSHAKE=0x00, READ=0x01, WRITE=0x02
};


//char* PFS_requestSectorsRead(uint32_t *sectors,size_t sectors_count);
msgNIPC_t PFS_requestSectorsRead(uint32_t *sectors,size_t sectors_count);

//char* PFS_request(char* msg);
char* PFS_request(msgNIPC_t msg);

#endif /* PFS_COMM_H_ */
