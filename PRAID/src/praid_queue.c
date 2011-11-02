/*
 * praid_queue.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include "praid_queue.h"
#include "tad_queue.h"

extern uint32_t RAID_STATUS; //0 INACTIVE - 1 ACTIVE - 2 WAIT_FIRST_PPD_REPLY
extern struct praid_list_node* PRAID_LIST;
extern struct praid_list_node* CURRENT_READ;

praid_list_node* PRAID_list_appendNode(uint32_t tid)//TODO pasarle el socket!
{
	praid_list_node *nodoLISTA = malloc(sizeof(praid_list_node));
	praid_list_node_content *data = malloc(sizeof(praid_list_node_content));
	queue_t* subList = malloc(sizeof(queue_t));
	QUEUE_initialize(subList);
	data->tid = tid;
	//TODO SOCKET PPD!

	if(RAID_STATUS!=0){
		data->ppdStatus = 1;//Sincronizando
		praid_sl_content *data_sublist= malloc(sizeof(praid_sl_content));
		data_sublist->synch = 1;
		//SOCKET PPD
		//NIPC Type WRITE_SECTORS
		/*
		queueNode_t *subListNode = malloc(sizeof(queueNode_t));
		subListNode->data = (praid_sl_content*)data_sublist;
		subListNode->next = NULL;
		subList->begin = subList->end = subListNode;
		*/
		QUEUE_appendNode(subList, data_sublist);
	}else{
		data->ppdStatus = 0;//Ready
	}
/*
	praid_sl_content data_SL_memcpy;
	memcpy(&data_SL_memcpy,subListNode->data,sizeof(praid_sl_content));
*/
	data->colaSublista = subList;
	nodoLISTA->info = data;
	nodoLISTA->next = PRAID_LIST;
	PRAID_LIST = nodoLISTA;
	if(RAID_STATUS!=0){
		CURRENT_READ = PRAID_LIST;
	}
	return nodoLISTA;
}

uint32_t PRAID_add_READ_Request(praid_sl_content* data_sublist)
{
	if(CURRENT_READ != NULL){
		if(CURRENT_READ->info->ppdStatus!=1){//Si no se esta sincronizando
			QUEUE_appendNode(CURRENT_READ->info->colaSublista, data_sublist);
		}//Si hay tiempo se lee del que se esta sincronizando
		if(CURRENT_READ->next!=NULL)
		CURRENT_READ = CURRENT_READ->next;
		else
		CURRENT_READ = PRAID_LIST;
		return 0;
	}else{
		return 1; //ERROR CURRENT_READ VACIA
	}
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

	}else{
		if(PRAID_LIST != NULL){
			while(aux_list_node!=NULL){
				if(aux_list_node->info->ppdStatus!=1){//Si no se esta sincronizando
					QUEUE_appendNode(aux_list_node->info->colaSublista, data_sublist);
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
	while(aux!=nodo){
		if(aux->next == NULL){
			return 1; //ERROR NODO NO ENCONTRADO!
		}
		aux = aux->next;
	}
	aux->next = nodo->next;
	free(nodo->info);
	free(nodo);
return 0;
}


/*
Agregar nodo SUBLISTA Tipo Read (Usa un puntero que va recorriendo)
Agregar nodo SUBLISTA Tipo Write (A todos los nodos)
Destruir nodo PRAID_LIST (en caso de fallo de disco, tiene que repartir pedidos pendientes)
Destruir nodo SUBLISTA
*/
