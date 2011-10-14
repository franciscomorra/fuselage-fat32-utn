/*
 * praid_comm.c
 *
 *  Created on: 26/09/2011
 *      Author: utn_so
 */

#include "nipc.h"
#include "praid_comm.h"
#include "praid_queue.h"
#include "praid_main.h"

//Decodificacion del NIPC

uint32_t pfs_receive(nipcMsg_t msgIn)
{

	switch (msgIn.type)
	{

		case HANDSHAKE:
			//if(raid_status!=0){
				//TODO praid_handlehandshake(msgIn);
			//}
		break;

		case WRITE_SECTORS:
			praid_WRITE_add(msgIn);
		break;

		case READ_SECTORS:
			praid_READ_add(msgIn);
		break;
	}
	return 0;
}
















/*
 * DEPRECADO
 *
uint32_t PRAID_manage_PPD(nipcMsg_t msgIn, PPDnode* first, PPDnode* last, diskInfo* sender, pfsInfo receiver){

	switch (msgIn.type)	{

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
