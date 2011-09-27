/*
 * ppd_comm.h
 *
 *  Created on: 06/09/2011
 *      Author: utn_so
 */

#ifndef PPD_COMM_H_
#define PPD_COMM_H_

#include <stdint.h>
#include "nipc.h"

enum {
	HANDSHAKE=0x00, READ=0x01, WRITE=0x02
};


int32_t ppd_send(msgNIPC_t msg);
msgNIPC_t ppd_receive(msgNIPC_t msgIn);

#endif /* PPD_COMM_H_ */
