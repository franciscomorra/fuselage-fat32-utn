/*
 * praid_queue.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "praid_queue.h"
#include "tad_queue.h"
#include "praid_console.h"
#include "log.h"

extern bool SYNCHRONIZING_DISCS; //0 INACTIVE - 1 ACTIVE

extern bool RAID_ACTIVE; //0 INACTIVE - 1 ACTIVE
extern queue_t* WRITE_QUEUE;

extern pthread_mutex_t mutex_WRITE_QUEUE;

extern struct praid_list_node* PRAID_LIST;
extern struct praid_list_node* CURRENT_READ;
extern t_log *raid_log_file;

praid_list_node* PRAID_ADD_PPD_NODE(pthread_t tid, praid_ppdThreadParam* mainParams)
{
	praid_list_node *nodoLISTA = malloc(sizeof(praid_list_node));
	queue_t* subList = malloc(sizeof(queue_t));
	QUEUE_initialize(subList);
	nodoLISTA->tid = tid;
	nodoLISTA->socketPPD = mainParams->socketPPD;
	nodoLISTA->diskID = mainParams->diskID;
	nodoLISTA->ammount_synch = 0;
	sem_init(&nodoLISTA->request_list_sem,NULL,0);

	free(mainParams);

	print_Console("NUEVO PPD",pthread_self(),1,true);//CONSOLE NEW PPD
	log_debug(raid_log_file,"PRAID","Nuevo PPD");

	if(PRAID_ACTIVE_PPD_COUNT() > 0){ //Hay mas de un disco
		nodoLISTA->ppdStatus = WAIT_SYNCH;
		print_Console("PPD ESPERANDO PARA SINCRONIZACION ",pthread_self(),1,true);
		sem_post(&nodoLISTA->request_list_sem);
	}else{ //Primer Disco
		nodoLISTA->ppdStatus = READY;
		RAID_ACTIVE = true;
		print_Console("----RAID ACTIVADO----\n",pthread_self(),1,false);
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

uint32_t PRAID_START_SYNCHR(uint32_t self_socket)
{
	uint32_t first_sector = 0;
	uint32_t idpedido = 0;

	uint32_t size = sizeof(uint32_t);
	int size2 = 2*size;
	char* msgOut = malloc(size2);

	memcpy(msgOut,&idpedido,size);
	memcpy(msgOut+size,&first_sector,size);


	print_Console("INICIANDO SINCRONIZACION",pthread_self(),1,true);
	log_debug(raid_log_file,"PRAID","Iniciando Sincronizacion");

	praid_sl_content *data_sublist= malloc(sizeof(praid_sl_content));
	data_sublist->synch = true;
	data_sublist->msg = NIPC_createMsg(READ_SECTORS,size2,msgOut);
	free(msgOut);
	data_sublist->status = 0;
	data_sublist->socketRequest = self_socket;//Socket vacio, es de sincronizacion

	PRAID_ADD_READ(data_sublist);

return 0;
}



uint32_t PRAID_ADD_READ(praid_sl_content* data_sublist)
{
	data_sublist->status = 0;
	PRAID_REFRESH_CURRENT_READ();
	if(CURRENT_READ!=NULL){
		QUEUE_appendNode(CURRENT_READ->colaSublista, data_sublist);
	//	print_Console("Agregando READ:",CURRENT_READ->tid);

		if(data_sublist->synch == true){
//			print_Console("READ a COLA (Sincronizacion) de:",CURRENT_READ->tid);
		}else{
//			print_Console("READ a COLA de:",CURRENT_READ->tid);
		}
		sem_post(&CURRENT_READ->request_list_sem);
		return 0;
	}else{
		print_Console("Error actualizando CURRENT_READ:",CURRENT_READ->tid,1,true);
		return 1;
	}
}


uint32_t PRAID_REFRESH_CURRENT_READ(void)
{//Va alternando entre threads de PPD, consume menos recursos y da resultados practicamente iguales a ir buscando cual tiene menos pedidos

	while(PRAID_LIST!=NULL){
		if(CURRENT_READ == NULL){
			CURRENT_READ=PRAID_LIST;
		}
		if(CURRENT_READ->next!=NULL){
			CURRENT_READ = CURRENT_READ->next;
		}
		if(CURRENT_READ->ppdStatus == READY){
//			print_Console("CURRENT_READ apunta a: ",CURRENT_READ->tid);
			return 0;
		}else if (CURRENT_READ->ppdStatus == SYNCHRONIZING){

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
}
*/

