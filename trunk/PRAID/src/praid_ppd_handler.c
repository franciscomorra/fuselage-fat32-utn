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
	queueNode_t* current_SL_node = self_SL->begin;

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
				SYNCHRONIZING_DISCS = true;
				self_list_node->ppdStatus = SYNCHRONIZING;
				PRAID_START_SYNCHR(self_list_node->socketPPD);
			}else{
				print_Console("Continua Espera Sincronizacion",self_tid,1,true);
			}
			pthread_mutex_unlock(&mutex_LIST);
		break;
		default:
			//Ya esta sincronizado
			//INICIO MIRAR LA COLA

			//while(count < (QUEUE_length(self_SL)))
			//while(QUEUE_length(self_SL) > 0)
			while(current_SL_node!=NULL)
			{
				pthread_mutex_lock(&mutex_LIST);
				praid_sl_content* current_sl_content =((praid_sl_content*) current_SL_node->data);

				switch(current_sl_content->status){
					case SENT://EL THREAD YA LO LEYO AL PEDIDO Y SE LO ENVIO AL DISCO ASIGNADO, PERO ESPERA RESPUESTA
						current_SL_node = current_SL_node->next;
						pthread_mutex_unlock(&mutex_LIST);
					break;
					case UNREAD://PEDIDO NUEVO
						//pthread_mutex_lock(&mutex_LIST);
						//QUEUE_appendNode(self_SL, current_sl_content);
						if(current_sl_content->synch == true ){
							if(current_sl_content->msg.type == WRITE_SECTORS){ //EL THREAD ES MIRROR, PEDIDO PARA QUE ESCRIBA EN EL DISCO
								if(self_list_node->ammount_synch < DISK_SECTORS_AMOUNT){//SI NO ES EL ULTIMO SECTOR
									self_list_node->ammount_synch++;
									uint32_t idpedido = self_list_node->socketPPD;
									uint32_t size = sizeof(uint32_t);
									int size2 = 2*size;
									char* msgOut = malloc(size2);//CREO PEDIDO NUEVO DE LECTURA PARA EL PROXIMO SECTOR
									memcpy(msgOut,&idpedido,size);
									memcpy(msgOut+size,&self_list_node->ammount_synch,size);
									//print_Console("WRITE a DISCO (Sincronizacion) de:",self_tid,1,true);
									praid_sl_content *new_data_sublist= malloc(sizeof(praid_sl_content));
									new_data_sublist->synch = true;
									new_data_sublist->status = UNREAD;
									new_data_sublist->socketRequest = self_list_node->socketPPD;
									new_data_sublist->msg = NIPC_createMsg(READ_SECTORS,size2,msgOut);
									free(msgOut);
									PRAID_ADD_READ(new_data_sublist);//LE AGREGO EL PEDIDO A ALGUN DISCO READY
								}else{//SI ES EL ULTIMO SECTOR
									self_list_node->ppdStatus = READY; //Cambia estado de PPD a ACTIVO
									SYNCHRONIZING_DISCS = false;
									print_Console("FIN SINCRONIZACION: ",self_tid,1,true);
									log_debug(raid_log_file,"PRAID","Fin Sincronizacion");
								}
							}else if(current_sl_content->msg.type == READ_SECTORS){//EL THREAD ES MASTER Y MANDO UN PEDIDOD AL DISCO PARA QUE LEA
							}//FIN IF ELSE TYPE
						}//FIN IF ELSE SYNCH
						current_sl_content->status = SENT;
						char *msgToPPD = NIPC_toBytes(&current_sl_content->msg);//SACO EL NIPC DEL PEDIDO
						uint16_t msgToPPD_len = *((uint16_t*) current_sl_content->msg.len);//SACO EL LENGTH

						current_SL_node = current_SL_node->next;//PASO AL SIGUIENTE PEDIDO
						pthread_mutex_unlock(&mutex_LIST);
						//Inicio envio a PPD
						//print_Console("PEDIDO ENVIADO A DISCO",self_tid,1,true);
						send(self_list_node->socketPPD,msgToPPD,msgToPPD_len+3,0);//SE LO ENVIO AL DISCO ASOCIADO AL THREAD
						free(msgToPPD);
					break;
					case RECEIVED://EL MAIN RECIBIO UNA RESPUESTA DEL DISCO Y ACTUALIZO EL PEDIDO
						if(current_sl_content->synch == true){//ES PARA SINCRONIZACION
							if(current_sl_content->msg.type == READ_SECTORS){//EL THREAD ES MASTER Y LEYO UN SECTOR
								//pthread_mutex_lock(&mutex_LIST);
								//print_Console("READ SINCR. LEIDO POR MASTER",self_tid,1,true);
								praid_sl_content *new_data_sublist= malloc(sizeof(praid_sl_content));
								new_data_sublist->synch = true;
								new_data_sublist->msg = current_sl_content->msg;
								new_data_sublist->msg.type = WRITE_SECTORS;//LO QUE ME LLEGO DEL DISCO, LO TIENE QUE ESCRIBIR EL MIRROR
								new_data_sublist->status = UNREAD;
								//new_data_sublist->socketRequest;

								if(PRAID_ADD_WRITE(new_data_sublist)!=0){//SE LO AGREGO AL MIRROR PARA QUE ESCRIBA
									print_Console("LIMPIANDO COLA DE PEDIDODS DE SINCRONIZACION \n",self_tid,1,false);

									queueNode_t* aux = current_SL_node->next;
									while(aux!=NULL){
										praid_sl_content* aux_sl_content =((praid_sl_content*) aux->data);
										if(aux_sl_content->synch == true){
											praid_sl_content* aux1 = aux->next;
											free(aux_sl_content);
											free(aux);
											aux = aux1;
										}
									}

								}

							}else if(current_sl_content->msg.type == WRITE_SECTORS){//EL THREAD ES MIRROR Y RECIBIO LA RESPUESTA DE ESCRITURA DEL DISCO

							}//FIN IF ELSE TYPE READ WRITE
						}else{//NO ES PARA SINCRONIZACION


							char *msgToPFS = NIPC_toBytes(&current_sl_content->msg);
							uint16_t msgToPFS_len = *((uint16_t*) current_sl_content->msg.len);
							if(current_sl_content->msg.type == READ_SECTORS){//PEDIDO NORMAL DE LECTURA, YA DEVUELTO POR EL DISCO
								send(current_sl_content->socketRequest,msgToPFS,msgToPFS_len+3,0);
								free(msgToPFS);
								//print_Console("READ A PFS",self_tid,1,true);
								log_debug(raid_log_file,"PRAID","READ A PFS");
								//pthread_mutex_lock(&mutex_LIST);
							}else if (current_sl_content->msg.type == WRITE_SECTORS){//PEDIDO NORMAL DE ESCRITURA, YA DEVUELTO POR EL DISCO
								//pthread_mutex_lock(&mutex_LIST);
								pthread_mutex_lock(&mutex_WRITE_QUEUE);
								uint32_t requestID;// = NIPC_getID(current_sl_content->msg);
								memcpy(&requestID,current_sl_content->msg.payload,4);
								queueNode_t* nodoWRITE = PRAID_GET_WRITE_NODE_BY_ID(requestID);
								praid_write_content* contenidoNodoWrite=((praid_write_content*) nodoWRITE->data);
								contenidoNodoWrite->threads_left--;
								if(contenidoNodoWrite->threads_left == 0){
									send (current_sl_content->socketRequest, msgToPFS, msgToPFS_len+3, 0);
									free(msgToPFS);
									queueNode_t* aux = WRITE_QUEUE->begin;
									while(aux->next!=nodoWRITE){
										aux = aux->next;
									}
									aux->next = nodoWRITE->next;
									free(contenidoNodoWrite);
									free(nodoWRITE);

									//print_Console("WRITE A PFS",self_tid,1,true);
									log_debug(raid_log_file,"PRAID","WRITE A PFS",self_tid,1,true);
								}
								pthread_mutex_unlock(&mutex_WRITE_QUEUE);
							}//FIN IF ELSE TYPE
						}//FIN IF ELSE SINC
						//NIPC_cleanMsg(&current_sl_content->msg);//

						free(current_sl_content);
						queueNode_t* aux = current_SL_node;

						if(self_SL->begin == current_SL_node){
							self_SL->begin = current_SL_node->next;
						}else{
							queueNode_t* anterior = self_SL->begin;
							do{
								anterior = anterior->next;
							}while(anterior->next!=current_SL_node || anterior == NULL);
							if(anterior->next==current_SL_node){
								if(self_SL->end == current_SL_node){
									self_SL->end = anterior;
								}
								anterior->next = current_SL_node->next;
							}else{
								print_Console("ERROR ELIMINANDO PEDIDO",pthread_self(),1,true);
								exit(0);
							}
						}


						current_SL_node = current_SL_node->next;
						free(aux);
					//	print_Console("Pedido Liberado.",self_tid,1,true);
						pthread_mutex_unlock(&mutex_LIST);

					break;

				}//Fin switch(current_sl_content->status)
			}//Fin while hay pedidos
		break;
	}//Fin switch(self_list_node->ppdStatus)
}//Fin while(1)
return 0;
}//Fin thread
