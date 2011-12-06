/*
 * ppd_thread.c
 *
 *  Created on: 04/12/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "tad_sockets.h"
#include "nipc.h"
#include "ppd_thread.h"
#include "ppd_queue.h"
#include "ppd_synchronizer.h"
#include "request_handler.h"
#include "utils.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

extern pthread_mutex_t PPD_SYNCHRONIZING_MUTEX;
extern t_log *raid_log;


void* ppd_thread(void *data)
{

	ppd_node_t *ppd_info = (ppd_node_t*) data;

	if (ppd_info->status == WAIT_SYNCH)
	{

		pthread_mutex_lock(&PPD_SYNCHRONIZING_MUTEX);
			ppd_info->status = SYNCHRONIZING;
			pthread_t sync_thread_id;
			pthread_create(&sync_thread_id,NULL,ppd_synchronizer,data);
			pthread_join(sync_thread_id,NULL);
			ppd_info->status = READY;
		pthread_mutex_unlock(&PPD_SYNCHRONIZING_MUTEX);
	}

	while (ppd_info->status == READY)
	{
		int32_t readable_bytes = 0;

		/*if (ioctl(ppd_info->ppd_fd,FIONREAD,&readable_bytes) == 0)
		{
			if (readable_bytes >= 523)
			{*/
				//pthread_mutex_lock(&ppd_info->sock_mutex);
				char *request_received = malloc(523);

				int32_t request_receive_result = recv(ppd_info->ppd_fd,request_received,523,MSG_WAITALL);

				if (request_receive_result != SOCK_DISCONNECTED)
				{
					uint32_t request_id = *((uint32_t*) (request_received+3));
					uint32_t sector = *((uint32_t*) (request_received+7));

					if (request_id == 0)
					{
						ppd_node_t *synchronizing_ppd = PPDQUEUE_getByStatus(SYNCHRONIZING);
						*request_received = WRITE_SECTORS;
						//printf("%d\n",getMicroseconds()-one_sector_read_time);
						//fflush(stdout);
						SOCKET_sendAll(synchronizing_ppd->ppd_fd,request_received,523,0);
						//send(synchronizing_ppd->ppd_fd,request_received,523,MSG_WAITALL);
						uint32_t santi = 0;

					}
					else
					{
						request_t *request = request_take(request_id,sector);
						send(request->pfs_fd,request_received,523,MSG_WAITALL);
						free(request_received);
						request_free(request);
					}
				}
				else
				{
					log_info(raid_log,"MAIN_THREAD","DESCONEXION DISCO [ID: %d]",ppd_info->disk_id);
					//REORGANIZAR REQUESTS DE ESTE DISCO
				}
				free(request_received);
				//pthread_mutex_unlock(&ppd_info->sock_mutex);
			/*}
		}*/
	}
}