uint32_t PRAID_ADD_WRITE(praid_sl_content* data_sublist)
{
	praid_list_node* aux_list_node = PRAID_LIST;
	if(data_sublist->synch == true){

		//aux_list_node = PRAID_GET_PPD_FROM_FD(data_sublist->socketRequest);
		aux_list_node = PRAID_GET_SYNCH_PPD();

		if(aux_list_node!=NULL){
			QUEUE_appendNode(aux_list_node->colaSublista, data_sublist);
			sem_post(&aux_list_node->request_list_sem);
//			print_Console("WRITE a COLA (Sincronizacion) de:",aux_list_node->tid);
		}else{
			print_Console("ERROR AGREGANDO ESCRITURA PARA SINCRONIZACION ",pthread_self(),1,false);
			return 1; //ERROR NO HAY NODOS SINCRONIZANDOSE!

		}
	}else{
		if(PRAID_LIST != NULL){
			while(aux_list_node!=NULL){
				if(aux_list_node->ppdStatus==READY){//Si esta listo
					QUEUE_appendNode(aux_list_node->colaSublista, data_sublist);
					sem_post(&aux_list_node->request_list_sem);
				}else if(aux_list_node->ppdStatus==SYNCHRONIZING){
					uint32_t requestedSector;
					memcpy(&requestedSector,data_sublist->msg.payload+4,4);
					if(requestedSector < aux_list_node->ammount_synch){
						QUEUE_appendNode(aux_list_node->colaSublista, data_sublist);
						sem_post(&aux_list_node->request_list_sem);
						//print_Console("Sector pedido es menor que el currentWrite en sync",0);
					}
				}
				//Si hay tiempo se lee del que se esta sincronizando
				aux_list_node = aux_list_node->next;
			}
			print_Console("WRITE a COLA de TODOS LOS DISCOS",0,1,true);

			uint32_t IDpedido; //= NIPC_getID(data_sublist->msg);

			memcpy(&IDpedido,data_sublist->msg.payload,4);
			praid_write_content* nodoREAD = malloc(sizeof(praid_write_content));
			nodoREAD->IDrequest = IDpedido;
			nodoREAD->threads_left = PRAID_ACTIVE_PPD_COUNT();
			pthread_mutex_lock(&mutex_WRITE_QUEUE);
			QUEUE_appendNode(WRITE_QUEUE,nodoREAD);
			pthread_mutex_unlock(&mutex_WRITE_QUEUE);
		}else{
			print_Console("ERROR ADD WRITE, PRAID_LIST NULL",0,1,true);

			return 1; //ERROR PRAID_LIST VACIA
		}
	}
	return 0;
}

uint32_t PRAID_REMOVE_PPD(praid_list_node* nodo)
{
	while(QUEUE_length(nodo->colaSublista) > 0){
		queueNode_t* sl_node = QUEUE_takeNode(nodo->colaSublista);
		praid_sl_content* contenidoNodoSL=((praid_sl_content*) sl_node->data);
		if(contenidoNodoSL->synch==false){
			if(contenidoNodoSL->status!=RECEIVED){
				if(contenidoNodoSL->msg.type == READ_SECTORS){
					PRAID_ADD_READ(sl_node->data);
				}else if(contenidoNodoSL->msg.type == WRITE_SECTORS){
					uint32_t requestID;//= NIPC_getID(contenidoNodo->msg);
					memcpy(&requestID,contenidoNodoSL->msg.payload,4);

					pthread_mutex_lock(&mutex_WRITE_QUEUE);
					queueNode_t* nodoWRITE = PRAID_GET_WRITE_NODE_BY_ID(requestID);
					praid_write_content* contenidoNodoWrite=((praid_write_content*) nodoWRITE->data);
					contenidoNodoWrite->threads_left--;
					if(contenidoNodoWrite->threads_left==0){
						char *msgToPFS = NIPC_toBytes(&contenidoNodoSL->msg);
						uint16_t msgToPFS_len = *((uint16_t*) contenidoNodoSL->msg.len);
						send(nodo->socketPPD,msgToPFS,msgToPFS_len+3,0);
						free(msgToPFS);
					}
					pthread_mutex_unlock(&mutex_WRITE_QUEUE);

				}
			}else{
				SYNCHRONIZING_DISCS = false;
				char *msgToPFS = NIPC_toBytes(&contenidoNodoSL->msg);
				uint16_t msgToPFS_len = *((uint16_t*) contenidoNodoSL->msg.len);
				send(nodo->socketPPD,msgToPFS,msgToPFS_len+3,0);
				free(msgToPFS);
			}
			free (contenidoNodoSL);

		}
		free (sl_node);
	}
	print_Console("COLA DE PEDIDOS LIBERADA",pthread_self(),1,true);

	//Sacar de la lista de ppds
	if(PRAID_LIST == nodo){
		PRAID_LIST = nodo->next;
	}else{
		praid_list_node* anterior = PRAID_LIST;
		do{
			anterior = anterior->next;
		}while(anterior->next!=nodo || anterior == NULL);
		if(anterior->next==nodo){
			anterior->next = nodo->next;
		}else{
			print_Console("Error al eliminar nodo de lista de PPDs",pthread_self(),1,true);
			return 1;
		}
	}
	free(nodo->colaSublista);
	free(nodo);
	print_Console("CANTIDAD DE DISCOS ACTIVOS: ",PRAID_ACTIVE_PPD_COUNT(),1,true);

	if(SYNCHRONIZING_DISCS ==true && PRAID_ACTIVE_PPD_COUNT()==1){
		SYNCHRONIZING_DISCS = false;
	}
	if(PRAID_ACTIVE_PPD_COUNT()==0){
		RAID_ACTIVE = false;//Lo reseteo
		print_Console("----RAID DESACTIVADO----",pthread_self(),1,false);

	}
	print_Console("\n ",PRAID_ACTIVE_PPD_COUNT(),1,false);

	return 0;
}

