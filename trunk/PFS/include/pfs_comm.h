/*
 * pfs_comm.h
 *
 *  Created on: 06/09/2011
 *      Author: utn_so
 */

#ifndef PFS_COMM_H_
#define PFS_COMM_H_
#include <stdint.h>

/*enum {
	HANDSHAKE=0x00, READ=0x01, WRITE=0x02
};*/


int32_t pfs_send(char* msg);
char* pfs_receive(char* msg);

#endif /* PFS_COMM_H_ */
