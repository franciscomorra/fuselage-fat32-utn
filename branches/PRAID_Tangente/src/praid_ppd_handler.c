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
#include <sys/ioctl.h>

/*extern uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
extern pthread_mutex_t mutex_LIST;
extern t_log *raid_log_file;*/

extern queue_t ppd_list;
extern queue_t pending_request_list;
extern pthread_mutex_t pending_request_list_mutex;
extern pthread_mutex_t sync_mutex;

void *ppd_handler_thread (void *data) //TODO recibir el socket de ppd
{

	ppd_node_t *thread_info_node = (ppd_node_t*) data;

	if (thread_info_node->status == WAIT_SYNCH)
	{
		pthread_mutex_lock(&sync_mutex);
		thread_info_node->status = SYNCHRONIZING;
		pthread_t synchronize_tid;
		//printf("Comenzando sincronizacion\n");
		print_Console("INICIO SINCRONZIACION:",thread_info_node->disk_ID,1,true);
		PRAID_WRITE_LOG("INICIO SINCRONZIACION");
		pthread_create(&synchronize_tid,NULL,synchronize_thread,(void*) thread_info_node);
	}

	while (1)
	{
		char *msg_buf;
		int32_t readable_bytes = 0;

		if (ioctl(thread_info_node->ppd_fd,FIONREAD,&readable_bytes) == 0)
		{
			if (readable_bytes >= 523)
			{
				msg_buf = malloc(523);
				int32_t res = SOCKET_recvAll(thread_info_node->ppd_fd,msg_buf,523,0);
				if (res == SOCK_DISCONNECTED || res == SOCK_ERROR)
				{

				}
				else
				{
					//TODO BUSCAR EL PENDIENTE
					uint32_t request_id = *((uint32_t) (msg_buf+3));
					uint32_t sector = *((uint32_t) (msg_buf+7));

					if (request_id == 0)
					{

					}
					pfs_pending_request_t *pending_request  = pfs_pending_request_searchAndTake(&pending_request_list,request_id,sector);
					pending_request

				}
			}
		}
	}
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


void PPD_handleWriteResponse()
{

}

void PPD_handleReadResponse()
{

}

