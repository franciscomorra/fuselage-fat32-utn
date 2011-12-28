/*
 * ppd_thread.c
 *
 *  Created on: 04/12/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "tad_sockets.h"
#include "tad_queue.h"
#include "nipc.h"
#include "ppd_thread.h"
#include "ppd_queue.h"
#include "ppd_synchronizer.h"
#include "request_handler.h"
#include "utils.h"
#include "log.h"
#include "pfs_queue.h"

extern pthread_mutex_t PPD_SYNCHRONIZING_MUTEX;
extern pthread_mutex_t PPD_QUEUE_MUTEX;
extern pthread_mutex_t REQUEST_QUEUE_MUTEX;
extern queue_t REQUEST_QUEUE;
extern queue_t PFS_QUEUE;
extern uint32_t PPD_COUNT;
extern t_log *raid_log;
void replan_requests(uint32_t ppd_fd);
void standby();

void* ppd_thread(void *data)
{

	ppd_node_t *ppd_info = (ppd_node_t*) data;
	//ppd_info->status = READY;

	if (ppd_info->status == WAIT_SYNCH)
	{

		pthread_mutex_lock(&PPD_SYNCHRONIZING_MUTEX);
			ppd_info->status = SYNCHRONIZING;
			pthread_t sync_thread_id;
			log_info(raid_log,"PPD_SYNCHRONIZER","ESTADO: SINCRONIZANDO DISCO [ID: %d]",ppd_info->disk_id);
			pthread_create(&sync_thread_id,NULL,ppd_synchronizer,data);
			pthread_join(sync_thread_id,NULL);
			log_info(raid_log,"PPD_SYNCHRONIZER","ESTADO: FIN SINCRONIZACION DISCO [ID: %d]",ppd_info->disk_id);
			ppd_info->status = READY;
			ppd_info->requests_count = 0;
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
						request->write_count--;
						pthread_mutex_unlock(&REQUEST_QUEUE_MUTEX);
					//TODO VER ERROR ACA DE PORQUE NO ENVIA LA RESPUESTA
					if (request->write_count == 0)
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

					if (PPD_COUNT > 1)
					{
						pthread_mutex_t *pfs_mutex = PFSQUEUE_getMutex(request->pfs_fd);
						pthread_mutex_lock(pfs_mutex);
						send(request->pfs_fd,request_received,523,MSG_WAITALL);
						pthread_mutex_unlock(pfs_mutex);
					}
					else
					{
						send(request->pfs_fd,request_received,523,MSG_WAITALL);
					}
					request_free(request);
				}

			}
		}
		else
		{

			uint32_t disk_id = ppd_info->disk_id;
			log_info(raid_log,"PPD_THREAD","DESCONEXION DISCO [ID: %d]",disk_id);
			pthread_mutex_lock(&PPD_QUEUE_MUTEX);
			PPDQUEUE_removePPD(disk_id);
			PPD_COUNT--;
			pthread_mutex_unlock(&PPD_QUEUE_MUTEX);

			if (PPD_COUNT > 0)
			{
				replan_requests(ppd_info->ppd_fd);
			}
			else
			{
				standby();
				log_info(raid_log,"PPD_THREAD","ESTADO: ESPERANDO CONEXION DE DISCO");
			}
			pthread_exit(NULL);
			//REORGANIZAR REQUESTS DE ESTE DISCO
		}
		free(request_received);
	}
}

void replan_requests(uint32_t ppd_fd)
{
	pthread_mutex_lock(&REQUEST_QUEUE_MUTEX);
	queueNode_t *cur_node = REQUEST_QUEUE.begin;
	while (cur_node != NULL)
	{
		request_t *cur_request = (request_t*) cur_node->data;
		if (cur_request->ppd_fd == 0)
		{
			if (cur_request->write_count > 0)
				cur_request->write_count -= 1;
		}
		else if (cur_request->ppd_fd == ppd_fd)
		{
			ppd_node_t *selected_ppd = PPDQUEUE_selectByLessRequests();
			send(selected_ppd->ppd_fd,cur_request->msg,cur_request->msg_len,MSG_WAITALL);
			selected_ppd->requests_count++;
		}

		cur_node = cur_node->next;
	}
	pthread_mutex_unlock(&REQUEST_QUEUE_MUTEX);
	return;
}

void standby()
{
	extern fd_set masterFDs;
	extern fd_set PFS_FDs;
	extern fd_set readFDs;
	queueNode_t *cur_node = REQUEST_QUEUE.begin;

	while (cur_node != NULL)
	{
		request_t *cur_request = (request_t*) cur_node->data;
		queueNode_t *aux = cur_node->next;
		free(cur_request->msg);
		free(cur_request);
		free(cur_node);
		cur_node = aux;
	}

	 REQUEST_QUEUE.begin = REQUEST_QUEUE.end = NULL;
	cur_node = PFS_QUEUE.begin;
	while (cur_node != NULL)
	{
		pfs_node_t *cur_pfs = (pfs_node_t*) cur_node->data;
		queueNode_t *aux = cur_node->next;
		pthread_mutex_destroy(&cur_pfs->socket_mutex);
		FD_CLR(cur_pfs->pfs_fd,&PFS_FDs);
		FD_CLR(cur_pfs->pfs_fd,&masterFDs);
		FD_CLR(cur_pfs->pfs_fd,&readFDs);
		close(cur_pfs->pfs_fd);
		free(cur_pfs);
		free(cur_node);
		cur_node = aux;
	}
//	readFDs = masterFDs;
	PFS_QUEUE.begin = PFS_QUEUE.end = NULL;

}

