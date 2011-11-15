/*
 * praid_comm.c
 *
 *  Created on: 26/09/2011
 *      Author: utn_so
 */

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nipc.h"
#include "praid_comm.h"
#include "praid_queue.h"
#include "praid_ppd_handler.h"

#include <errno.h>
#define PORT 9333
extern bool RAID_ACTIVE;
extern uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
extern queue_t* WRITE_QUEUE;

extern pthread_mutex_t mutex_LIST;
extern pthread_mutex_t mutex_WRITE_QUEUE;

#include "comm.h"
//extern struct praid_list_node* PRAID_LIST;
//extern pthread_mutex_t mutex_LIST;



praid_ppdThreadParam* PRAID_ValidatePPD(char* msgIn, uint32_t newPPD_FD)
{
	uint16_t len;
	memcpy(msgIn+1,&len,2);
	if(len == 8){
		uint32_t disk_sectors_rcv;
		memcpy(msgIn+7,&disk_sectors_rcv,4);

		if(RAID_ACTIVE==true){
			if(disk_sectors_rcv!=DISK_SECTORS_AMOUNT){
				return NULL;
			}

		}else{
			if(disk_sectors_rcv > 0){
				DISK_SECTORS_AMOUNT = disk_sectors_rcv; //No hace falta un mutex, el select va a hacer de a un pedido
			}else{
				return NULL;
			}
		}
		uint32_t diskID;
		memcpy(msgIn+3,&diskID,4);
		pthread_mutex_lock(&mutex_LIST);
		if(PRAID_discoExiste(diskID)){
			return NULL;
		}
		pthread_mutex_unlock(&mutex_LIST);

		praid_ppdThreadParam* parameters = malloc(sizeof(praid_ppdThreadParam));
		parameters->diskID = diskID;
		parameters->socketPPD = newPPD_FD;
		return parameters;

	}
return NULL;

}



uint32_t PRAID_pfs_receive(char* msgIn,uint32_t fd)
{
	praid_sl_content* data_sublist = malloc(sizeof(praid_sl_content));
	data_sublist->synch = false;
	data_sublist->msg = NIPC_toMsg(msgIn);
	data_sublist->socketPFS = fd;

	switch (msgIn[0])
	{
		//Pedido de READ:
		case READ_SECTORS:{
			pthread_mutex_lock(&mutex_LIST);
			PRAID_ADD_READ(data_sublist);
			pthread_mutex_unlock(&mutex_LIST);

		}
		break;
		//Pedido de WRITE:
		case WRITE_SECTORS:{
			pthread_mutex_lock(&mutex_LIST);
			PRAID_ADD_WRITE(data_sublist);
			pthread_mutex_unlock(&mutex_LIST);

			praid_read_content* nodoREAD = malloc(sizeof(praid_read_content));

			uint32_t IDpedido = NIPC_getID(data_sublist->msg);

			nodoREAD->IDrequest = IDpedido;
			nodoREAD->threads_left = PRAID_ActiveThreads_Amount();
			pthread_mutex_lock(&mutex_WRITE_QUEUE);
			QUEUE_appendNode(WRITE_QUEUE,nodoREAD);
			pthread_mutex_unlock(&mutex_WRITE_QUEUE);

		}
		break;
	}
	return 0;

}


uint32_t PRAID_ppd_receive(char*  msgIn,uint32_t fd)
{

	nipcMsg_t NIPCmsgIn = NIPC_toMsg(msgIn);

	pthread_mutex_lock(&mutex_LIST);
	uint32_t IDrequest= NIPC_getID(NIPCmsgIn);

	praid_list_node* nodoBuscado = PRAID_SearchPPDBySocket(fd);
	//Se fija que nodo de la lista tiene el socket del que proviene el pedido
	queueNode_t* nodoSublista = PRAID_Search_Requests_SL(IDrequest,nodoBuscado->colaSublista);//Busca en la cola para el primer pedido que tenga el sector, y que este en estado ENVIADO
	praid_sl_content* contenidoNodoSublista=((praid_sl_content*) nodoSublista->data);
	contenidoNodoSublista->status = RECEIVED;
	contenidoNodoSublista->msg = NIPCmsgIn;
	pthread_mutex_unlock(&mutex_LIST);

	return 0;
}


/*Del Switch de PFS
		case HANDSHAKE:
				if(RAID_ACTIVE == false){
				//Responde hanshake
				 nipcMsg_t msgOut = NIPC_createMsg(HANDSHAKE,0,0);  //si esta bien contestar type 0, payload 0
				 COMM_send(NIPC_toBytes(&msgOut),fd);
			}else{
				//Respoder hanshake:Error, no hay PPD asociado
				char* error = malloc(1);
			//	*error = PRAID_NOTREADY;
				nipcMsg_t msgOut = NIPC_createMsg(HANDSHAKE,1,error);
				COMM_send(NIPC_toBytes(&msgOut),fd);
			}
		break;
*/

