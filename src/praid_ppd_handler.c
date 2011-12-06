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
#include <signal.h>

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

volatile sig_atomic_t got_pipe_broken;
void sigpipe_handler(int sig)
{
	got_pipe_broken = 1;
}

/*extern uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
extern pthread_mutex_t mutex_LIST;
extern t_log *raid_log_file;*/

extern queue_t ppd_list;
extern queue_t pending_request_list;
extern pthread_mutex_t pending_request_list_mutex;
extern pthread_mutex_t sync_mutex;


void *ppd_handler_thread (void *data) //TODO recibir el socket de ppd
{
		struct sigaction sa;

		got_pipe_broken = 0;

	    sa.sa_handler = sigpipe_handler;
	    sa.sa_flags = 0;
	    sigemptyset(&sa.sa_mask);

	    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
	        perror("sigaction");
	        exit(1);
	    }

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

	while ((thread_info_node->disconnected == false) && (!got_pipe_broken))
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
			//TODO VER BROKEN PIPE. SI SE DESCONECTA EL DISCO CUANDO SE LE ESTABA MANDANDO ALGO.
			//usar PPDLIST_handleDownPPD(thread_info_node);

		{
			/*if (*(new_request->msg) == 0x00)
			{
				uint32_t santi = 0;
			}
			*/
			//assert(*((uint32_t*)(new_request->msg+7)) <= 1048576);

			sent = SOCKET_sendAll(thread_info_node->ppd_fd,new_request->msg,*((uint16_t*)(new_request->msg+1))+3,0);
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

		if (sent > 0 && !got_pipe_broken)
		{
			pthread_mutex_lock(&pending_request_list_mutex);

			if (pfs_pending_request_exist(&pending_request_list,new_request->request_id,*((uint32_t*) (new_request->msg+7))) == false)
			{
				pfs_pending_request_t *new_pending_request = malloc(sizeof(pfs_pending_request_t));
				new_pending_request->pfs_fd = new_request->pfs_fd;
				new_pending_request->request_id = new_request->request_id;
				new_pending_request->ppd_fd = thread_info_node->ppd_fd;
				new_pending_request->sector = *((uint32_t*) (new_request->msg+7));
				new_pending_request->write_count = (new_request->request_id == 0) ? 0 : QUEUE_length(&ppd_list);
				new_pending_request->sync_write_response = (thread_info_node->status == SYNCHRONIZING) ? true : false;
				QUEUE_appendNode(&pending_request_list,new_pending_request);
			}
			pthread_mutex_unlock(&pending_request_list_mutex);
		 }
		 pthread_mutex_unlock(&thread_info_node->sock_mutex);

		 pfs_request_free(new_request);
		 free(new_request_node);

	}

	PPDLIST_handleDownPPD(thread_info_node);
	print_Console("THREAD terminado",thread_info_node->disk_ID,1,true);
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


