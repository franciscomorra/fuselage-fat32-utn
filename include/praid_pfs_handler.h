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

typedef struct pfs_pending_request
{
	uint32_t request_id;
	uint32_t write_count;
	uint32_t pfs_fd;
	uint32_t sector;
	uint32_t ppd_fd;
	bool sync_write_response;

} pfs_pending_request_t;

void pfs_pending_request_addNew(uint32_t sector, uint32_t ppd_fd,uint32_t pfs_fd);
void pfs_pending_request_attendTo(uint32_t ppd_fd,char* msg);
void pfs_pending_request_removeAll();
pfs_pending_request_t* pfs_pending_request_searchAndTake(queue_t* response_list,uint32_t request_id,uint32_t sector);

#endif /* PRAID_PFS_HANDLER_H_ */
