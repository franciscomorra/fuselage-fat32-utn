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
ppd_node_t *thread_info_node;
extern queue_t ppdlist;
extern pthread_mutex_t ppdlist_mutex;
extern queue_t responselist;
extern pthread_mutex_t responselist_mutex;
extern pthread_mutex_t sync_mutex;

void *ppd_handler_thread (void *data) //TODO recibir el socket de ppd
{
	pthread_mutex_lock(&ppdlist_mutex);
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
		uint32_t sector = 0;
		uint32_t max_sectors = 524288;
		for (;sector < max_sectors;sector++)
		{
			nipcMsg_t msg = NIPC_createMsg(READ_SECTORS,sizeof(uint32_t),(char*) &sector);
			char* msg_inBytes = NIPC_toBytes(&msg);
			PFSREQUEST_addNew(thread_info_node->ppd_fd,msg_inBytes);
			free(msg_inBytes);
			NIPC_cleanMsg(&msg);
		}
		//TODO CAMBIAR ESTADO A READY
		pthread_mutex_unlock(&sync_mutex);
	}
	//SINCRONIZAR!!!! ENVIAR PEDIDO DE LECTURA DE TODOS LOS SECTORES A LOS OTROS DISCOS, CUANDO FUERON LEIDOS ENCOLARLOS EN LA COLA DE ESTE THREAD Y EMPEZAR A PROCESAR

	while (1)
	{
		sem_wait(&thread_info_node->request_list_sem);
		pthread_mutex_lock(&thread_info_node->request_list_mutex);

			 new_request_node = QUEUE_takeNode(&thread_info_node->request_list);
			 pfs_request_t* new_request = (pfs_request_t*) new_request_node->data;
			 char *msgToPPD = NIPC_toBytes(&new_request->msg);
			 uint16_t msgToPPD_len = *((uint16_t*) new_request->msg.len);

			 if (send(thread_info_node->ppd_fd,msgToPPD,msgToPPD_len+3) != -1)
			 {
				 //ENVIO A PPD OK // TODO DEJAR EL PFS_REQUEST PERO MARCARLO COMO ENVIADO, AGREGARLE UN ID DE PEDIDO, EL ID DEL THREAD QUE SE ENCARGO DE PEDIRLO
				 pthread_mutex_lock(&responselist_mutex);
					pfs_response_t *new_response = malloc(sizeof(pfs_response_t));
					new_response->pfs_fd = new_request->pfs_fd;
					new_response->ppd_fd = thread_info_node->ppd_fd;
					new_response->sector = *((uint32_t*) (msgToPPD+3));
					new_response->sync_response = false;
					QUEUE_appendNode(&responselist,new_response);
				 pthread_mutex_unlock(&responselist_mutex);

			 }
			 PFSREQUEST_free(new_request);
			 free(new_request_node);

		pthread_mutex_unlock(&thread_info_node->request_list_mutex);
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
