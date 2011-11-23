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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "praid_ppd_handler.h"
#include "praid_console.h"
#include "log.h"
#include "praid_comm.h"
#include "praid_queue.h"
#include "tad_queue.h"
#include "nipc.h"
#include "tad_sockets.h"

extern queue_t* WRITE_QUEUE;
extern uint32_t DISK_SECTORS_AMOUNT; //CANTIDAD DE SECTORES DEL DISCO, PARA SYNCHRONIZE
extern pthread_mutex_t mutex_LIST;
extern pthread_mutex_t mutex_WRITE_QUEUE;
extern t_log *raid_log_file;
extern bool SYNCHRONIZING_DISCS; //0 INACTIVE - 1 ACTIVE


void *ppd_handler_thread (void *thread_data)
{

uint32_t self_tid = pthread_self(); //TID del PPD
praid_ppdThreadParam* handler_param = (praid_ppdThreadParam*)thread_data;

pthread_mutex_lock(&mutex_LIST);
praid_list_node* self_list_node = PRAID_ADD_PPD_NODE(self_tid, handler_param);
queue_t* self_SL = self_list_node->colaSublista;
pthread_mutex_unlock(&mutex_LIST);


while(1){
	sem_wait(&self_list_node->request_list_sem);
	switch(self_list_node->ppdStatus){
		case DISCONNECTED://Se desconecto y el main me actualizo el estado
			pthread_mutex_lock(&mutex_LIST);
			uint32_t auxRemove = PRAID_REMOVE_PPD(self_list_node);
			pthread_mutex_unlock(&mutex_LIST);
			return auxRemove;
		break;
		case WAIT_SYNCH:  //Si se conecto pero hay un disco sincronizandose, espera a sincronizarse
			pthread_mutex_lock(&mutex_LIST);
			if(SYNCHRONIZING_DISCS == false){//Se empieza a sincronizar
				print_Console("handler Fin Espera Sincronizacion",self_tid);
				SYNCHRONIZING_DISCS = true;
				self_list_node->ppdStatus = SYNCHRONIZING;
				PRAID_START_SYNCHR();
				self_list_node->ammount_synch++;
			}else{
				print_Console("handler Continua Espera Sincronizacion",self_tid);
			}
			pthread_mutex_unlock(&mutex_LIST);
		break;
		default:
			//Ya esta sincronizado
			//INICIO MIRAR LA COLA
			if(QUEUE_length(self_SL) > 0){
				queueNode_t* current_SL_node = QUEUE_takeNode(self_SL);
				praid_sl_content* current_sl_content =((praid_sl_content*) current_SL_node->data);
				print_Console("handler Mirando en la cola de pedidos:",self_tid);

				switch(current_sl_content->status){
					case UNREAD://PEDIDO NUEVO
						print_Console("handler Pedido Nuevo",self_tid);
						//Inicio envio a PPD
						char *msgToPPD = NIPC_toBytes(&current_sl_content->msg);
						uint16_t msgToPPD_len = *((uint16_t*) current_sl_content->msg.len);
						send(self_list_node->socketPPD,msgToPPD,msgToPPD_len+3,0);
						print_Console("handler Enviado a disco",self_tid);

						pthread_mutex_lock(&mutex_LIST);
						current_sl_content->status = SENT;
						QUEUE_appendNode(self_SL, current_sl_content);
						if(current_sl_content->synch == true && current_sl_content->msg.type == WRITE_SECTORS){
							if(self_list_node->ammount_synch < DISK_SECTORS_AMOUNT){//Si no es el ultimo
								self_list_node->ammount_synch++;
								uint32_t idpedido = self_list_node->ammount_synch;

								uint32_t size = sizeof(uint32_t);
								int size2 = 2*size;
								char* msgOut = malloc(size2);

								memcpy(msgOut,&idpedido,size);
								memcpy(msgOut+size,&self_list_node->ammount_synch,size);

								print_Console("handler WRITE a DISCO (Sincronizacion) de:",self_tid);
								praid_sl_content *new_data_sublist= malloc(sizeof(praid_sl_content));
								new_data_sublist->synch = true;
								new_data_sublist->status = UNREAD;//Unread
								new_data_sublist->socketPFS = self_list_node->socketPPD;
								new_data_sublist->msg = NIPC_createMsg(READ_SECTORS,size2,msgOut);

								free(msgOut);
								PRAID_ADD_READ(new_data_sublist);
							}else{//Si era el ultimo
								self_list_node->ppdStatus = READY; //Cambia estado de PPD a ACTIVO
								SYNCHRONIZING_DISCS = false;
								print_Console("handler Fin Sincronizacion: ",self_tid);
								log_debug(raid_log_file,"PRAID","Fin Sincronizacion");
							}
						}else{
							print_Console("handler READ a DISCO de:",self_tid);
						}
						pthread_mutex_unlock(&mutex_LIST);

					break;
					case SENT://EL THREAD YA LO LEYO AL PEDIDO Y SE LO ENVIO AL DISCO ASIGNADO, PERO ESPERA RESPUESTA
						pthread_mutex_lock(&mutex_LIST);
						QUEUE_appendNode(self_SL, current_sl_content);
						pthread_mutex_unlock(&mutex_LIST);

					break;
					case RECEIVED://EL MAIN RECIBIO UNA RESPUESTA DEL DISCO Y ACTUALIZO EL PEDIDO

						if(current_sl_content->synch == false){//NO ES PARA SINCRONIZACION
							char *msgToPFS = NIPC_toBytes(&current_sl_content->msg);
							uint16_t msgToPFS_len = *((uint16_t*) current_sl_content->msg.len);

							if(current_sl_content->msg.type == READ_SECTORS){
								send(current_sl_content->socketPFS,msgToPFS,msgToPFS_len+3,0);
								print_Console("handler READ de DISCO de: ",self_tid);
								log_debug(raid_log_file,"PRAID","Pedido de lectura enviado a proceso PPD");
								pthread_mutex_lock(&mutex_LIST);
							}else{//WRITE_SECTORS
								pthread_mutex_lock(&mutex_LIST);

								pthread_mutex_lock(&mutex_WRITE_QUEUE);
								uint32_t requestID;// = NIPC_getID(current_sl_content->msg);
								memcpy(&requestID,current_sl_content->msg.payload,4);
								queueNode_t* nodoWRITE = PRAID_GET_WRITE_NODE_BY_ID(requestID);
								praid_write_content* contenidoNodoWrite=((praid_write_content*) nodoWRITE->data);
								pthread_mutex_unlock(&mutex_WRITE_QUEUE);
								contenidoNodoWrite->threads_left--;
								if(contenidoNodoWrite->threads_left == 0){
									send (current_sl_content->socketPFS, msgToPFS, msgToPFS_len+3, 0);
									pthread_mutex_lock(&mutex_WRITE_QUEUE);//Saco nodo de la cola
									queueNode_t* aux = WRITE_QUEUE->begin;
									while(aux->next!=nodoWRITE){
										aux = aux->next;
									}
									aux->next = nodoWRITE->next;
									free(contenidoNodoWrite);
									free(nodoWRITE);
									pthread_mutex_unlock(&mutex_WRITE_QUEUE);

									print_Console("handler WRITE de DISCO de:",self_tid);
									log_debug(raid_log_file,"PRAID","Pedido de escritura enviado a todos los discos",self_tid);
								}

							}
						}else{//ES PARA SINCRONIZACION
							if(current_sl_content->msg.type == READ_SECTORS){
								pthread_mutex_lock(&mutex_LIST);
								print_Console("handler READ de DISCO (Sincronizacion) de:",self_tid);
								praid_sl_content *new_data_sublist= malloc(sizeof(praid_sl_content));
								new_data_sublist->synch = true;
								new_data_sublist->msg = current_sl_content->msg;
								new_data_sublist->status = UNREAD;
								new_data_sublist->socketPFS = current_sl_content->socketPFS;
								PRAID_ADD_WRITE(new_data_sublist);
							}
						}
						print_Console("handler Liberando Pedido.",self_tid);

						//NIPC_cleanMsg(&current_sl_content->msg);//
						free(current_sl_content);
						free(current_SL_node);
						print_Console("handler Pedido Liberado.",self_tid);
						pthread_mutex_unlock(&mutex_LIST);

					break;

				}//Fin switch(current_sl_content->status){
			}//Fin if(QUEUE_length(self_SL) > 0){
		break;
	}//Fin switch(self_list_node->ppdStatus){
}//Fin while(1)
return 0;
}//Fin thread
