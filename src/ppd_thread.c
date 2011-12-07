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
#include <assert.h>
#include <stdlib.h>
#include <string.h>

extern pthread_mutex_t PPD_SYNCHRONIZING_MUTEX;
extern pthread_mutex_t REQUEST_QUEUE_MUTEX;
extern t_log *raid_log;


void* ppd_thread(void *data)
{

	ppd_node_t *ppd_info = (ppd_node_t*) data;

	ppd_info->status = READY;
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
		char *request_received = malloc(523);

		int32_t request_receive_result = recv(ppd_info->ppd_fd,request_received,523,MSG_WAITALL);

		if (request_receive_result > 0)
		{
			//TODO VERIFICAR QUE NO SEA CERO ANTES DE DECREMENTAR
			ppd_info->requests_count--;
			char request_type = *request_received;
			uint32_t request_id = *((uint32_t*) (request_received+3));
			uint32_t sector = *((uint32_t*) (request_received+7));
			//assert(request_id == 0);

			if (request_id == 0)
			{
				//request_t *sync_request = request_search(request_id,sector);
				ppd_node_t *synchronizing_ppd = PPDQUEUE_getByStatus(SYNCHRONIZING);
				*request_received = WRITE_SECTORS;

					pthread_mutex_lock(&synchronizing_ppd->sock_mutex);
						send(synchronizing_ppd->ppd_fd,request_received,523,MSG_WAITALL);
					pthread_mutex_unlock(&synchronizing_ppd->sock_mutex);
			}
			else
			{
				if (request_type == WRITE_SECTORS)
				{
					pthread_mutex_lock(&REQUEST_QUEUE_MUTEX);
						request_t *request = request_search(request_id,sector);
					pthread_mutex_unlock(&REQUEST_QUEUE_MUTEX);
					//TODO VER ERROR ACA DE PORQUE NO ENVIA LA RESPUESTA
					if (--(request->write_count) == 0)
					{
						request = request_take(request_id,sector);
						send(request->pfs_fd,request_received,523,MSG_WAITALL);
						request_free(request);
					}

				}
				else if (request_type == READ_SECTORS)
				{
					pthread_mutex_lock(&REQUEST_QUEUE_MUTEX);
						request_t *request = request_take(request_id,sector);
					pthread_mutex_unlock(&REQUEST_QUEUE_MUTEX);
					send(request->pfs_fd,request_received,523,MSG_WAITALL);
					request_free(request);
				}

			}
		}
		else
		{

			log_info(raid_log,"MAIN_THREAD","DESCONEXION DISCO [ID: %d]",ppd_info->disk_id);
			pthread_exit(NULL);
			//REORGANIZAR REQUESTS DE ESTE DISCO
		}
		free(request_received);
	}
}