uint32_t PRAID_ACTIVE_PPD_COUNT(void)
{
	if(PRAID_LIST == NULL){
		return 0;
	}
	uint32_t count = 0;
	praid_list_node* aux = PRAID_LIST;
	do{
		if(aux->ppdStatus != WAIT_SYNCH && aux->ppdStatus != DISCONNECTED){
			count++;
		}
		aux = aux->next;
	}while(aux!=NULL);
	return count;
}

bool PRAID_DISK_ID_EXISTS(uint32_t diskID)
{
	if(PRAID_LIST==NULL){
		//print_Console("El ID del disco es nuevo",diskID);
		return false;
	}

	praid_list_node* aux_list_node = PRAID_LIST;
	do{
		if(aux_list_node->diskID == diskID){
			print_Console("El disco ya existe!",diskID,1,true);
			return true;
		}
		aux_list_node = aux_list_node->next;
	}while(aux_list_node != NULL);
print_Console("El ID del disco es nuevo",diskID,1,true);
return false;
}

praid_list_node* PRAID_GET_SYNCH_PPD(void)
{
praid_list_node *aux = PRAID_LIST;
	while (aux != NULL){
		if (aux->ppdStatus== SYNCHRONIZING){
		//	print_Console("Encontrado disco",aux->tid);
			return aux;
		}
		aux = aux->next;
	}
print_Console("DISCO SINCRONIZACION NO ENCONTRADO",pthread_self(),1,true);
return NULL;
}

praid_list_node* PRAID_GET_PPD_FROM_FD(uint32_t socketBuscado)
{
praid_list_node *aux = PRAID_LIST;
	while (aux != NULL){
		if (aux->socketPPD == socketBuscado){
			//print_Console("Encontrado disco",aux->tid);
			return aux;
		}
		aux = aux->next;
	}
print_Console("Disco con ese Socket no encontrado!!",0,1,true);
return NULL;
}

queueNode_t* PRAID_GET_WRITE_NODE_BY_ID(uint32_t requestID)
{
	queueNode_t *cur = WRITE_QUEUE->begin;
	while (cur != NULL)
	{
		praid_write_content* current_sl_content =((praid_write_content*) cur->data);

		if (current_sl_content->IDrequest == requestID){
			return cur;
		}
		cur = cur->next;
	}
	return NULL;
}
queueNode_t* PRAID_GET_REQUEST_BY_ID(uint32_t requestID,queue_t* line)
{
	queueNode_t *cur = line->begin;
	while (cur != NULL)
	{
		praid_sl_content* current_sl_content =((praid_sl_content*) cur->data);
		uint32_t idPedido=0; //= NIPC_getID(current_sl_content->msg);
		memcpy(&idPedido,current_sl_content->msg.payload,4);

		if (idPedido == requestID)
		{
			return cur;
		}
		cur = cur->next;
	}
	return NULL;
}
/*
uint32_t NIPC_getID(nipcMsg_t msg)
{
	nipcMsg_t* aux = malloc(sizeof(nipcMsg_t));
	char* mensaje = NIPC_toBytes(aux);
	free(aux);
	uint32_t messageID;
	memcpy(&messageID,mensaje+3,4);
	free(mensaje);
	return messageID;
}
*/
/*

bool PRAID_hay_discos_sincronizandose(void)
{

	praid_list_node* aux_list_node = PRAID_LIST;
	while(aux_list_node->next != NULL){ //Recorre toda la lista
		if(aux_list_node->ppdStatus == SYNCHRONIZING){//Se esta sincronizando
			print_Console("Ya hay un disco sincronizandose",pthread_self(),1,true);

			return true;
		}
		aux_list_node = aux_list_node->next;
	}
print_Console("No hay discos sincronizandose",pthread_self(),1,true);

return false;
	return SYNCHRONIZING_DISCS;
}
*/
