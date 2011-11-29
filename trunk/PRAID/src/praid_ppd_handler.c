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
extern bool SYNCHRONIZING_DISCS; //0 INACTIVE - 1 ACTIVE


void *ppd_handler_thread (void *thread_data)
{

uint32_t self_tid = pthread_self(); //TID del PPD
praid_ppdThreadParam* handler_param = (praid_ppdThreadParam*)thread_data;

pthread_mutex_lock(&mutex_LIST);
praid_list_node* self_list_node = PRAID_ADD_PPD_NODE(self_tid, handler_param);
queue_t* self_SL = self_list_node->colaSublista;
pthread_mutex_unlock(&mutex_LIST);
//free(handler_param);

while(1){
	sem_wait(&self_list_node->request_list_sem);

	switch(self_list_node->ppdStatus){
		case DISCONNECTED://Se desconecto y el main me actualizo el estado
			PRAID_REMOVE_PPD(self_list_node);
			return NULL;
		break;
		case WAIT_SYNCH:  //Si se conecto pero hay un disco sincronizandose, espera a sincronizarse
			if(SYNCHRONIZING_DISCS == false){//Se empieza a sincronizar
				SYNCHRONIZING_DISCS = true;
				print_Console("INICIANDO SINCRONIZACION",self_list_node->diskID,1,true);
				PRAID_WRITE_LOG("Iniciando Sincronizacion");
				//log_debug(raid_log_file,"PRAID","Iniciando Sincronizacion");
				self_list_node->ppdStatus = SYNCHRONIZING;
				uint32_t sectores = PRAID_START_SYNCHR(self_list_node->socketPPD);
				print_Console("AGREGADOS TODOS LOS PEDIDOS DE SINCRONIZACION",sectores,1,true);
			}else{
				print_Console("YA HAY DISCOS SINCRONIZANDOSE, ESPERANDO",self_tid,1,true);
			}
		break;
		default://READY O SYNCHRONIZING
			{
			queueNode_t* current_SL_node = self_SL->begin;

			while(current_SL_node!=NULL)
			{
				praid_sl_content* current_sl_content =((praid_sl_content*) current_SL_node->data);
				switch(current_sl_content->status){
					case SENT://EL THREAD YA LO LEYO AL PEDIDO Y SE LO ENVIO AL DISCO ASIGNADO, PERO ESPERA RESPUESTA
						pthread_mutex_lock(&mutex_LIST);
						current_SL_node = current_SL_node->next;
						pthread_mutex_unlock(&mutex_LIST);
					break;
					case UNREAD://PEDIDO NUEVO
					{
						pthread_mutex_lock(&mutex_LIST);
						char *msgToPPD = NIPC_toBytes(&current_sl_content->msg);//SACO EL NIPC DEL PEDIDO
						uint16_t msgToPPD_len = *((uint16_t*) current_sl_content->msg.len);//SACO EL LENGTH
						if(current_sl_content->synch == true ){
							if(current_sl_content->msg.type == WRITE_SECTORS){ //EL THREAD ES MIRROR, PEDIDO PARA QUE ESCRIBA EN EL DISCO
								memcpy(&self_list_node->ammount_synch,&msgToPPD+3,(sizeof(uint32_t)));

								if(self_list_node->ammount_synch < DISK_SECTORS_AMOUNT){//SI NO ES EL ULTIMO SECTOR
									self_list_node->ammount_synch++;
									/* SE VA A MANDAR A MEDIDA QUE LLEGUEN*/
									/*
									praid_sl_content* new_request_data = NEW_SYNCH_REQUEST(self_list_node->ammount_synch);
									PRAID_ADD_READ(new_request_data);
									*/
								}else{//SI ES EL ULTIMO SECTOR
									self_list_node->ppdStatus = READY; //Cambia estado de PPD a ACTIVO
									SYNCHRONIZING_DISCS = false;
									PRAID_ACTIVATE_NEXT_SYNCH();//ACTIVAR EL PROXIMO EN WAIT
									print_Console("FIN SINCRONIZACION: ",self_list_node->diskID,1,true);
									PRAID_WRITE_LOG("Fin Sincronizacion");
									//log_debug(raid_log_file,"PRAID","Fin Sincronizacion");
								}
							}else if(current_sl_content->msg.type == READ_SECTORS){//EL THREAD ES MASTER Y MANDO UN PEDIDOD AL DISCO PARA QUE LEA
							}//FIN IF ELSE TYPE
						}//FIN IF ELSE SYNCH
						current_sl_content->status = SENT;

						current_SL_node = current_SL_node->next;//PASO AL SIGUIENTE PEDIDO
						pthread_mutex_unlock(&mutex_LIST);

//**********************************************************************
						fd_set writeFDs;
						uint32_t sent = 0;
						FD_ZERO(&writeFDs);
						FD_SET(self_list_node->socketPPD,&writeFDs);
						if(select((self_list_node->socketPPD) +1,NULL,&writeFDs,NULL,NULL) == -1)
							perror("select");
						while(sent == 0){
							if(FD_ISSET(self_list_node->socketPPD,&writeFDs)){
								sent = send(self_list_node->socketPPD,msgToPPD,msgToPPD_len+3,0);//SE LO ENVIO AL DISCO ASOCIADO AL THREAD
							}
						}
//**********************************************************************
						free(msgToPPD);


					break;
					}
					case RECEIVED://EL MAIN RECIBIO UNA RESPUESTA DEL DISCO Y ACTUALIZO EL PEDIDO
					{
						pthread_mutex_unlock(&mutex_LIST);
						bool toSend = false;
						char *msgToPFS = NIPC_toBytes(&current_sl_content->msg);
						uint16_t msgToPFS_len = *((uint16_t*) current_sl_content->msg.len);

						if(current_sl_content->synch == true){//ES PARA SINCRONIZACION

							if(current_sl_content->msg.type == READ_SECTORS){//EL THREAD ES MASTER Y LEYO UN SECTOR
								//print_Console("READ SINCR. LEIDO POR MASTER",self_tid,1,true);
								praid_sl_content *new_request_data= malloc(sizeof(praid_sl_content));//CREO NUEVO PEDIDO DE ESCRITURA
								new_request_data->synch = true;
								new_request_data->msg = current_sl_content->msg;
								new_request_data->msg.type = WRITE_SECTORS;//LO QUE ME LLEGO DEL DISCO, LO TIENE QUE ESCRIBIR EL MIRROR
								new_request_data->status = UNREAD;
								//new_data_sublist->socketRequest;
								if(PRAID_ADD_WRITE(new_request_data)!=0){//SE LO AGREGO AL MIRROR PARA QUE ESCRIBA
									print_Console("LIMPIANDO COLA DE PEDIDOS DE SINCRONIZACION",self_list_node->diskID,2,true);//EL DISCO SINCR. SE DESCONECTO
									queueNode_t* aux_request_node = current_SL_node->next;
									uint32_t count = 0;
									while(aux_request_node!=NULL){
										count++;
										praid_sl_content* aux_sl_content =((praid_sl_content*) aux_request_node->data);
										if(aux_sl_content->synch == true){
											queueNode_t* sync_request_node = aux_request_node;
											QUEUE_SEARCH_REMOVE(aux_request_node, self_SL);
											free(aux_sl_content->msg.payload);
											free(aux_sl_content);
											free(sync_request_node);
										}
										aux_request_node = aux_request_node->next;

									}
									print_Console("CANTIDAD DE PEDIDOS DESCARTADOS",count,1,true);
									print_Console("\n",self_tid,2,false);

								}
							}else if(current_sl_content->msg.type == WRITE_SECTORS){//EL THREAD ES MIRROR Y RECIBIO LA RESPUESTA DE ESCRITURA DEL DISCO

							}//FIN IF ELSE TYPE READ WRITE
						}else{//NO ES PARA SINCRONIZACION
							if(current_sl_content->msg.type == READ_SECTORS){//PEDIDO NORMAL DE LECTURA, YA DEVUELTO POR EL DISCO
								toSend = true;
							}else if (current_sl_content->msg.type == WRITE_SECTORS){//PEDIDO NORMAL DE ESCRITURA, YA DEVUELTO POR EL DISCO
								//pthread_mutex_lock(&mutex_LIST);
								pthread_mutex_lock(&mutex_WRITE_QUEUE);
								uint32_t requestID;// = NIPC_getID(current_sl_content->msg);
								memcpy(&requestID,current_sl_content->msg.payload,sizeof(uint32_t));
								queueNode_t* write_queue_node = PRAID_GET_WRITE_NODE_BY_ID(requestID);
								praid_write_content* contenidoNodoWrite=((praid_write_content*) write_queue_node->data);
								contenidoNodoWrite->threads_left--;
								pthread_mutex_unlock(&mutex_WRITE_QUEUE);

								if(contenidoNodoWrite->threads_left == 0){
									queueNode_t* write_remove_node = write_queue_node;
									pthread_mutex_lock(&mutex_WRITE_QUEUE);
									QUEUE_SEARCH_REMOVE(write_queue_node,WRITE_QUEUE);
									free(contenidoNodoWrite);
									free(write_remove_node);
									pthread_mutex_unlock(&mutex_WRITE_QUEUE);
									toSend = true;
								}
							}//FIN IF ELSE TYPE
						}//FIN IF ELSE SINC
						char* payload = current_sl_content->msg.payload;
						free(payload);
						free(current_sl_content);
						queueNode_t* sublist_remove_node = current_SL_node;
						if(QUEUE_SEARCH_REMOVE(current_SL_node, self_SL)!=true){
							print_Console("ERROR ELIMINANDO NODO.",self_list_node->diskID,1,true);
							exit(0);
						}
						current_SL_node = current_SL_node->next;
						free(sublist_remove_node);
						pthread_mutex_unlock(&mutex_LIST);
						if(toSend == true){
							send(current_sl_content->socketRequest,msgToPFS,msgToPFS_len+3,0);
						}
						free(msgToPFS);

					break;
					}
				}//Fin switch(current_sl_content->status)
			}//Fin while hay pedidos
		break;
		}
	}//Fin switch(self_list_node->ppdStatus)
}//Fin while(1)
return 0;
}//Fin thread
