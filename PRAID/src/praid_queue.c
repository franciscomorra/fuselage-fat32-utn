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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern bool RAID_ACTIVE; //0 INACTIVE - 1 ACTIVE
extern queue_t* WRITE_QUEUE;

extern pthread_mutex_t mutex_WRITE_QUEUE;

extern struct praid_list_node* PRAID_LIST;
extern struct praid_list_node* CURRENT_READ;
extern t_log *raid_log_file;

praid_list_node* PRAID_list_appendNode(pthread_t tid, uint32_t self_socket)
{
	praid_list_node *nodoLISTA = malloc(sizeof(praid_list_node));
	queue_t* subList = malloc(sizeof(queue_t));
	QUEUE_initialize(subList);
	nodoLISTA->tid = tid;
	nodoLISTA->socketPPD = self_socket;

	print_Console("Nuevo PPD: ",pthread_self());//CONSOLE NEW PPD
	log_debug(raid_log_file,"PRAID","Nuevo PPD");
	print_Console("El Socket Pasado como argumento es:",self_socket);

	if(PRAID_ActiveThreads_Amount() > 0){ //Hay mas de un disco
		nodoLISTA->ppdStatus = WAIT_SYNCH;
		print_Console("Esperando para Sincronizacion: ",pthread_self());
	}else{ //Primer Disco
		nodoLISTA->ppdStatus = READY;
		RAID_ACTIVE = true;
		print_Console("RAID Activado por: ",pthread_self());
		log_debug(raid_log_file,"PRAID","RAID Activado");
	}
	nodoLISTA->colaSublista = subList;

	nodoLISTA->next = PRAID_LIST;
	PRAID_LIST = nodoLISTA;
	if(RAID_ACTIVE==false){
		CURRENT_READ = PRAID_LIST;
	}
	return nodoLISTA;

}

uint32_t PRAID_Start_Synch(void)
{
	uint32_t first_sector = 0;
	print_Console("Iniciando Sincronizacion",pthread_self());
	log_debug(raid_log_file,"PRAID","Iniciando Sincronizacion");

	praid_sl_content *data_sublist= malloc(sizeof(praid_sl_content));
	data_sublist->synch = true;
	data_sublist->msg = NIPC_createMsg(READ_SECTORS,sizeof(uint32_t),(char*) &first_sector);//TODO Verificar NIPC (ID del pedido?)
	data_sublist->status = 0;
	data_sublist->socketPFS = 0;//Socket vacio, es de sincronizacion
	PRAID_ADD_READ(data_sublist);

return 0;
}



uint32_t PRAID_ADD_READ(praid_sl_content* data_sublist)
{
	data_sublist->status = 0;
	PRAID_actualizar_CurrentRead();
	QUEUE_appendNode(CURRENT_READ->colaSublista, data_sublist);

	if(data_sublist->synch == true)
		print_Console("READ a COLA (Sincronizacion) de:",CURRENT_READ->tid);
	else
		print_Console("READ en COLA de:",CURRENT_READ->tid);

return 0;
}


