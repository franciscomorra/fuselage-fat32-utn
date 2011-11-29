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

extern bool SYNCHRONIZING_DISCS;
extern uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
extern uint32_t ACTIVE_DISKS_AMOUNT;
extern uint32_t DISKS_AMOUNT;
extern bool RAID_ACTIVE; //0 INACTIVE - 1 ACTIVE
extern queue_t* WRITE_QUEUE;
extern pthread_mutex_t mutex_WRITE_QUEUE;
extern struct praid_list_node* PRAID_LIST;
extern pthread_mutex_t mutex_LIST;

extern struct praid_list_node* CURRENT_READ;

bool QUEUE_SEARCH_REMOVE(queueNode_t* current_node, queue_t* queue)
{

	if(current_node == NULL){
		print_Console("NODO NULL",pthread_self(),1,true);
		return false;
	}
	if(queue->begin == queue->end && queue->end == NULL){
		print_Console("ERROR COLA VACIA",pthread_self(),1,true);
		return false;
	}
	if(queue->begin == current_node){//SI ES EL PRIMERO
		queue->begin = current_node->next;
		if(queue->end == current_node){//SI TENGO SOLO UN NODO EN LA COLA
			queue->end = current_node->next;
		}
	}else{//SI ESTA A LA MITAD O AL FINAL
		queueNode_t* anterior = queue->begin;
		while(anterior != NULL && anterior->next != current_node){
			anterior = anterior->next;
		}
		if(anterior == NULL){
			print_Console("ERROR ELIMINANDO PEDIDO",pthread_self(),1,true);
			return false;
		}else{
			//(anterior->next==current_node){
			anterior->next = current_node->next;
			if(queue->end == current_node){
				queue->end = anterior;
			}
		}
	}
	return true;
}




praid_list_node* PRAID_ADD_PPD_NODE(pthread_t tid, praid_ppdThreadParam* mainParams)
{
	praid_list_node *nodoLISTA = malloc(sizeof(praid_list_node));
	queue_t* subList = malloc(sizeof(queue_t));
	QUEUE_initialize(subList);
	nodoLISTA->tid = tid;
	nodoLISTA->socketPPD = mainParams->socketPPD;
	nodoLISTA->diskID = mainParams->diskID;
	nodoLISTA->ammount_synch = 0;
	sem_init(&nodoLISTA->request_list_sem,0,0);

	free(mainParams);

	print_Console("NUEVO PPD",nodoLISTA->diskID,1,true);//CONSOLE NEW PPD
	PRAID_WRITE_LOG("Nuevo PPD");
	//log_debug(raid_log_file,"PRAID","Nuevo PPD");

	if(ACTIVE_DISKS_AMOUNT > 0){ //Hay mas de un disco
		nodoLISTA->ppdStatus = WAIT_SYNCH;
		sem_post(&nodoLISTA->request_list_sem);
	}else{ //Primer Disco
		nodoLISTA->ppdStatus = READY;
		RAID_ACTIVE = true;
		ACTIVE_DISKS_AMOUNT=1;
		print_Console("CANTIDAD DE SECTORES",DISK_SECTORS_AMOUNT,1,true);

		print_Console("----RAID ACTIVADO----\n",pthread_self(),1,false);
		PRAID_WRITE_LOG("RAID Activado");

		//log_debug(raid_log_file,"PRAID","RAID Activado");
	}
	DISKS_AMOUNT++;
	nodoLISTA->colaSublista = subList;

	nodoLISTA->next = PRAID_LIST;
	PRAID_LIST = nodoLISTA;
	if(RAID_ACTIVE==false){
		CURRENT_READ = PRAID_LIST;
	}
	print_Console("CANTIDAD DE DISCOS ACTIVOS",ACTIVE_DISKS_AMOUNT,1,true);

	return nodoLISTA;
}


