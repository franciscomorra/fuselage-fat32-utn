/*
 * praid_ppd_main.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "praid_ppd_handler.h"
#include "praid_console.h"
#include "praid_comm.h"
#include "praid_queue.h"
#include "tad_queue.h"


extern uint32_t RAID_STATUS; //0 INACTIVE - 1 ACTIVE - 2 WAIT_FIRST_PPD_REPLY
extern uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
extern struct praid_list_node* PRAID_LIST;
extern pthread_mutex_t mutex_LIST;
extern pthread_mutex_t mutex_RAID_STATUS;

void *ppd_handler_thread (void *data) //TODO recibir el socket de ppd
{
	uint32_t* sectorCount = 0;//Primer Sector del disco
	uint32_t self_tid = (uint32_t)pthread_self(); //TID del PPD

	pthread_mutex_lock(&mutex_LIST);
	praid_list_node* self_list_node = PRAID_list_appendNode(self_tid);
	queue_t* self_SL = self_list_node->info->colaSublista;
	pthread_mutex_unlock(&mutex_LIST);

	while (1){
		uint32_t i=0;
		i++;
		if(RAID_STATUS == 0 || RAID_STATUS == 2){
			//Si es el primer PPD que se conecta, pide informacion del disco (DISK_SECTORS_AMOUNT)
			//Por si acaso se cae el primer disco antes de recibir el pedido de DISK_SECTORS_AMOUNT, pregunto estado 2
			pthread_mutex_lock(&mutex_RAID_STATUS);
			RAID_STATUS = 2; //RAID ESPERANDO DISK_SECTORS_AMOUNT
			pthread_mutex_unlock(&mutex_RAID_STATUS);
		/*
			//TODO Enviar Pedido de informacion de Disco (DISK_SECTORS_AMOUNT)
			//Esperar respuesta
			//Decodificar respuesta
		*/
			if(RAID_STATUS != 1){//Si no lo activaron a ultimo momento
				//Setear DISK_SECTORS_AMOUNT
				pthread_mutex_lock(&mutex_RAID_STATUS);
				RAID_STATUS = 1; //RAID ACTIVADO
				pthread_mutex_unlock(&mutex_RAID_STATUS);

				print_Console("RAID Activado",(uint32_t)pthread_self());//CONSOLE STATUS ACTIVE
			}
		}else{ //RAID tiene al menos un disco, estado ACTIVE
			print_Console("Nuevo PPD",(uint32_t)pthread_self());//CONSOLE NEW PPD
		}
		print_Console("Iniciando Sincronizacion",(uint32_t)pthread_self());//CONSOLE INICIO_SYNCHRONIZE

		uint32_t self_list_accesses=0;
		while(1){
			self_list_accesses++;
			pthread_mutex_lock(&mutex_LIST);
			while(QUEUE_length(self_SL) > 0){
				queueNode_t* current_SL_node = QUEUE_takeNode(self_SL);

				praid_sl_content current_SL_content;
				memcpy(&current_SL_content,current_SL_node->data,sizeof(praid_sl_content));

//TODO	Envialo a PPD

				if(current_SL_content.msg.type == READ_SECTORS){
					if(current_SL_content.synch == 0){

						print_Console("Request Sent To Disk for read",(uint32_t)pthread_self());//CONSOLE READ
/*
						TODO Espera a que vuelva
						Cuando vuelve, mandaselo al PFS directamente
*/
					}else{
/*
						TODO Espera a que vuelva
						Cuando vuelve
*/
							queueNode_t *subListNode = malloc(sizeof(queueNode_t));
							praid_sl_content *data_sublist= malloc(sizeof(praid_sl_content));
							data_sublist->synch = 1;
							//SOCKET PPD
							data_sublist->msg.type = WRITE_SECTORS;
							subListNode->data = (praid_sl_content*)data_sublist;
							subListNode->next = NULL;
							//NIPC tyoe READ_SECTORS
							//TODO agregar subListNode a la cola del que le toca proximo
					}
				}else if (current_SL_content.msg.type == WRITE_SECTORS){
					if(current_SL_content.synch == 0){
						print_Console("Request Sent To Disk for Write",(uint32_t)pthread_self());//CONSOLE WRITE
/*
						TODO Cuando vuelve, mandaselo al PFS directamente
*/
					}else{
/*
						TODO Espera a que vuelva
						Cuando vuelve:
*/
						if(sectorCount<DISK_SECTORS_AMOUNT){//Si no es el ultimo
							queueNode_t *subListNode = malloc(sizeof(queueNode_t));
							praid_sl_content *data_sublist= malloc(sizeof(praid_sl_content));
							data_sublist->synch = 1;
							//TODO SOCKET PPD
							data_sublist->msg.type = READ_SECTORS;
							//TODO NIPC tyoe READ_SECTORS

							subListNode->data = (praid_sl_content*)data_sublist;
							subListNode->next = NULL;
							self_SL->end->next = subListNode;
							self_SL->end = subListNode;
							sectorCount++;
						}else{//Si era el ultimo
							pthread_mutex_lock(&mutex_RAID_STATUS);
							self_list_node->info->ppdStatus = 0; //Cambia estado de PPD a ACTIVO
							pthread_mutex_unlock(&mutex_RAID_STATUS);

						}

					}

				}

			}
			//free(current_SL_content);
			pthread_mutex_unlock(&mutex_LIST);
		}

	}
return NULL;
}

