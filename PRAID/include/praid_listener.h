/*
 * praid_listener.h
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */

#ifndef PRAID_LISTENER_H_
#define PRAID_LISTENER_H_

#include <pthread.h>
#include "praid_comm.h"
#include "praid_ppd_handler.h"
#include "praid_console.h"
#include "praid_queue.h"

void *praid_listener (void *);


#endif /* PRAID_LISTENER_H_ */
