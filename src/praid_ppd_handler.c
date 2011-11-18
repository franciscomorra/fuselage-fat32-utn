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

/*extern uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
extern pthread_mutex_t mutex_LIST;
extern t_log *raid_log_file;*/

extern queue_t ppdlist;
extern pthread_mutex_t ppdlist_mutex;
extern queue_t responselist;
extern pthread_mutex_t responselist_mutex;
extern pthread_mutex_t sync_mutex;
extern uint32_t sync_write_count;
extern uint32_t total_sectors;
extern sem_t sync_ready_sem;
extern pthread_mutex_t prueba_mutex;

void *ppd_handler_thread (void *data) //TODO recibir el socket de ppd
{
	pthread_mutex_lock(&ppdlist_mutex);
	ppd_node_t *thread_info_node;
	queueNode_t *new_request_node;
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
	pthread_mutex_unlock(&ppdlist_mutex);

	if (thread_info_node->status == WAIT_SYNCH)
	{
		pthread_mutex_lock(&sync_mutex);
		sync_write_count = total_sectors;

		uint32_t sector = 0;
		for (;sector < total_sectors;sector++)
		{
			char *sync_msg = malloc(8);
			*sync_msg = 0x01;
			*((uint16_t*) (sync_msg+1)) = 8;
			*((uint32_t*) (sync_msg+3)) = 0;
			*((uint32_t*) (sync_msg+7)) = sector;

			pthread_mutex_lock(&prueba_mutex);
			PFSREQUEST_addNew(thread_info_node->ppd_fd,sync_msg);

			free(sync_msg);
		}
		sem_wait(&sync_ready_sem);
		sem_wait(&sync_ready_sem);
		thread_info_node->status = READY;
		sem_post(&sync_ready_sem);
		pthread_mutex_unlock(&sync_mutex);
	}
	//SINCRONIZAR!!!! ENVIAR PEDIDO DE LECTURA DE TODOS LOS SECTORES A LOS OTROS DISCOS, CUANDO FUERON LEIDOS ENCOLARLOS EN LA COLA DE ESTE THREAD Y EMPEZAR A PROCESAR

	while (1)
	{
		sem_wait(&thread_info_node->request_list_sem);
		pthread_mutex_lock(&thread_info_node->request_list_mutex);

			 new_request_node = QUEUE_takeNode(&thread_info_node->request_list);
			 pfs_request_t* new_request = (pfs_request_t*) new_request_node->data;
			 pthread_mutex_unlock(&thread_info_node->request_list_mutex);
			  pthread_mutex_lock(&thread_info_node->sock_mutex);
			  int32_t sent = COMM_send(new_request->msg,thread_info_node->ppd_fd);
			  pthread_mutex_unlock(&thread_info_node->sock_mutex);
			 if (sent != -1)
			 {

				 //ENVIO A PPD OK // TODO DEJAR EL PFS_REQUEST PERO MARCARLO COMO ENVIADO, AGREGARLE UN ID DE PEDIDO, EL ID DEL THREAD QUE SE ENCARGO DE PEDIRLO
				 pthread_mutex_lock(&responselist_mutex);
					pfs_response_t *new_response = malloc(sizeof(pfs_response_t));
					new_response->pfs_fd = new_request->pfs_fd;
					new_response->request_id = new_request->request_id;
					new_response->ppd_fd = thread_info_node->ppd_fd;
					if (new_request->msg[0] == WRITE_SECTORS)
					{
						//printf("W%d -> PPD:%d\n",*((uint32_t*) (new_request->msg+7)),thread_info_node->ppd_fd);
						new_response->write_count = QUEUE_length(&ppdlist);
					}
					else
					{
						//printf("R%d -> PPD:%d\n",*((uint32_t*) (new_request->msg+7)),thread_info_node->ppd_fd);
						new_response->write_count = 0;
					}
					new_response->sector = *((uint32_t*) (new_request->msg+7));
					new_response->sync_write_response = false;
					QUEUE_appendNode(&responselist,new_response);
				 pthread_mutex_unlock(&responselist_mutex);

			 }

			 PFSREQUEST_free(new_request);
			 free(new_request_node);


	}
	//SINCRONIZAR!!!!
	//TODO Recibir cantidad de sectores en el HANDSHAKE


return NULL;
}


/*
if(RAID_STATUS == 0 || RAID_STATUS == 2){ //SI ME LO PASAN POR HANDSHAKE ESTE BARDO NO VA
	//Si es el primer PPD que se conecta, pide informacion del disco (DISK_SECTORS_AMOUNT)
	//Por si acaso se cae el primer disco antes de recibir el pedido de DISK_SECTORS_AMOUNT, pregunto estado 2
	pthread_mutex_lock(&mutex_RAID_STATUS);
	RAID_STATUS = 2; //RAID ESPERANDO DISK_SECTORS_AMOUNT
	pthread_mutex_unlock(&mutex_RAID_STATUS);

	//TODO Enviar Pedido de informacion de Disco (DISK_SECTORS_AMOUNT)
	//Esperar respuesta
	//Decodificar respuesta
	if(RAID_STATUS != 1){//Si no lo activaron a ultimo momento
		//Setear DISK_SECTORS_AMOUNT
		pthread_mutex_lock(&mutex_RAID_STATUS);
		RAID_STATUS = 1; //RAID ACTIVADO
		pthread_mutex_unlock(&mutex_RAID_STATUS);
		print_Console("RAID Activado",self_tid);//CONSOLE STATUS ACTIVE
		log_debug(raid_log_file,"PRAID","RAID Activado(%s)",pthread_self());
	}

} ESTO IRIA ENCAPSULADO EN UN WHILE ANTES DE TOMAR PEDIDOS
*/
