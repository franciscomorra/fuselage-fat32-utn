/*
 * praid_ppdlist.c
 *
 *  Created on: 09/11/2011
 *      Author: utn_so
 */
#include "praid_ppdlist.h"
#include <pthread.h>
#include <stdint.h>
#include "tad_queue.h"
#include <semaphore.h>

extern pthread_mutex_t ppd_list_mutex;
extern queue_t ppd_list;
extern uint32_t sectors_perDisk;
extern uint32_t ACTIVE_DISKS_AMOUNT;
extern uint32_t DISKS_AMOUNT;
extern uint32_t pending_writes_forSyncronization;
ppd_node_t *PPDLIST_addNewPPD(uint32_t ppd_fd,pthread_t thread_id,uint32_t diskID)
{
	ppd_node_t *new_ppd = malloc(sizeof(ppd_node_t));
	new_ppd->ppd_fd = ppd_fd;
	new_ppd->thread_id = thread_id;
	new_ppd->disk_ID = diskID;
	new_ppd->disconnected = false;
	//if (QUEUE_length(&ppd_list) == 0)
	if (DISKS_AMOUNT == 0)
	{
		new_ppd->status=READY;
		ACTIVE_DISKS_AMOUNT++;
	}
	else
	{
		new_ppd->status=WAIT_SYNCH;
	}
	DISKS_AMOUNT++;

	QUEUE_initialize(&new_ppd->request_list);
	pthread_mutex_init(&new_ppd->request_list_mutex,NULL);
	pthread_mutex_init(&new_ppd->sock_mutex,NULL);
	sem_init(&new_ppd->request_list_sem,NULL,0);
	QUEUE_appendNode(&ppd_list,new_ppd);
	return new_ppd;

}

void pfs_request_addNew(uint32_t pfs_fd,char* msgFromPFS, bool toSynchronize)
{
	//nipcMsg_t new_request_msg = NIPC_toMsg(msgFromPFS);

	uint32_t msg_len = *((uint16_t*) (msgFromPFS+1)) + 3;
	char *msg = malloc(msg_len);
	memcpy(msg,msgFromPFS,msg_len);

	pfs_request_t *new_pfsrequest = malloc(sizeof(pfs_request_t));
	new_pfsrequest->request_id =  *((uint32_t*) (msg+3));
	uint32_t sector = *((uint32_t*) (msg+7));

	new_pfsrequest->msg = msg;
	new_pfsrequest->pfs_fd = pfs_fd;

	queueNode_t *cur_ppd_node = ppd_list.begin;

	if (*msg == WRITE_SECTORS)
	{
		pthread_mutex_lock(&ppd_list_mutex);

		while (cur_ppd_node != NULL)
		{
			ppd_node_t *cur_ppd = (ppd_node_t*) cur_ppd_node->data;
			if ((cur_ppd->status == READY) ||(cur_ppd->status == SYNCHRONIZING && toSynchronize == true && sector < pending_writes_forSyncronization ))//Si llegan de WRITE mientras se sincroniza
			{
				pthread_mutex_lock(&cur_ppd->request_list_mutex);
				QUEUE_appendNode(&cur_ppd->request_list,new_pfsrequest);
				sem_post(&cur_ppd->request_list_sem);
				pthread_mutex_unlock(&cur_ppd->request_list_mutex);
			}
			cur_ppd_node = cur_ppd_node->next;
		}
		pthread_mutex_unlock(&ppd_list_mutex);
	}
	else
	{
		ppd_node_t *selected_ppd = PPDLIST_selectByLessRequests();
		pthread_mutex_lock(&selected_ppd->request_list_mutex);
			QUEUE_appendNode(&selected_ppd->request_list,new_pfsrequest);
			sem_post(&selected_ppd->request_list_sem);
		pthread_mutex_unlock(&selected_ppd->request_list_mutex);
	}
	return;
}

void pfs_request_free(pfs_request_t *request)
{
	free(request->msg);
	free(request);
	return;
}

ppd_node_t* PPDLIST_selectByLessRequests()//Recorre todos los pedidos (puede ser numero muy grande y consume tiempo)
{
	pthread_mutex_lock(&ppd_list_mutex);
	queueNode_t *cur_ppdnode = ppd_list.begin;
	uint32_t less = 9999999;
	ppd_node_t *selected_one = NULL;
	while (cur_ppdnode != NULL)
	{
		ppd_node_t *cur_ppd = (ppd_node_t*) cur_ppdnode->data;

		pthread_mutex_lock(&cur_ppd->request_list_mutex);
		size_t requests_number = QUEUE_length(&cur_ppd->request_list);
		pthread_mutex_unlock(&cur_ppd->request_list_mutex);
		if (requests_number < less && cur_ppd->status == READY)
		{
			selected_one = cur_ppd;
			less = requests_number;
		}

		cur_ppdnode = cur_ppdnode->next;
	}
	pthread_mutex_unlock(&ppd_list_mutex);
	return selected_one;
}

ppd_node_t* PPDLIST_getByFd(queue_t ppdlist,uint32_t fd)
{
	queueNode_t *cur_ppd_node = ppdlist.begin;
	while (cur_ppd_node != NULL)
	{
		ppd_node_t* cur_ppd = (ppd_node_t*) cur_ppd_node->data;
		if (cur_ppd->ppd_fd == fd) return cur_ppd;
		cur_ppd_node = cur_ppd_node->next;
	}
	return NULL;
}


