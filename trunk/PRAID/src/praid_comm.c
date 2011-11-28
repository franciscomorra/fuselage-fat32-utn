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
extern uint32_t ACTIVE_DISKS_AMOUNT;
extern bool SYNCHRONIZING_DISCS;

extern uint32_t DISKS_AMOUNT;
//extern queue_t* WRITE_QUEUE;

extern pthread_mutex_t mutex_LIST;
//extern pthread_mutex_t mutex_WRITE_QUEUE;

#include "comm.h"
//extern struct praid_list_node* PRAID_LIST;
//extern pthread_mutex_t mutex_LIST;



praid_ppdThreadParam* PRAID_ValidatePPD(char* msgIn, uint32_t newPPD_FD)
{
	uint16_t len;
	memcpy(&len,msgIn+1,sizeof(uint16_t));
	if(len == 8){
		uint32_t disk_sectors_rcv;
		memcpy(&disk_sectors_rcv,(msgIn+sizeof(uint32_t)+sizeof(uint16_t)+1),sizeof(uint32_t));

		if(RAID_ACTIVE==true){
			if(disk_sectors_rcv < DISK_SECTORS_AMOUNT){
				print_Console("ERROR, DISCO CON MENOR CANTIDAD DE SECTORES",disk_sectors_rcv,1,true);
				return NULL;
			}

		}else{
			if(disk_sectors_rcv > 0){
				DISK_SECTORS_AMOUNT = disk_sectors_rcv; //No hace falta un mutex, el select va a hacer de a un pedido
			}else{
				print_Console("ERROR, CANTIDAD DE SECTORES NO VALIDA",pthread_self(),1,false);
				return NULL;
			}
		}
		uint32_t diskID;
		memcpy(&diskID,msgIn+1+sizeof(uint16_t),sizeof(uint32_t));
		pthread_mutex_lock(&mutex_LIST);
		if(PRAID_DISK_ID_EXISTS(diskID) == true){
			print_Console("ID DE DISCO YA EXISTENTE",diskID,1,true);
			return NULL;
		}
		pthread_mutex_unlock(&mutex_LIST);

		praid_ppdThreadParam* parameters = malloc(sizeof(praid_ppdThreadParam));
		parameters->diskID = diskID;
		parameters->socketPPD = newPPD_FD;
		return parameters;

	}
print_Console("ERROR, TAMAÑO DE HANDSHAKE",pthread_self(),1,false);
return NULL;
}



uint32_t PRAID_PFS_RECEIVE_REQUEST(char* msgIn,uint32_t fd)
{
	praid_sl_content* data_sublist = malloc(sizeof(praid_sl_content));
	data_sublist->synch = false;
	data_sublist->msg = NIPC_toMsg(msgIn);
	data_sublist->socketRequest = fd;

	switch (msgIn[0])
	{
		//Pedido de READ:
		case READ_SECTORS:{
			//print_Console("comm Pedido nuevo de READ",((uint32_t)msgIn+7));
			pthread_mutex_lock(&mutex_LIST);
			PRAID_ADD_READ(data_sublist);
			pthread_mutex_unlock(&mutex_LIST);

		}
		break;
		//Pedido de WRITE:
		case WRITE_SECTORS:{
			//print_Console("comm Pedido nuevo de WRITE",((uint32_t)msgIn+7));
			pthread_mutex_lock(&mutex_LIST);
			PRAID_ADD_WRITE(data_sublist);
			pthread_mutex_unlock(&mutex_LIST);
		}
		break;
	}
	return 0;

}


