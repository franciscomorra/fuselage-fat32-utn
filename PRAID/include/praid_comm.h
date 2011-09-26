/*
 * praid_comm.h
 *
 *  Created on: 26/09/2011
 *      Author: utn_so
 */

#ifndef PRAID_COMM_H_
#define PRAID_COMM_H_
#include <stdint.h>
#include <stdlib.h>

enum {
	HANDSHAKE=0x00, READ=0x01, WRITE=0x02
};


uint32_t PRAID_takeRequest(msgNIPC_t msg, nipc_node* first);
uint32_t PRAID_manageRequest(msgNIPC_t msg,nipc_node* first, nipc_node* last);


#endif /* PRAID_COMM_H_ */
