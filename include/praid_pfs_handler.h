/*
 * praid_pfs_handler.h
 *
 *  Created on: 09/11/2011
 *      Author: utn_so
 */

#ifndef PRAID_PFS_HANDLER_H_
#define PRAID_PFS_HANDLER_H_
#include <stdint.h>
#include <stdbool.h>
#include "tad_queue.h"

typedef struct pfs_response
{
	uint32_t request_id;
	uint32_t write_count;
	uint32_t pfs_fd;
	uint32_t sector;
	uint32_t ppd_fd;
	bool sync_write_response;

} pfs_response_t;

void PFSRESPONSE_addNew(uint32_t sector, uint32_t ppd_fd,uint32_t pfs_fd);
void* PFSHANDLER_sendResponse(uint32_t ppd_fd,char* msg);
void PFSREQUEST_removeAll();
pfs_response_t* PFSRESPONSE_searchAndTake(queue_t* response_list,uint32_t request_id,uint32_t sector);

#endif /* PRAID_PFS_HANDLER_H_ */
