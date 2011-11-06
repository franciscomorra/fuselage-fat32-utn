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
#include "log.h"
#include "praid_comm.h"
#include "praid_queue.h"
#include "tad_queue.h"

extern uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
extern pthread_mutex_t mutex_LIST;
extern t_log *raid_log_file;


void *ppd_handler_thread (void *data) //TODO recibir el socket de ppd
{

	//TODO Recibir cantidad de sectores en el HANDSHAKE

	uint32_t sectorCount = 1;//Primer Sector del disco mas uno por el de synch
	uint32_t self_tid = (uint32_t)pthread_self(); //TID del PPD

	pthread_mutex_lock(&mutex_LIST);
	praid_list_node* self_list_node = PRAID_list_appendNode(self_tid);
	queue_t* self_SL = self_list_node->info->colaSublista;
	pthread_mutex_unlock(&mutex_LIST);

	uint32_t self_sl_accesses=0;
	uint32_t self_sl_node_read=0;

	while(1){
		self_sl_accesses++;
		pthread_mutex_lock(&mutex_LIST);
		if (self_list_node->info->ppdStatus == 2){ //Si se conecto pero hay un disco sincronizandose, espera a sincronizarse
			if(PRAID_hay_discos_sincronizandose() != 0){
				self_list_node->info->ppdStatus = 1;
				print_Console("Fin Espera Sincronizacion",self_tid);
				log_debug(raid_log_file,"PRAID","Fin Espera Sincronizacion(%s)",self_tid);
				PRAID_Start_Synch();
			}
		}else{
			//INICIO MIRAR LA COLA
			if(QUEUE_length(self_SL) > 0){
				queueNode_t* current_SL_node = QUEUE_takeNode(self_SL);
				self_sl_node_read++;
				print_Console("Cantidad de pedidos tomados:",self_tid);
				print_Console("Pedidos tomados por el thread:",self_sl_node_read);

				//TODO	Envialo a PPD

				if(((praid_sl_content*) current_SL_node->data)->msg.type == READ_SECTORS){
					if(((praid_sl_content*) current_SL_node->data)->synch == 0){
						print_Console("READ a DISCO de: ",self_tid);
						log_debug(raid_log_file,"PRAID","Request Sent To Disk for read (%s)",self_tid);
					}else{//El pedido es de SYNCH
						print_Console("READ a DISCO (Sincronizacion) de:",self_tid);
						praid_sl_content *data_sublist= malloc(sizeof(praid_sl_content));
						data_sublist->synch = 1;
						uint32_t contenido_Leido = 0303456;
						//TODO NIPC
						data_sublist->msg = NIPC_createMsg(WRITE_SECTORS,sizeof(uint32_t),(char*) &contenido_Leido);
						PRAID_add_WRITE_Request(data_sublist);
					}
				}else{//WRITE_SECTORS
					if(((praid_sl_content*) current_SL_node->data)->synch == 0){
						print_Console("WRITE a DISCO de:",self_tid);
						log_debug(raid_log_file,"PRAID","Request Sent To Disk for Write (%s)",self_tid);
					}else{//El pedido es de SYNCH
						if(sectorCount<DISK_SECTORS_AMOUNT){//Si no es el ultimo
							sectorCount++;
							print_Console("WRITE a DISCO (Sincronizacion) de:",self_tid);
							praid_sl_content *data_sublist= malloc(sizeof(praid_sl_content));
							data_sublist->synch = 1;
							//TODO SOCKET PPD
							data_sublist->msg = NIPC_createMsg(READ_SECTORS,sizeof(uint32_t),(char*) &sectorCount);
							PRAID_add_READ_Request(data_sublist);
						}else{//Si era el ultimo
							self_list_node->info->ppdStatus = 0; //Cambia estado de PPD a ACTIVO
							print_Console("Fin Sincronizacion: ",self_tid);
							log_debug(raid_log_file,"PRAID","Fin Sincronizacion (%s)",self_tid);
						}
					}
				}
				free(current_SL_node->data);
				free(current_SL_node);
				print_Console("Nodo Liberado.",self_tid);//CONSOLE READ
			}
			//FIN MIRAR LA COLA

		}
		pthread_mutex_unlock(&mutex_LIST);
	}
return NULL;
}


/*
if(RAID_STATUS == 0 || RAID_STATUS == 2){ //SI ME LO PASAN POR HANDSHAKE ESTE BARDO NO VA
	//Si es el primer PPD que se conecta, pide informacion del disco (DISK_SECTORS_AMOUNT)
	//Por si acaso se cae el primer disco antes de recibir el pedido de DISK_SECTORS_AMOUNT, pregunto estado 2
	pthread_mutex_lock(&mutex_RAID_STATUS);
	RAID_STATUS = 2; //RAID ESPERANDO DISK_SECTORS_AMOUNT
	pthread_mutex_unlock(&mutex_RAID_STATUS);

	//TODO Enviar Pedido de informacion de Disco (DISK_SECTORS_AMOUNT)
	//Esperar respuesta
	//Decodificar respuesta
	if(RAID_STATUS != 1){//Si no lo activaron a ultimo momento
		//Setear DISK_SECTORS_AMOUNT
		pthread_mutex_lock(&mutex_RAID_STATUS);
		RAID_STATUS = 1; //RAID ACTIVADO
		pthread_mutex_unlock(&mutex_RAID_STATUS);
		print_Console("RAID Activado",self_tid);//CONSOLE STATUS ACTIVE
		log_debug(raid_log_file,"PRAID","RAID Activado(%s)",pthread_self());
	}

} ESTO IRIA ENCAPSULADO EN UN WHILE ANTES DE TOMAR PEDIDOS
*/
