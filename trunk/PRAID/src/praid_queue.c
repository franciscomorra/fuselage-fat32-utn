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

extern uint32_t RAID_STATUS; //0 INACTIVE - 1 ACTIVE
extern queue_t* WRITE_QUEUE;

extern pthread_mutex_t mutex_RAID_STATUS;
extern pthread_mutex_t mutex_WRITE_QUEUE;

extern struct praid_list_node* PRAID_LIST;
extern struct praid_list_node* CURRENT_READ;
extern t_log *raid_log_file;

praid_list_node* PRAID_list_appendNode(pthread_t tid, uint32_t self_socket)
{
	praid_list_node *nodoLISTA = malloc(sizeof(praid_list_node));
	praid_list_node_content *data = malloc(sizeof(praid_list_node_content));
	queue_t* subList = malloc(sizeof(queue_t));
	QUEUE_initialize(subList);
	data->tid = tid;
	data->socketPPD = self_socket;

	print_Console("Nuevo PPD: ",pthread_self());//CONSOLE NEW PPD
	log_debug(raid_log_file,"PRAID","Nuevo PPD(%s)",pthread_self());
	print_Console("El Socket Pasado como argumento es:",self_socket);

	if(PRAID_ActiveThreads_Amount() > 0){ //Hay mas de un disco
		data->ppdStatus = 2;//WaitSynch
		print_Console("Esperando para Sincronizacion: ",pthread_self());
		log_debug(raid_log_file,"PRAID","Esperando para Sincronizacion(%s)",pthread_self());
	}else{ //Primer Disco
		data->ppdStatus = 0;//Ready
		print_Console("RAID Activado por: ",pthread_self());
		log_debug(raid_log_file,"PRAID","RAID Activado(%s)",pthread_self());
	}
	data->colaSublista = subList;
	nodoLISTA->info = data;
	nodoLISTA->next = PRAID_LIST;
	PRAID_LIST = nodoLISTA;
	pthread_mutex_lock(&mutex_RAID_STATUS);
	if(RAID_STATUS==0){
		CURRENT_READ = PRAID_LIST;
	}
	return nodoLISTA;
	pthread_mutex_unlock(&mutex_RAID_STATUS);

}

uint32_t PRAID_Start_Synch(void)
{
	uint32_t first_sector = 0;
	print_Console("Iniciando Sincronizacion",pthread_self());
	log_debug(raid_log_file,"PRAID","Iniciando Sincronizacion(%s)",pthread_self());

	praid_sl_content *data_sublist= malloc(sizeof(praid_sl_content));
	data_sublist->synch = 1;
	data_sublist->msg = NIPC_createMsg(READ_SECTORS,sizeof(uint32_t),(char*) &first_sector);//TODO Verificar NIPC
	data_sublist->status = 0;
	data_sublist->socketPFS = 0;//Socket vacio, es de sincronizacion
	PRAID_ADD_READ(data_sublist);

return 0;
}



uint32_t PRAID_ADD_READ(praid_sl_content* data_sublist)
{
	data_sublist->status = 0;
	PRAID_actualizar_CurrentRead();
	QUEUE_appendNode(CURRENT_READ->info->colaSublista, data_sublist);

	if(data_sublist->synch == 1)
		print_Console("READ a COLA (Sincronizacion) de:",CURRENT_READ->info->tid);
	else
		print_Console("READ en COLA de:",CURRENT_READ->info->tid);

return 0;
}


uint32_t PRAID_actualizar_CurrentRead(void)
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
		}else if (aux->info->ppdStatus == 1){
			//TODO Si es que hay tiempo READ a disco en sincronizacion
			/*
			-Hay que guardar el current read del thread en el nodo lista
			-Me tienen que pasar el sector del pedido por parametro a la funcion
			-Si el sector es menor a current read y no esta en la queue con estado 2 o 3
			-Puedo tomarlo como valido
			*/
		}
	}
	return 1;
}


uint32_t PRAID_ADD_WRITE(praid_sl_content* data_sublist)
{
	praid_list_node* aux_list_node = PRAID_LIST;
	if(data_sublist->synch == 1){
			while(aux_list_node->info->ppdStatus != 1){
				if(aux_list_node->next == NULL){
					return 2; //ERROR NO HAY NODOS SINCRONIZANDOSE!
				}
				aux_list_node = aux_list_node->next;
			}

		data_sublist->status = 0;
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

uint32_t PRAID_clear_list_node(praid_list_node* nodo)//TODO Verificar Funcion clear_list_node
{
	while(QUEUE_length(nodo->info->colaSublista) > 0){
		queueNode_t* sl_node = QUEUE_takeNode(nodo->info->colaSublista);
		praid_sl_content* contenidoNodo=((praid_sl_content*) sl_node->data);
		if(contenidoNodo->synch==0){
			if(contenidoNodo->status!=3){
				if(contenidoNodo->msg.type == READ_SECTORS){
					PRAID_ADD_READ(sl_node->data);
				}else{
					//TODO Sacar ID Pedido del NIPC
					uint32_t requestID;
					pthread_mutex_lock(&mutex_WRITE_QUEUE);
					queueNode_t* nodoWRITE = PRAID_Search_WRITE(requestID);
					praid_read_content* contenidoNodoWrite=((praid_read_content*) nodoWRITE->data);
					contenidoNodoWrite->threads_left--;
					pthread_mutex_unlock(&mutex_WRITE_QUEUE);

					free (sl_node->data);
				}
			}else{//TODO Enviar a PFS, de READ y WRITE

			}
		}
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

uint32_t PRAID_ActiveThreads_Amount(void)
{
	uint32_t count = 0;
	praid_list_node* aux = PRAID_LIST;
	while(aux!=NULL){
		if(aux->info->ppdStatus !=2){//Si estan esperando, que se curtan
			count++;
		}
		aux = aux->next;
	}

	return count;
}

uint32_t PRAID_hay_discos_sincronizandose(void)
{
	praid_list_node* aux_list_node = PRAID_LIST;
	while(aux_list_node->next != NULL){ //Recorre toda la lista
		if(aux_list_node->info->ppdStatus == 1){//Se esta sincronizando
			return 0;//True
		}
		aux_list_node = aux_list_node->next;
	}

return 1;
}
//self_list_node->info->ppdStatus =1;

praid_list_node* PRAID_SearchBySocket(uint32_t socketBuscado)
{

praid_list_node *aux = PRAID_LIST;
	while (aux != NULL){
		if (aux->info->socketPPD == socketBuscado)	{
			return aux;
		}
		aux = aux->next;
	}
return NULL;
}

queueNode_t* PRAID_Search_WRITE(uint32_t requestID)
{
	queueNode_t *cur = WRITE_QUEUE->begin;
	while (cur != NULL)
	{
			//TODO sacar cada uno y volverlo a meter ordenado
			uint32_t idPedido;
			//TODO Sacar ID Pedido del NIPC
			if (idPedido == requestID){
				return cur;
			}
			cur = cur->next;
	}
	return NULL;
}
queueNode_t* PRAID_SearchSL(uint32_t requestID,queue_t* line)
{

if ((line)->begin != NULL)
{
	queueNode_t *cur = (line)->begin;
	while (cur != NULL)
	{
			//TODO sacar cada uno y volverlo a meter ordenado
			uint32_t idPedido;
			//TODO Sacar ID Pedido del NIPC
			if (idPedido == requestID)
			{
				return cur;
			}
			cur = cur->next;
	}
	return NULL;
 }
return NULL;
}