uint32_t PRAID_actualizar_CurrentRead(void)
{//Va alternando entre threads de PPD, consume menos recursos y da resultados practicamente iguales a ir buscando cual tiene menos pedidos
	praid_list_node* aux;

	aux = CURRENT_READ;

	while(1){
		if(aux == NULL){
			aux = PRAID_LIST;//Volve a empezar
		}
		aux = aux->next;
		if(aux->ppdStatus == READY){
			CURRENT_READ = aux;
			print_Console("CURRENT_READ apunta a: ",CURRENT_READ->tid);
			return 0;
		}else if (aux->ppdStatus == SYNCHRONIZING){
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


/*
 * Se puede si no reemplazar por esta que mando el amigo Carullo
	praid_list_node *cur_ppdnode = PRAID_LIST;

	uint32_t less = 9999999;
	CURRENT_READ = PRAID_LIST
	while (cur_ppdnode != NULL)
	{
		queue_t* cur_colaSublista = cur_ppdnode->colaSublista;
		size_t requests_number = QUEUE_length(&cur_colaSublista);

		if (requests_number < less && cur_ppdnode->ppdStatus == READY)
		{
			CURRENT_READ = cur_ppdnode;
			less = requests_number;
			return 0;
		}
		cur_ppdnode = cur_ppdnode->next;
	}
	return 1;
*/
}


uint32_t PRAID_ADD_WRITE(praid_sl_content* data_sublist)
{
	praid_list_node* aux_list_node = PRAID_LIST;
	if(data_sublist->synch == true){
			while(aux_list_node->ppdStatus != SYNCHRONIZING){
				if(aux_list_node->next == NULL){
					return 2; //ERROR NO HAY NODOS SINCRONIZANDOSE!
				}
				aux_list_node = aux_list_node->next;
			}

		data_sublist->status = 0;
		QUEUE_appendNode(aux_list_node->colaSublista, data_sublist);
		print_Console("WRITE a COLA (Sincronizacion) de:",aux_list_node->tid);
	}else{
		if(PRAID_LIST != NULL){
			while(aux_list_node!=NULL){
				if(aux_list_node->ppdStatus==READY){//Si esta listo
					QUEUE_appendNode(aux_list_node->colaSublista, data_sublist);
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
	while(QUEUE_length(nodo->colaSublista) > 0){
		queueNode_t* sl_node = QUEUE_takeNode(nodo->colaSublista);
		praid_sl_content* contenidoNodo=((praid_sl_content*) sl_node->data);
		if(contenidoNodo->synch==false){
			if(contenidoNodo->status!=3){
				if(contenidoNodo->msg.type == READ_SECTORS){
					PRAID_ADD_READ(sl_node->data);
				}else if(contenidoNodo->msg.type == WRITE_SECTORS){
					uint32_t requestID = NIPC_getID(contenidoNodo->msg);
					pthread_mutex_lock(&mutex_WRITE_QUEUE);
					queueNode_t* nodoWRITE = PRAID_Search_WRITE_Queue(requestID);
					praid_read_content* contenidoNodoWrite=((praid_read_content*) nodoWRITE->data);
					contenidoNodoWrite->threads_left--;
					if(contenidoNodoWrite->threads_left==0){
						char *msgToPFS = NIPC_toBytes(&contenidoNodo->msg);
						uint16_t msgToPFS_len = *((uint16_t*) contenidoNodo->msg.len);
						send(nodo->socketPPD,msgToPFS,msgToPFS_len+3,0);
						free(msgToPFS);
					}
					pthread_mutex_unlock(&mutex_WRITE_QUEUE);

				}
			}else{
				char *msgToPFS = NIPC_toBytes(&contenidoNodo->msg);
				uint16_t msgToPFS_len = *((uint16_t*) contenidoNodo->msg.len);
				send(nodo->socketPPD,msgToPFS,msgToPFS_len+3,0);
				free(msgToPFS);
			}
			free (contenidoNodo);

		}
		free (sl_node);
	}
	praid_list_node* aux = PRAID_LIST;
	while(aux->next!=nodo){//Sacar de la lista de ppds
		if(aux->next == NULL){
			return 1; //ERROR NODO NO ENCONTRADO!
		}
		aux = aux->next;
	}
	aux->next = nodo->next;
	free(nodo->colaSublista);
	free(nodo);
return 0;
}

uint32_t PRAID_ActiveThreads_Amount(void)
{
	uint32_t count = 0;
	praid_list_node* aux = PRAID_LIST;
	while(aux!=NULL){
		if(aux->ppdStatus != WAIT_SYNCH && aux->ppdStatus != DISCONNECTED){
			count++;
		}
		aux = aux->next;
	}

	return count;
}

bool PRAID_hay_discos_sincronizandose(void)
{
	praid_list_node* aux_list_node = PRAID_LIST;
	while(aux_list_node->next != NULL){ //Recorre toda la lista
		if(aux_list_node->ppdStatus == SYNCHRONIZING){//Se esta sincronizando
			return true;
		}
		aux_list_node = aux_list_node->next;
	}

return false;
}

praid_list_node* PRAID_SearchPPDBySocket(uint32_t socketBuscado)
{

praid_list_node *aux = PRAID_LIST;
	while (aux != NULL){
		if (aux->socketPPD == socketBuscado)	{
			return aux;
		}
		aux = aux->next;
	}
return NULL;
}

queueNode_t* PRAID_Search_WRITE_Queue(uint32_t requestID)
{
	queueNode_t *cur = WRITE_QUEUE->begin;
	while (cur != NULL)
	{
		praid_read_content* current_sl_content =((praid_read_content*) cur->data);

		if (current_sl_content->IDrequest == requestID){
			return cur;
		}
		cur = cur->next;
	}
	return NULL;
}
queueNode_t* PRAID_Search_Requests_SL(uint32_t requestID,queue_t* line)
{
	queueNode_t *cur = line->begin;
	while (cur != NULL)
	{
		praid_sl_content* current_sl_content =((praid_sl_content*) cur->data);

		uint32_t idPedido = NIPC_getID(current_sl_content->msg);
		if (idPedido == requestID)
		{
			return cur;
		}
	}
	return NULL;
}

uint32_t NIPC_getID(nipcMsg_t msg)
{
	//TODO Sacar ID Pedido del NIPC
return 0;
}

