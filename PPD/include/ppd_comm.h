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

#define HANDSHAKE 0
#define READ 2
#define WRITE 1

uint32_t ppd_send(nipcMsg_t msg);
uint32_t ppd_receive(nipcMsg_t msgIn);

#endif /* PPD_COMM_H_ */
