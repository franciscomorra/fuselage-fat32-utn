/*
 * ppd_queue.h
 *
 *  Created on: 04/12/2011
 *      Author: utn_so
 */

#ifndef PPD_QUEUE_H_
#define PPD_QUEUE_H_

#include <pthread.h>
#include <stdint.h>

enum
{
	READY = 0, WAIT_SYNCH = 1, SYNCHRONIZING = 3,
};

typedef struct ppd_node_t
{
	pthread_t thread_id;
	uint32_t ppd_fd;
	uint32_t disk_id;
	pthread_mutex_t sock_mutex;
	uint32_t requests_count;
	uint32_t status;
	uint32_t sectors_count;

} ppd_node_t;

ppd_node_t* PPDQUEUE_getByID(uint32_t disk_id);
ppd_node_t* PPDQUEUE_getByStatus(uint32_t status);
ppd_node_t* PPDQUEUE_getByFd(uint32_t fd);
ppd_node_t* PPDQUEUE_selectByLessRequests();
ppd_node_t* PPDQUEUE_addNewPPD(uint32_t ppd_fd,uint32_t disk_id,uint32_t sectors_count);

#endif /* PPD_QUEUE_H_ */
