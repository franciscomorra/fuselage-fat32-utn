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

extern queue_t* WRITE_QUEUE;
extern uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
extern pthread_mutex_t mutex_LIST;
extern pthread_mutex_t mutex_WRITE_QUEUE;
extern t_log *raid_log_file;


void *ppd_handler_thread (void *thread_data)
{

	uint32_t sectorCount = 1;//Primer Sector del disco mas uno por el de synch
	uint32_t self_tid = pthread_self(); //TID del PPD
	uint32_t socketPPD = (uint32_t)thread_data;

	pthread_mutex_lock(&mutex_LIST);
	praid_list_node* self_list_node = PRAID_list_appendNode(self_tid, socketPPD);
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
			}else{
				//print_Console("Continua Espera Sincronizacion",self_tid);
			}

		}else{
			//INICIO MIRAR LA COLA
			if(QUEUE_length(self_SL) > 0){
				queueNode_t* current_SL_node = QUEUE_takeNode(self_SL);
				self_sl_node_read++;
				praid_sl_content* current_sl_content =((praid_sl_content*) current_SL_node->data);
				print_Console("Cantidad de pedidos tomados:",self_tid);
				print_Console("Pedidos tomados por el thread:",self_sl_node_read);

				switch(current_sl_content->status){

					case 0:	//PEDIDO NUEVO
				//		char* serializado = NIPC_toBytes(current_sl_content->msg);
						//COMM_send(serializado,current_sl_content->socketPFS);
						current_sl_content->status = 1;
						QUEUE_appendNode(self_SL, current_sl_content);

						if(current_sl_content->synch == 1 && current_sl_content->msg.type == WRITE_SECTORS){
							if(sectorCount < DISK_SECTORS_AMOUNT){//Si no es el ultimo
								sectorCount++;
								print_Console("WRITE a DISCO (Sincronizacion) de:",self_tid);
								praid_sl_content *new_data_sublist= malloc(sizeof(praid_sl_content));
								new_data_sublist->synch = 1;
								new_data_sublist->status = 0;//Unread
								new_data_sublist->socketPFS = 0;//El socket va a estar vacio, es de sincronizacion
								new_data_sublist->msg = NIPC_createMsg(READ_SECTORS,sizeof(uint32_t),(char*) &sectorCount);
								//TODO Verificar NIPC
								PRAID_ADD_READ(new_data_sublist);
							}else{//Si era el ultimo
								self_list_node->info->ppdStatus = 0; //Cambia estado de PPD a ACTIVO
								print_Console("Fin Sincronizacion: ",self_tid);
								log_debug(raid_log_file,"PRAID","Fin Sincronizacion (%s)",self_tid);
							}
						}
					break;
					case 1://EL THREAD YA LO LEYO AL PEDIDO Y SE LO ENVIO AL DISCO ASIGNADO
						QUEUE_appendNode(self_SL, current_sl_content);
					break;
					case 2://EL MAIN RECIBIO UNA RESPUESTA DEL DISCO Y ACTUALIZO EL PEDIDO
						if(current_sl_content->synch == 0){//NO ES PARA SINCRONIZACION

							if(current_sl_content->msg.type == READ_SECTORS){
								//TODO ENVIAR A PFS DIRECTAMENTE
								print_Console("READ a DISCO de: ",self_tid);
								log_debug(raid_log_file,"PRAID","Request Sent To Disk for read (%s)",self_tid);
							}else{//WRITE_SECTORS

								//TODO Sacar ID Pedido del NIPC

								pthread_mutex_lock(&mutex_WRITE_QUEUE);
								uint32_t requestID;
								queueNode_t* nodoWRITE = PRAID_Search_WRITE(requestID);

								praid_read_content* contenidoNodoWrite=((praid_read_content*) nodoWRITE->data);
								contenidoNodoWrite->threads_left--;
								if(contenidoNodoWrite->threads_left == 0){
									pthread_mutex_unlock(&mutex_WRITE_QUEUE);
									//TODO ENVIAR A PFS
									print_Console("WRITE a DISCO de:",self_tid);
									log_debug(raid_log_file,"PRAID","Request Sent To Disk for Write (%s)",self_tid);
									pthread_mutex_lock(&mutex_WRITE_QUEUE);
									QUEUE_appendNode(WRITE_QUEUE,contenidoNodoWrite);
								}
								pthread_mutex_unlock(&mutex_WRITE_QUEUE);



							}
						}else{//ES PARA SINCRONIZACION
							if(current_sl_content->msg.type == READ_SECTORS){
								print_Console("READ a DISCO (Sincronizacion) de:",self_tid);
								praid_sl_content *new_data_sublist= malloc(sizeof(praid_sl_content));
								new_data_sublist->synch = 1;
								new_data_sublist->msg = current_sl_content->msg;
								new_data_sublist->status = 0;
								PRAID_ADD_WRITE(new_data_sublist);
							}
						}
						free(current_SL_node->data);
						free(current_SL_node);
						print_Console("Nodo Liberado.",self_tid);
					break;

				}
			}

		}
		pthread_mutex_unlock(&mutex_LIST);
	}
return NULL;
}


/*

pthread_t main_ppd_thread;
	uint32_t socketPPD = 555;
	pthread_create(&main_ppd_thread,NULL,ppd_handler_thread,(void *)socketPPD);
	pthread_create(&main_ppd_thread,NULL,ppd_handler_thread,(void *)socketPPD);
	pthread_create(&main_ppd_thread,NULL,ppd_handler_thread,(void *)socketPPD);
	pthread_create(&main_ppd_thread,NULL,ppd_handler_thread,(void *)socketPPD);
	int i;
	while(1){i++;}



if(RAID_STATUS == 0 || RAID_STATUS == 2){ //SI ME LO PASAN POR HANDSHAKE ESTE BARDO NO VA
	//Si es el primer PPD que se conecta, pide informacion del disco (DISK_SECTORS_AMOUNT)
	//Por si acaso se cae el primer disco antes de recibir el pedido de DISK_SECTORS_AMOUNT, pregunto estado 2
	pthread_mutex_lock(&mutex_RAID_STATUS);
	RAID_STATUS = 2; //RAID ESPERANDO DISK_SECTORS_AMOUNT
	pthread_mutex_unlock(&mutex_RAID_STATUS);


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
