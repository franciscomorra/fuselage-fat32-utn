/*
 * ppd_synchronizer.c
 *
 *  Created on: 04/12/2011
 *      Author: utn_so
 */


#include <stdint.h>
#include <sys/ioctl.h>
#include "log.h"
#include <sys/socket.h>
#include "nipc.h"
#include "request_handler.h"
#include "ppd_synchronizer.h"
#include "ppd_queue.h"
#include "tad_sockets.h"
#include <stdlib.h>

extern pthread_mutex_t REQUEST_QUEUE_MUTEX;

void* ppd_synchronizer(void *data)
{
	uint32_t *sync_result = malloc(4);
	ppd_node_t *ppd_info = (ppd_node_t*) data;

	uint32_t sectors_to_synchronize = ppd_info->sectors_count;



	uint32_t requests_sent = 0, requests_received = 0;
	char *msg_buf;
	int32_t left, received;
	while (requests_received < sectors_to_synchronize)
	{
		if (requests_sent < sectors_to_synchronize )
		{
			ppd_node_t *selected_ppd = PPDQUEUE_selectByLessRequests();

			if (SOCKET_canSend(selected_ppd->ppd_fd))
			{
			//pthread_mutex_lock(&selected_ppd->sock_mutex);
				msg_buf = malloc(11);
				*msg_buf = READ_SECTORS;
				*((uint16_t*)(msg_buf+1)) = 8;
				*((uint32_t*)(msg_buf+3)) = 0;
				*((uint32_t*)(msg_buf+7)) = requests_sent;
				//one_sector_read_time = getMicroseconds();
				//send(selected_ppd->ppd_fd,msg_buf,11,MSG_WAITALL);

				//SOCKET_sendAll(selected_ppd->ppd_fd,msg_buf,11,0);

				send(selected_ppd->ppd_fd,msg_buf,11,MSG_WAITALL);

				request_addNew(selected_ppd->ppd_fd,ppd_info->ppd_fd,msg_buf);
				requests_sent++;
				//free(msg_buf);
			}
			//pthread_mutex_unlock(&selected_ppd->sock_mutex);
		}

		int32_t readable_bytes = 0;

		if (ioctl(ppd_info->ppd_fd,FIONREAD,&readable_bytes) == 0)
		{
			if (readable_bytes >= 523)
			{
				msg_buf = malloc(523);
				received = recv(ppd_info->ppd_fd,msg_buf,523, MSG_WAITALL);

				if (received > 0)
				{
					uint32_t request_id = *((uint32_t*)(msg_buf+3));
					uint32_t sector = *((uint32_t*)(msg_buf+7));
					pthread_mutex_lock(&REQUEST_QUEUE_MUTEX);
					request_t* sync_request = request_take(request_id,sector);
					pthread_mutex_unlock(&REQUEST_QUEUE_MUTEX);
					request_free(sync_request);
					requests_received++;
				}
				free(msg_buf);

			}
		}

	}

	return sync_result;
}




