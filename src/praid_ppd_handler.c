/*
 * praid_ppd_main.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "praid_ppd_handler.h"
#include "praid_pfs_handler.h"
#include "praid_console.h"
#include "log.h"
#include "praid_comm.h"
#include "praid_queue.h"
#include "tad_queue.h"
#include "praid_ppdlist.h"
#include "praid_synchronize.h"
#include "tad_sockets.h"
#include <assert.h>
/*extern uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
extern pthread_mutex_t mutex_LIST;
extern t_log *raid_log_file;*/

extern queue_t ppdlist;
extern queue_t responselist;
extern pthread_mutex_t responselist_mutex;
extern pthread_mutex_t sync_mutex;

void *ppd_handler_thread (void *data) //TODO recibir el socket de ppd
{
	/*pthread_mutex_lock(&ppdlist_mutex);
	ppd_node_t *thread_info_node;
	queueNode_t *new_request_node;
	pfs_request_t *new_request;
	queueNode_t *cur_ppdnode = ppdlist.begin;
	while (cur_ppdnode != NULL)
	{
		if (((ppd_node_t*) cur_ppdnode->data)->thread_id == pthread_self())
		{
			thread_info_node = (ppd_node_t*) cur_ppdnode->data;
			break;
		}
		cur_ppdnode = cur_ppdnode->next;
	}
	pthread_mutex_unlock(&ppdlist_mutex);*/
	ppd_node_t *thread_info_node = (ppd_node_t*) data;
	if (thread_info_node->status == WAIT_SYNCH)
	{
		pthread_mutex_lock(&sync_mutex);
		pthread_t synchronize_tid;
		printf("Comenzando sincronizacion\n");
		pthread_create(&synchronize_tid,NULL,synchronize_thread,(void*) thread_info_node);
	}

	while (1)
	{

		//if (sync_write_count == 0) thread_info_node->status = READY;
		sem_wait(&thread_info_node->request_list_sem);

		pthread_mutex_lock(&thread_info_node->request_list_mutex);
			queueNode_t *new_request_node = QUEUE_takeNode(&thread_info_node->request_list);
			pfs_request_t *new_request = (pfs_request_t*) new_request_node->data;
		pthread_mutex_unlock(&thread_info_node->request_list_mutex);

		fd_set write_set;
		FD_ZERO(&write_set);
		FD_SET(thread_info_node->ppd_fd,&write_set);
		select(thread_info_node->ppd_fd+1,NULL,&write_set,NULL,NULL);

		pthread_mutex_lock(&thread_info_node->sock_mutex);

		int32_t	sent = 0;

		if(FD_ISSET(thread_info_node->ppd_fd,&write_set))
		{
			/*if (*(new_request->msg) == 0x00)
			{
				uint32_t santi = 0;
			}
			*/
			//assert(*((uint32_t*)(new_request->msg+7)) <= 1048576);
			sent = SOCKET_sendAll(thread_info_node->ppd_fd,new_request->msg,*((uint16_t*)(new_request->msg+1))+3,NULL);
			/*assert(*((uint16_t*)(new_request->msg+1))+3 == 523 || *((uint16_t*)(new_request->msg+1))+3 == 11);
			assert(sent == 523 || sent == 11);
			//sent = COMM_send(new_request->msg,thread_info_node->ppd_fd);
			if (sent == SOCK_DISCONNECTED || sent == SOCK_ERROR)
			{
				uint32_t error = 0;
			}*/
			/*if (*(new_request->msg) == 0x02)
			{
				printf("OUT W%d\n",*((uint32_t*) (new_request->msg+7)));
			}
			else
			{
				printf("OUT R%d\n",*((uint32_t*) (new_request->msg+7)));
			}*/

		}

		if (sent > 0)
		{
			pthread_mutex_lock(&responselist_mutex);

			//if (PFSRESPONSE_search(&responselist,new_request->request_id,*((uint32_t*) (new_request->msg+7))) == false)
			{
				pfs_response_t *new_response = malloc(sizeof(pfs_response_t));
				new_response->pfs_fd = new_request->pfs_fd;
				new_response->request_id = new_request->request_id;
				new_response->ppd_fd = thread_info_node->ppd_fd;
				new_response->write_count = 0;
				new_response->sector = *((uint32_t*) (new_request->msg+7));

				if (thread_info_node->status == WAIT_SYNCH)
					new_response->sync_write_response = true;
				else
					new_response->sync_write_response = false;

				QUEUE_appendNode(&responselist,new_response);
			}
			pthread_mutex_unlock(&responselist_mutex);
		 }
		 pthread_mutex_unlock(&thread_info_node->sock_mutex);

		 PFSREQUEST_free(new_request);
		 free(new_request_node);

	}

	return NULL;
}


void printTime()
{
	time_t     now;
	struct tm *ts;
	char       buf[80];

	/* Obtener la hora actual */
	now = time(0);

	/* Formatear e imprimir el tiempo, "ddd yyyy-mm-dd hh:mm:ss zzz" */
	ts = localtime(&now);
	strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
	printf("%s\n", buf);
}