uint32_t PRAID_PPD_RECEIVE_REQUEST(char*  msgIn,uint32_t fd)
{
	nipcMsg_t NIPCmsgIn = NIPC_toMsg(msgIn);
	uint32_t IDrequest = 0;
	memcpy(&IDrequest,msgIn+3,sizeof(uint32_t));
	uint32_t sector =0;
	memcpy(&sector,msgIn+7,sizeof(uint32_t));

	free(msgIn);
	/*
	pthread_mutex_lock(&mutex_LIST);
	uint32_t IDrequest= NIPC_getID(NIPCmsgIn);
	*/
	//print_Console("comm Respuesta de PPD Sector:",sector);
	//print_Console("comm Respuesta de PPD ID:",IDrequest);

	praid_list_node* nodoBuscado = PRAID_GET_PPD_FROM_FD(fd);//Se fija que nodo de la lista tiene el socket del que proviene el pedido
	if(nodoBuscado!=NULL){
		queueNode_t* nodoSublista = PRAID_GET_REQUEST_BY_ID(IDrequest,nodoBuscado->colaSublista);//Busca en la cola para el primer pedido que tenga el sector, y que este en estado ENVIADO
		if(nodoSublista!=NULL){
			praid_sl_content* contenidoNodoSublista=((praid_sl_content*) nodoSublista->data);
			if(contenidoNodoSublista->msg.type==READ_SECTORS){
				//print_Console("Actualizando pedido de READ",nodoBuscado->tid,2,true);
			}else{
				//print_Console("Actualizando pedido de WRITE",nodoBuscado->tid,2,true);
			}

			contenidoNodoSublista->status = RECEIVED;
			contenidoNodoSublista->msg = NIPCmsgIn;
			sem_post(&nodoBuscado->request_list_sem);
		}else{
			print_Console("PEDIDO NO ENCONTRADO",IDrequest,1,true);//ERROR DEL HANSHAKE
		}
	}else{
		print_Console("DISCO NO ENCONTRADO",IDrequest,1,true);

	}
	return 0;
}

uint32_t PRAID_HANDLE_DOWN_PPD(uint32_t currFD)
{

	praid_list_node* nodoBuscado = PRAID_GET_PPD_FROM_FD(currFD);
	print_Console("DESACTIVANDO DISCO",nodoBuscado->diskID,1,true);

	if(nodoBuscado!=NULL){
		nodoBuscado->ppdStatus = DISCONNECTED;
		sem_post(&nodoBuscado->request_list_sem);
		print_Console("DISCO DESACTIVADO",nodoBuscado->diskID,1,true);
	}else{
		print_Console("DISCO CON ESE SOCKET NO ENCONTRADO",nodoBuscado->diskID,1,true);
		return 1;
	}

//TODO ver bien la desconexion de discos
	if(ACTIVE_DISKS_AMOUNT==1 && nodoBuscado->ppdStatus== READY){
		print_Console("UNICO DISCO DESCONECTADO -- APAGANDO PROCESO",pthread_self(),1,false);
		PRAID_WRITE_LOG("Apagado Proceso RAID, Unico PPD desconectado");
		exit(0);
	}
	if(nodoBuscado->ppdStatus == READY && ACTIVE_DISKS_AMOUNT==2 && SYNCHRONIZING_DISCS==true){
		print_Console("MASTER DESCONECTADO MIENTRAS SINCRONIZABA -- APAGANDO PROCESO",pthread_self(),1,false);
		PRAID_WRITE_LOG("Apagado Proceso RAID, Master desconectado mientras sincronizaba");
		//ERROR GRAVE, SI SE QUIERE, QUE AGARRE EL PRIMER DISCO Y VACIE LA COLA DE PEDIDOS DEL PFS
		exit(0);
	}
	if(nodoBuscado->ppdStatus == SYNCHRONIZING){
		SYNCHRONIZING_DISCS = false;
		if(PRAID_ACTIVATE_NEXT_SYNCH()==false){
			print_Console("NO HAY MAS DISCOS PARA SINCRONIZAR",pthread_self(),1,false);
		}else{
			print_Console("ACTIVANDO DISCO A SINCRONIZAR",nodoBuscado->tid,1,true);
		}
	}



}