praid_sl_content* NEW_SYNCH_REQUEST(uint32_t sector)
{
	uint32_t payloadSize = 2*(sizeof(uint32_t));
	char* new_payload = malloc(payloadSize);//CREO PEDIDO NUEVO DE LECTURA PARA EL PROXIMO SECTOR
	memcpy(new_payload,&sector,sizeof(uint32_t));//EL ID DEL PEDIDO VA A SER EL SECTOR
	memcpy(new_payload+(sizeof(uint32_t)),&sector,(sizeof(uint32_t)));

	praid_sl_content* new_request_data= malloc(sizeof(praid_sl_content));
	memcpy(new_request_data->msg.len,&payloadSize,2);

	new_request_data->msg.payload = new_payload;
	new_request_data->msg.type = READ_SECTORS;

	//= NIPC_createMsg(READ_SECTORS,payloadSize,new_payload);;
	new_request_data->synch = true;
	new_request_data->status = UNREAD;
	//free(new_payload);
	return new_request_data;
	//LE AGREGO EL PEDIDO A ALGUN DISCO READY

}
uint32_t PRAID_START_SYNCHR(uint32_t self_socket)
{
	uint32_t sector = 0;
	while(sector < DISK_SECTORS_AMOUNT){
		praid_sl_content* new_request_data = NEW_SYNCH_REQUEST(sector);
		pthread_mutex_lock(&mutex_LIST);
		PRAID_ADD_READ(new_request_data);
		pthread_mutex_unlock(&mutex_LIST);
		sector++;
	}
	/*
	praid_sl_content* new_request_data = NEW_SYNCH_REQUEST(sector);

	pthread_mutex_lock(&mutex_LIST);
	PRAID_ADD_READ(new_request_data);
	pthread_mutex_unlock(&mutex_LIST);

	*/
	ACTIVE_DISKS_AMOUNT++;
	return sector;


}