ppd_node_t* PPDLIST_getByID(queue_t ppdlist,uint32_t diskID)
{
	queueNode_t *cur_ppd_node = ppdlist.begin;
	while (cur_ppd_node != NULL)
	{
		ppd_node_t* cur_ppd = (ppd_node_t*) cur_ppd_node->data;
		if (cur_ppd->disk_ID == diskID) return cur_ppd;
		cur_ppd_node = cur_ppd_node->next;
	}
	return NULL;
}

ppd_node_t* PPDLIST_getByStatus(queue_t ppdlist,uint32_t status)
{
	queueNode_t *cur_ppd_node = ppdlist.begin;
	while (cur_ppd_node != NULL)
	{
		ppd_node_t* cur_ppd = (ppd_node_t*) cur_ppd_node->data;
		if (cur_ppd->status == status) return cur_ppd;
		cur_ppd_node = cur_ppd_node->next;
	}
	return NULL;
}

bool PRAID_ValidatePPD(uint32_t diskID, uint32_t received_Sectors_Amount)
{
	if(sectors_perDisk > 0 ){
		if(sectors_perDisk < received_Sectors_Amount){
			print_Console("ERROR, DISCO CON MENOR CANTIDAD DE SECTORES",received_Sectors_Amount,1,true);
			return false;
		}
	}else{
		if(received_Sectors_Amount > 0){
			sectors_perDisk = received_Sectors_Amount; //No hace falta un mutex, el select va a hacer de a un pedido
		}else{
			print_Console("ERROR, CANTIDAD DE SECTORES NO VALIDA",pthread_self(),1,false);
			return false;
		}
		if(PPDLIST_getByID(ppd_list,diskID) != NULL){
			print_Console("ID DE DISCO YA EXISTENTE",diskID,1,true);
			return false;
		}
	}
	return true;
}



void PPDLIST_reorganizeRequests(uint32_t ppd_fd)
{
	queueNode_t *cur_ppd_node = ppd_list.begin;
	//queueNode_t *cur_ppd_node = PPDLIST_getByFd(ppd_list,ppd_fd);
	queueNode_t *prev_ppd_node = NULL;
/*
	pthread_mutex_lock(&pending_request_list_mutex);
	pthread_mutex_unlock(&pending_request_list_mutex);
*/
	pthread_mutex_lock(&ppd_list_mutex);
	while (cur_ppd_node != NULL)//TODO SI EL PPD ESTABA EN SINC QUE FLETE LOS PEDIDOS QUE SEAN DE SINC EN PENDING QUEUE
	{
		if (((ppd_node_t*)cur_ppd_node->data)->ppd_fd == ppd_fd)
		{
			if (prev_ppd_node != NULL)
			{
				prev_ppd_node->next = cur_ppd_node->next;
				if (prev_ppd_node->next == NULL) ppd_list.end = prev_ppd_node;
			}
			else
			{
				ppd_list.begin = cur_ppd_node->next;
				if (ppd_list.begin == NULL)
				{
					ppd_list.end = NULL;
				}
			}
			break;
		}
		prev_ppd_node = cur_ppd_node;
		cur_ppd_node = cur_ppd_node->next;
	}
	pthread_mutex_unlock(&ppd_list_mutex);

			ppd_node_t *selected_ppd = (ppd_node_t*) cur_ppd_node->data;//OJO Si no lo encuentra puede tirar SEG FAULT
			pthread_mutex_lock(&selected_ppd->request_list_mutex);
			queueNode_t *cur_request_node;
			while ((cur_request_node = QUEUE_takeNode(&selected_ppd->request_list)) != NULL)
			{
				pfs_request_t *cur_request = (pfs_request_t*) cur_request_node->data;
				if (*(cur_request->msg) != 0x02)
				{
					ppd_node_t *ppd = PPDLIST_selectByLessRequests();
					pthread_mutex_lock(&ppd->request_list_mutex);
					QUEUE_appendNode(&ppd->request_list,cur_request);
					sem_post(&ppd->request_list_sem);
					pthread_mutex_unlock(&ppd->request_list_mutex);
				}
				else
				{
					free(cur_request->msg);
					free(cur_request_node->data);
					free(cur_request_node);
				}
			}
			pthread_mutex_unlock(&selected_ppd->request_list_mutex);
			free(cur_ppd_node->data);
			free(cur_ppd_node);
}

void PPDLIST_handleDownPPD(ppd_node_t* cur_ppd_node)
{



	if(cur_ppd_node->status == READY)
	{
		if(ACTIVE_DISKS_AMOUNT<2){
			print_Console("Disco master desconectado",cur_ppd_node->disk_ID,1,true);
			exit(0);
		}else{
			//Redistribuir todos los pedidos
		}

	}
	else if(cur_ppd_node->status == SYNCHRONIZING)
	{
		//Resetear sincronizacion (eliminar pedidos )
		//Redistribuir pedidos

	}
	else
	{
		//Esta en wait, eliminalo nomas
	}

}
