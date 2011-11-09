/*
 * ppd_main.h
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */

#ifndef PPD_MAIN_H_
#define PPD_MAIN_H_

#include <pthread.h>
#include "praid_ppd_handler.h"
#include "praid_console.h"
#include "praid_comm.h"
#include "praid_queue.h"

void *ppd_handler_thread(void *);

#endif /* PPD_MAIN_H_ */
