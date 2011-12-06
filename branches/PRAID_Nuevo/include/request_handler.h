/*
 * request_handler.h
 *
 *  Created on: 04/12/2011
 *      Author: utn_so
 */

#ifndef REQUEST_HANDLER_H_
#define REQUEST_HANDLER_H_

#include <stdint.h>

typedef struct request_t
{
	char 		*msg;
	uint32_t 	msg_len;
	uint32_t	ppd_fd;
	uint32_t	pfs_fd;
	uint32_t 	sector;
	uint32_t 	request_id;
} request_t;

request_t *request_take(uint32_t request_id,uint32_t sector);


#endif /* REQUEST_HANDLER_H_ */