uint32_t PRAID_ADD_READ(praid_sl_content* data_sublist)
{
	data_sublist->status = 0;
	PRAID_REFRESH_CURRENT_READ();
	if(CURRENT_READ!=NULL){
		QUEUE_appendNode(CURRENT_READ->colaSublista, data_sublist);
//		print_Console("Agregando READ:",CURRENT_READ->tid);

		if(data_sublist->synch == true){
//			print_Console("READ a COLA (Sincronizacion) de:",CURRENT_READ->tid);
		}else{
//			print_Console("READ a COLA de:",CURRENT_READ->tid);
		}
		sem_post(&CURRENT_READ->request_list_sem);
		return 0;
	}else{
		print_Console("Error actualizando CURRENT_READ:",CURRENT_READ->diskID,1,true);
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
	if(data_sublist->synch == true){ //PARA SINCRONIZACION
		aux_list_node = PRAID_GET_SYNCH_PPD();
		if(aux_list_node!=NULL){
			QUEUE_appendNode(aux_list_node->colaSublista, data_sublist);
			sem_post(&aux_list_node->request_list_sem);
//			print_Console("WRITE a COLA (Sincronizacion) de:",aux_list_node->tid);
		}else{
			print_Console("NODO DE SINCRONIZACION NO ENCONTRADO",data_sublist->socketRequest,1,false);
			return 1; //ERROR NO HAY NODOS SINCRONIZANDOSE!

		}
	}else{//PEDIDO DE PFS
		if(PRAID_LIST != NULL){
			while(aux_list_node!=NULL){
				if(aux_list_node->ppdStatus==READY){//Si esta listo
					QUEUE_appendNode(aux_list_node->colaSublista, data_sublist);
					sem_post(&aux_list_node->request_list_sem);
				}else if(aux_list_node->ppdStatus==SYNCHRONIZING){
					uint32_t requestedSector;
					memcpy(&requestedSector,data_sublist->msg.payload+sizeof(uint32_t),sizeof(uint32_t));
					if(requestedSector < aux_list_node->ammount_synch){
						QUEUE_appendNode(aux_list_node->colaSublista, data_sublist);
						sem_post(&aux_list_node->request_list_sem);
						//print_Console("Sector pedido es menor que el currentWrite en sync",0);
					}
				}
				//Si hay tiempo se lee del que se esta sincronizando
				aux_list_node = aux_list_node->next;
			}
			print_Console("WRITE a COLA de TODOS LOS DISCOS",0,1,false);

			uint32_t IDpedido; //= NIPC_getID(data_sublist->msg);

			memcpy(&IDpedido,data_sublist->msg.payload,sizeof(uint32_t));
			praid_write_content* nodoREAD = malloc(sizeof(praid_write_content));
			nodoREAD->IDrequest = IDpedido;
			nodoREAD->threads_left = ACTIVE_DISKS_AMOUNT;
			QUEUE_appendNode(WRITE_QUEUE,nodoREAD);
		}else{
			print_Console("ERROR ADD WRITE, PRAID_LIST NULL",0,1,false);

			return 1; //ERROR PRAID_LIST VACIA
		}
	}
	return 0;
}

uint32_t PRAID_REMOVE_PPD(praid_list_node* nodo)
{

	queueNode_t* sl_node = QUEUE_takeNode(nodo->colaSublista);
	uint32_t count = 0;
	while(sl_node!=NULL){
		pthread_mutex_lock(&mutex_LIST);
		count++;
		praid_sl_content* contenidoNodoSL=((praid_sl_content*) sl_node->data);
		if(contenidoNodoSL->synch==false){
			if(contenidoNodoSL->status == RECEIVED){//Envialo al PFS
				char *msgToPFS = NIPC_toBytes(&contenidoNodoSL->msg);
				uint16_t msgToPFS_len = *((uint16_t*) contenidoNodoSL->msg.len);
				send(contenidoNodoSL->socketRequest,msgToPFS,msgToPFS_len+3,0);
				free(msgToPFS);
				free (contenidoNodoSL->msg.payload);
				free (contenidoNodoSL);
				queueNode_t* aux = sl_node;
				sl_node = sl_node->next;
				free (aux);

			}else{
				if(contenidoNodoSL->msg.type == READ_SECTORS){
					PRAID_ADD_READ(sl_node->data);
				}else if(contenidoNodoSL->msg.type == WRITE_SECTORS){

					uint32_t requestID;//= NIPC_getID(contenidoNodo->msg);
					memcpy(&requestID,contenidoNodoSL->msg.payload,sizeof(uint32_t));
					pthread_mutex_lock(&mutex_WRITE_QUEUE);
					queueNode_t* nodoWRITE = PRAID_GET_WRITE_NODE_BY_ID(requestID);
					praid_write_content* contenidoNodoWrite=((praid_write_content*) nodoWRITE->data);
					contenidoNodoWrite->threads_left--;
					if(contenidoNodoWrite->threads_left==0){
						char *msgToPFS = NIPC_toBytes(&contenidoNodoSL->msg);
						uint16_t msgToPFS_len = *((uint16_t*) contenidoNodoSL->msg.len);
						send(contenidoNodoSL->socketRequest,msgToPFS,msgToPFS_len+3,0);
						queueNode_t* write_remove_node = nodoWRITE;
						QUEUE_SEARCH_REMOVE(nodoWRITE,WRITE_QUEUE);
						free(contenidoNodoWrite);
						free(write_remove_node);
						send (contenidoNodoSL->socketRequest, msgToPFS, msgToPFS_len+3, 0);
						free(msgToPFS);
					}
					pthread_mutex_unlock(&mutex_WRITE_QUEUE);
					free (contenidoNodoSL->msg.payload);
					free (contenidoNodoSL);
				}
				queueNode_t* aux = sl_node;
				sl_node = sl_node->next;
				free (aux);
			}
		}else{//NODO PARA SINCRONIZACION
			char* payload = contenidoNodoSL->msg.payload;
			free(payload);
			free (contenidoNodoSL);
			queueNode_t* aux = sl_node;
			sl_node = sl_node->next;
			free (aux);
		}

	}
	print_Console("COLA DE PEDIDOS LIBERADA",nodo->diskID,2,true);
	print_Console("CANTIDAD DE PEDIDOS REDISTRIBUIDOS",count,2,true);

	//Sacar de la lista de ppds
	if(PRAID_LIST == nodo){
		PRAID_LIST = nodo->next;
	}else{
		praid_list_node* anterior = PRAID_LIST;
		while(anterior->next!=nodo && anterior != NULL){
			anterior = anterior->next;
		}
		if(anterior->next==nodo){
			anterior->next = nodo->next;
		}else{
			print_Console("Error al eliminar nodo de lista de PPDs",pthread_self(),1,true);
			return 1;
		}
	}

	free(nodo->colaSublista);
	free(nodo);
	pthread_mutex_unlock(&mutex_LIST);

	ACTIVE_DISKS_AMOUNT--;
	DISKS_AMOUNT--;
	print_Console("CANTIDAD DE DISCOS ACTIVOS: ",ACTIVE_DISKS_AMOUNT,2,true);

	if(SYNCHRONIZING_DISCS == true && ACTIVE_DISKS_AMOUNT==1){
		SYNCHRONIZING_DISCS = false;
	}
	if(ACTIVE_DISKS_AMOUNT==0){
		RAID_ACTIVE = false;//Lo reseteo
		print_Console("----RAID DESACTIVADO----",pthread_self(),1,false);
		if(DISKS_AMOUNT != 0){//HAY AL MENOS UN DISCO EN WAIT_SYNCH
			print_Console("HAY DISCOS ESPERANDO PARA SINCRONIZAR - APAGANDO RAID",DISKS_AMOUNT,1,false);
			exit(0);
		}
	}

	print_Console("\n ",ACTIVE_DISKS_AMOUNT,1,false);
	return 0;
}
bool PRAID_ACTIVATE_NEXT_SYNCH(void)
{
	praid_list_node* aux = PRAID_LIST;
	while(aux!=NULL){
		if(aux->ppdStatus == WAIT_SYNCH){
			sem_post(&aux->request_list_sem);
			return true;
		}
		aux = aux->next;
	}
	return false;
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
	praid_list_node *aux_list_node = PRAID_LIST;
	while (aux_list_node != NULL){
			if (aux_list_node->diskID == diskID){
				//print_Console("El disco ya existe!",diskID,1,true);
				return true;
			}
			aux_list_node = aux_list_node->next;
		}
	print_Console("ID DEL DISCO ES NUEVO",diskID,1,true);
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
print_Console("DISCO SINCRONIZACION NO ENCONTRADO",pthread_self(),1,false);
return NULL;
}

praid_list_node* PRAID_GET_PPD_FROM_FD(uint32_t socketBuscado)
{
praid_list_node *aux = PRAID_LIST;
	while (aux != NULL){
		if (aux->socketPPD == socketBuscado){
			//print_Console("ENCONTRADO DISCO",socketBuscado,2,true);
			return aux;
		}
		aux = aux->next;
	}
print_Console("Disco con ese Socket no encontrado!!",0,1,true);
return NULL;
}

queueNode_t* PRAID_GET_WRITE_NODE_BY_ID(uint32_t requestID)
{
	queueNode_t* current = WRITE_QUEUE->begin;
	while (current != NULL)
	{
		praid_write_content* current_sl_content =((praid_write_content*) current->data);
		if (current_sl_content->IDrequest == requestID){
			return current;
		}
		current = current->next;
	}
	return NULL;
}
queueNode_t* PRAID_GET_REQUEST_BY_ID(uint32_t requestID,queue_t* line)
{
	queueNode_t *cur = line->begin;
	while (cur != NULL)
	{
		praid_sl_content* current_sl_content =((praid_sl_content*) cur->data);
		uint32_t idPedido;
		memcpy(&idPedido,current_sl_content->msg.payload,sizeof(uint32_t));//TODO ERROR DE READ?

		if (idPedido == requestID){
			return cur;
		}
		cur = cur->next;
	}
	return NULL;
}