/* Del switch PPD
	case HANDSHAKE:{
		nipcMsg_t msgOut = NIPC_createMsg(HANDSHAKE,0,0);  //si esta bien contestar type 0, payload 0
		COMM_send(NIPC_toBytes(&msgOut),fd);

		//OBTENER CANTIDAD DE SECTORES DISK_SECTORS_AMOUNT;

		pthread_t main_ppd_thread;
		pthread_create(&main_ppd_thread,NULL,ppd_handler_thread,(void *)fd);
	}
	break;
*/


/*
void error_fd(uint32_t fd)
{
	uint32_t cur;
	cur = QUEUE_searchNode(pfs_list,fd,dataLength);
	if(cur !=NULL){
		//FINN!! no anda el PFS
	}else{
		cur = QUEUE_searchNode(ppd_list,fd,dataLength);
		if(cur !=NULL){
			//PRAID_clear_list_node(praid_list_node* nodo); primero hay que buscar el nodo que tenga este fd
		}
	}
}



uint32_t Create_Sockets_INET(uint32_t* listenFD){

	struct sockaddr_in dir;

	if((*listenFD = socket(AF_INET,SOCK_STREAM,0)) == -1){
		perror("socket");
		exit(1);
	}
	dir.sin_family = AF_INET;
	dir.sin_addr.s_addr = INADDR_ANY;
	dir.sin_port = htons(PORT);
	memset(&(dir.sin_zero),'\0',8);

	if(bind(*listenFD,(struct sockaddr *)&dir,sizeof(dir))==-1){
		perror("bind");
		exit(1);
	}
	if(listen(*listenFD,10) == -1){
		perror("listen");
		exit(1);
	}

	return 0;
}






*/

















/*
 * DEPRECADO
 *
uint32_t PRAID_manage_PPD(nipcMsg_t msgIn, PPDnode* first, PPDnode* last, diskInfo* sender, pfsInfo receiver){

	switch (msgIn[0])	{

		case HANDSHAKE:
			if(first == last){
				//PRAID_requestDiskInfo(sender, 0x13);
				//PEDIR INFORMACION DEL DISCO
			}else{
				//ESPEJAR
				//ESPEJAR
				//THREAD NUEVO AL QUE LE PASO LA FUNCION, EL NODO QUE VA A HACER DE MASTER, Y EL SOCKET AL DISCO NUEVO
			}
		break;
		case READ_SECTORS://Se leyo el sector ok
			if(first == last){ //Si proviene del primer disco conectado
				//ME SIRVE EL SECTOR 0X13??
				//SI NO, PEDI EL 0X20
				//SI ME SIRVE, CREA UN THREAD PARA EL DISCO

			}else{
				//SE LO MANDO AL THREAD PFS
			}
		break;
		case WRITE_SECTORS:
			//Se escribio el sector, ver si todos ya escribieron.
			//RECORRER LA COLA DE PPDS, Y VER EL STATUS DE CADA UNO.
			//SI ESTAN TODOS, SE LO MANDO AL PFS, SETEO PRAID_STATUS EN ACTIVE
		break;
	}
	return 0;
}


uint32_t PRAID_manage_PFS(nipcMsg_t msgIn, PPDnode first, PPDnode last){
diskInfo readNext;
	switch (msgIn.type)
	{

		case HANDSHAKE:
			//if (PRAID_STATUS!=INACTIVE){


		break;

		case WRITE_SECTORS:
			//Enviar pedido a todos los discos
			//SETEAR VARIABLE PRAID_STATUS EN WAIT_WRITE
			//PRAID_requestPPD_Write(msgIn, first, last);
		break;

		case READ_SECTORS: //ELEGIR EL DISCO, MANDARSELO AL THREAD ASOCIADO
			//readNext = PRAID_select_free_PPD(first, last);
			//PRAID_requestPPD(msgIn, readNext);
		break;
	}
	return 0;
}
uint32_t PRAID_takeRequest(msgNIPC_t msg,nipc_node* first, nipc_node* last)
{
	nipc_node* new = malloc(sizeof(nipc_node));
	new->info = msg;
	new->next = 0x0;

	if(first == 0x0){
		first = new;
		last = new;
	}
	else {
		last->next = new;
		last = new;
	}

	return 0;
}

uint32_t PRAID_manageRequest(msgNIPC_t msg, nipc_node* first) {
	if (first == 0x0)
			return 1;
	if (msg.type == READ)
	{
		//Enviala a cualquier thread libre excepto los que se estan actualizando
	}

	if (msg.type == WRITE)
	{
		//Fijate que no se este actualizando ningun disco nuevo, y mandasela a cualquiera
	}
	//Elimino el nodo de la cola
	nipc_node* aux;
	first = aux;
	msg = first->info;
	first = first->next;
	free(aux);
	return 0;
}
FIN DEPRECADO

*/
