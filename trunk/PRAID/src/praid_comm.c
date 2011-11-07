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
#include <errno.h>
#define PORT 9333
extern uint32_t RAID_STATUS; //0 INACTIVE - 1 ACTIVE
//extern struct praid_list_node* PRAID_LIST;
//extern pthread_mutex_t mutex_LIST;



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



/*
Manejar pedido de nuevo PPD
Manejar pedido de PFS
Enviar Mensaje a Socket (a PPD o a PFS)
*/

//Decodificacion del NIPC

/*uint32_t pfs_receive(char* msgIn,uint32_t fd)
{


switch (msgIn[0])
	//Handshake:
	case HANDSHAKE:
		if(RAID_STATUS!=1){
			//Responde hanshake
			 uint32_t len, bytes_sent;
			 char *msgOut=NIPC_createCharMsg(0,0,0);  //si esta bien contestar type 0, payload 0
			 len = strlen(msgOut);
			 bytes_sent = send(fd, msgOut, len, 0);
		}else{
			//Respoder hanshake:Error, no hay PPD asociado
			 char *msgOut = "Error, no hay PPD asociado";
             uint32_t len, bytes_sent;
             len = strlen(msgOut);
             bytes_sent = send(fd, msgOut, len, 0);


		}
	break;
	//Pedido de READ:
	case WRITE_SECTORS:

		praid_sl_content* data_sublist;
		data_sublist.synch = 0;
		data_sublist.msg = msgIn;
		//data_sublist.socket = fd; falta agregarlo en el struct

		PRAID_add_READ_Request(data_sublist);

	break;
	//Pedido de WRITE:
	case WRITE_SECTORS:


		praid_sl_content* data_sublist;
		data_sublist.synch = 1;
		data_sublist.msg = msgIn;
		//data_sublist.socket = fd;   falta agregarlo en el struct

		PRAID_add_WRITE_Request(data_sublist);
	break;
	return 0;
}
*/

uint32_t ppd_receive(char* msgIn,uint32_t fd)
{
/*
	Handshake:
			//Responde hanshake

			 char *msgOut=NIPC_createCharMsg(0,0,0)  //si esta bien contestar type 0, payload 0
			 uint32_t len, bytes_sent;
             len = strlen(msgOut);
             bytes_sent = send(sockfd, msgOut, len, 0);


	Pedido de READ:
		TODO
	Pedido de WRITE:
		TODO
*/
	return 0;
}


/*void fd_appendNode(queue_t *list, void *fd)
{
	QUEUE_initialize(list);
	QUEUE_appendNode(list,fd);
	return 0;
}
*/
/*void error_fd(uint32_t fd)
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
