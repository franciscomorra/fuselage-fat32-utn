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
typedef struct pfs_response
{
	uint32_t pfs_fd;
	uint32_t sector;
	uint32_t ppd_fd;
	bool sync_response;
} pfs_response_t;

void PFSRESPONSE_addNew(uint32_t sector, uint32_t ppd_fd,uint32_t pfs_fd);
void* PFSHANDLER_sendResponse(uint32_t ppd_fd,char* msg);
#endif /* PRAID_PFS_HANDLER_H_ */