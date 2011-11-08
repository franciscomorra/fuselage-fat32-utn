/*
 * praid_queue.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include "praid_queue.h"
#include "tad_queue.h"
#include "praid_console.h"
#include "log.h"

extern uint32_t RAID_STATUS; //0 INACTIVE - 1 ACTIVE - 2 WAIT_FIRST_PPD_REPLY
extern uint32_t DISK_SECTORS_AMOUNT;

extern pthread_mutex_t mutex_RAID_STATUS;

extern struct praid_list_node* PRAID_LIST;
extern struct praid_list_node* CURRENT_READ;
extern t_log *raid_log_file;

praid_list_node* PRAID_list_appendNode(uint32_t tid)//TODO pasarle el socket!
{
	praid_list_node *nodoLISTA = malloc(sizeof(praid_list_node));
	praid_list_node_content *data = malloc(sizeof(praid_list_node_content));
	queue_t* subList = malloc(sizeof(queue_t));
	QUEUE_initialize(subList);
	data->tid = tid;
	//TODO SOCKET PPD!

	print_Console("Nuevo PPD: ",(uint32_t)pthread_self());//CONSOLE NEW PPD
	log_debug(raid_log_file,"PRAID","Nuevo PPD(%s)",(uint32_t)pthread_self());

	if(PRAID_ppd_thread_count() > 0){ //Hay mas de un disco
		data->ppdStatus = 2;//WaitSynch
		print_Console("Esperando para Sincronizacion: ",(uint32_t)pthread_self());
		log_debug(raid_log_file,"PRAID","Esperando para Sincronizacion(%s)",(uint32_t)pthread_self());
	}else{ //Primer Disco
		//TODO Handshake, que devuelva el tamaño de disco DISK_SECTORS_AMOUNT
		data->ppdStatus = 0;//Ready
		pthread_mutex_lock(&mutex_RAID_STATUS);
		RAID_STATUS = 1; //RAID ACTIVADO
		pthread_mutex_unlock(&mutex_RAID_STATUS);
		print_Console("RAID Activado por: ",(uint32_t)pthread_self());
		log_debug(raid_log_file,"PRAID","RAID Activado(%s)",pthread_self());
	}
	data->colaSublista = subList;
	nodoLISTA->info = data;
	nodoLISTA->next = PRAID_LIST;
	PRAID_LIST = nodoLISTA;
	if(RAID_STATUS!=0){
		CURRENT_READ = PRAID_LIST;
	}
	return nodoLISTA;
}

uint32_t PRAID_Start_Synch()
{
	uint32_t first_sector = 0;
	print_Console("Iniciando Sincronizacion",(uint32_t)pthread_self());
	log_debug(raid_log_file,"PRAID","Iniciando Sincronizacion(%s)",(uint32_t)pthread_self());

	praid_sl_content *data_sublist= malloc(sizeof(praid_sl_content));
	data_sublist->synch = 1;
	//data_sublist->msg.type = READ_SECTORS;//Hardcodeado, cambiar a NIPC cuando este listo
	data_sublist->msg = NIPC_createMsg(READ_SECTORS,sizeof(uint32_t),(char*) &first_sector);

	//SOCKET PPD
	//NIPC READ
	PRAID_add_READ_Request(data_sublist);

return 0;
}



uint32_t PRAID_add_READ_Request(praid_sl_content* data_sublist)
{

	PRAID_actualizar_CurrentRead();
	if(data_sublist->synch == 1)
		print_Console("READ a COLA (Sincronizacion) de:",CURRENT_READ->info->tid);
	else
		print_Console("READ en COLA de:",CURRENT_READ->info->tid);

	QUEUE_appendNode(CURRENT_READ->info->colaSublista, data_sublist);
return 0;
}


uint32_t PRAID_actualizar_CurrentRead()
{
	praid_list_node* aux;
	aux = CURRENT_READ;

	while(1){
		aux = aux->next;
		if(aux == NULL)	aux = PRAID_LIST;//Volve a empezar
		if(aux->info->ppdStatus == 0){
			CURRENT_READ = aux;
			print_Console("CURRENT_READ apunta a: ",CURRENT_READ->info->tid);

			return 0;
		}
	}
	return 1;
}


uint32_t PRAID_add_WRITE_Request(praid_sl_content* data_sublist)
{
	praid_list_node* aux_list_node = PRAID_LIST;
	if(data_sublist->synch == 1){
			while(aux_list_node->info->ppdStatus != 1){
				if(aux_list_node->next == NULL){
					return 2; //ERROR NO HAY NODOS SINCRONIZANDOSE!
				}
				aux_list_node = aux_list_node->next;
			}
		QUEUE_appendNode(aux_list_node->info->colaSublista, data_sublist);
		print_Console("WRITE a COLA (Sincronizacion) de:",aux_list_node->info->tid);
	}else{
		if(PRAID_LIST != NULL){
			while(aux_list_node!=NULL){
				if(aux_list_node->info->ppdStatus==0){//Si esta listo
					QUEUE_appendNode(aux_list_node->info->colaSublista, data_sublist);
					print_Console("WRITE a COLA de TODOS LOS DISCOS",0);

				}//Si hay tiempo se lee del que se esta sincronizando
				aux_list_node = aux_list_node->next;
			}
		}else{
			return 1; //ERROR PRAID_LIST VACIA
		}
	}
	return 0;
}

uint32_t PRAID_clear_list_node(praid_list_node* nodo)
{
	while(QUEUE_length(nodo->info->colaSublista) > 0){
		queueNode_t* sl_node = QUEUE_takeNode(nodo->info->colaSublista);
		PRAID_add_READ_Request(sl_node->data);
		free (sl_node);
	}
	praid_list_node* aux = PRAID_LIST;
	while(aux->next!=nodo){
		if(aux->next == NULL){
			return 1; //ERROR NODO NO ENCONTRADO!
		}
		aux = aux->next;
	}
	aux->next = nodo->next;
	free(nodo->info->colaSublista);
	free(nodo->info);
	free(nodo);
return 0;
}

uint32_t PRAID_ppd_thread_count(void)
{
	uint32_t count = 0;
	praid_list_node* aux = PRAID_LIST;
	while(aux!=NULL){
		aux = aux->next;
		count++;
	}

	return count;
}

uint32_t PRAID_hay_discos_sincronizandose()
{
	uint32_t flag = 1;//False
	praid_list_node* aux_list_node = PRAID_LIST;
	while(aux_list_node->next != NULL){ //Recorre toda la lista
		if(aux_list_node->info->ppdStatus == 1){//Se esta sincronizando
			flag = 0;//True
		}
		aux_list_node = aux_list_node->next;
	}

return flag;
}
//self_list_node->info->ppdStatus =1;




/*
TODO Destruir nodo PRAID_LIST (en caso de fallo de disco, tiene que repartir pedidos pendientes)
*/
