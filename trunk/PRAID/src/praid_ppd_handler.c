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
#include <stdbool.h>
#include "praid_ppd_handler.h"
#include "praid_console.h"
#include "log.h"
#include "praid_comm.h"
#include "praid_queue.h"
#include "tad_queue.h"
#include "nipc.h"
#include "tad_sockets.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern queue_t* WRITE_QUEUE;
extern uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
extern pthread_mutex_t mutex_LIST;
extern pthread_mutex_t mutex_WRITE_QUEUE;
extern t_log *raid_log_file;


void *ppd_handler_thread (void *thread_data)
{

	uint32_t sectorCount = 1;//Primer Sector del disco mas uno por el de synch
	uint32_t self_tid = pthread_self(); //TID del PPD
	praid_ppdThreadParam* handler_param = (praid_ppdThreadParam*)thread_data;

	pthread_mutex_lock(&mutex_LIST);
	praid_list_node* self_list_node = PRAID_list_appendNode(self_tid, handler_param);
	queue_t* self_SL = self_list_node->colaSublista;
	pthread_mutex_unlock(&mutex_LIST);

	uint32_t self_sl_accesses=0;
	uint32_t self_sl_node_read=0;

	while(1){
		self_sl_accesses++;
		if(self_list_node->ppdStatus == DISCONNECTED){//Se desconecto y el main me actualizo el estado
			pthread_mutex_lock(&mutex_LIST);
			return PRAID_clear_list_node(self_list_node);
			pthread_mutex_unlock(&mutex_LIST);

		}else{

			if (self_list_node->ppdStatus == WAIT_SYNCH){ //Si se conecto pero hay un disco sincronizandose, espera a sincronizarse
				pthread_mutex_lock(&mutex_LIST);
				if(PRAID_hay_discos_sincronizandose() == false){//Se empieza a sincronizar
					self_list_node->ppdStatus = SYNCHRONIZING;
					print_Console("Fin Espera Sincronizacion",self_tid);
					PRAID_Start_Synch();
				}else{
					//print_Console("Continua Espera Sincronizacion",self_tid);
				}
				pthread_mutex_unlock(&mutex_LIST);

			}else{//Ya esta sincronizado
				pthread_mutex_lock(&mutex_LIST);

				//INICIO MIRAR LA COLA
				if(QUEUE_length(self_SL) > 0){
					queueNode_t* current_SL_node = QUEUE_takeNode(self_SL);
					self_sl_node_read++;
					praid_sl_content* current_sl_content =((praid_sl_content*) current_SL_node->data);
					print_Console("Mirando en la cola de pedidos:",self_tid);
					print_Console("Pedidos tomados por el thread:",self_sl_node_read);

					switch(current_sl_content->status){

						case UNREAD://PEDIDO NUEVO
							pthread_mutex_unlock(&mutex_LIST);//Deja libre la cola mientras envias

							char *msgToPPD = NIPC_toBytes(&current_sl_content->msg);
							uint16_t msgToPPD_len = *((uint16_t*) current_sl_content->msg.len);
							send(self_list_node->socketPPD,msgToPPD,msgToPPD_len+3,0);
							pthread_mutex_lock(&mutex_LIST);

							current_sl_content->status = SENT;
							QUEUE_appendNode(self_SL, current_sl_content);

							if(current_sl_content->synch == true && current_sl_content->msg.type == WRITE_SECTORS){
								if(sectorCount < DISK_SECTORS_AMOUNT){//Si no es el ultimo
									sectorCount++;
									print_Console("WRITE a DISCO (Sincronizacion) de:",self_tid);
									praid_sl_content *new_data_sublist= malloc(sizeof(praid_sl_content));
									new_data_sublist->synch = true;
									new_data_sublist->status = UNREAD;//Unread
									new_data_sublist->socketPFS = 0;//El socket va a estar vacio, es de sincronizacion
									new_data_sublist->msg = NIPC_createMsg(READ_SECTORS,sizeof(uint32_t),(char*) &sectorCount);
									//TODO Verificar NIPC (el ID del pedido a donde se pone?)
									PRAID_ADD_READ(new_data_sublist);
								}else{//Si era el ultimo
									self_list_node->ppdStatus = READY; //Cambia estado de PPD a ACTIVO
									print_Console("Fin Sincronizacion: ",self_tid);
									log_debug(raid_log_file,"PRAID","Fin Sincronizacion");
								}
							}else{
								print_Console("READ a DISCO de:",self_tid);
							}
						break;
						case SENT://EL THREAD YA LO LEYO AL PEDIDO Y SE LO ENVIO AL DISCO ASIGNADO, PERO ESPERA RESPUESTA
							QUEUE_appendNode(self_SL, current_sl_content);
						break;
						case RECEIVED://EL MAIN RECIBIO UNA RESPUESTA DEL DISCO Y ACTUALIZO EL PEDIDO

							if(current_sl_content->synch == false){//NO ES PARA SINCRONIZACION
								char *msgToPFS = NIPC_toBytes(&current_sl_content->msg);
								uint16_t msgToPFS_len = *((uint16_t*) current_sl_content->msg.len);

								if(current_sl_content->msg.type == READ_SECTORS){
									pthread_mutex_unlock(&mutex_LIST);

									send(current_sl_content->socketPFS,msgToPFS,msgToPFS_len+3,0);

									print_Console("READ de DISCO de: ",self_tid);
									log_debug(raid_log_file,"PRAID","Pedido de lectura enviado a proceso PPD");
									pthread_mutex_lock(&mutex_LIST);

								}else{//WRITE_SECTORS


									pthread_mutex_lock(&mutex_WRITE_QUEUE);
									uint32_t requestID = NIPC_getID(current_sl_content->msg);
									queueNode_t* nodoWRITE = PRAID_Search_WRITE_Queue(requestID);
									praid_read_content* contenidoNodoWrite=((praid_read_content*) nodoWRITE->data);
									contenidoNodoWrite->threads_left--;
									pthread_mutex_unlock(&mutex_WRITE_QUEUE);

									if(contenidoNodoWrite->threads_left == 0){
										pthread_mutex_unlock(&mutex_WRITE_QUEUE);
										send (current_sl_content->socketPFS, msgToPFS, msgToPFS_len+3, 0);
										pthread_mutex_lock(&mutex_WRITE_QUEUE);

										pthread_mutex_lock(&mutex_WRITE_QUEUE);//Saco nodo de la cola
										queueNode_t* aux = WRITE_QUEUE->begin;
										while(aux->next!=nodoWRITE){
											aux = aux->next;
										}
										aux->next = nodoWRITE->next;
										free(contenidoNodoWrite);
										free(nodoWRITE);
										pthread_mutex_unlock(&mutex_WRITE_QUEUE);

										print_Console("WRITE de DISCO de:",self_tid);
										log_debug(raid_log_file,"PRAID","Pedido de escritura enviado a todos los discos",self_tid);
									}

								}
							}else{//ES PARA SINCRONIZACION
								if(current_sl_content->msg.type == READ_SECTORS){
									print_Console("READ de DISCO (Sincronizacion) de:",self_tid);
									praid_sl_content *new_data_sublist= malloc(sizeof(praid_sl_content));
									new_data_sublist->synch = true;
									new_data_sublist->msg = current_sl_content->msg;
									new_data_sublist->status = UNREAD;
									PRAID_ADD_WRITE(new_data_sublist);
								}
							}
							NIPC_cleanMsg(&current_sl_content->msg);//
							free(current_sl_content);
							free(current_SL_node);
							print_Console("Nodo Liberado.",self_tid);
						break;

					}
				}
				pthread_mutex_unlock(&mutex_LIST);

			}
		}
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

*/
