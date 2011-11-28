/*
 * comm.h
 *
 *  Created on: Nov 5, 2011
 *      Author: utn_so
 */

#ifndef COMM_H_
#define COMM_H_


char* COMM_receive(uint32_t currFD,uint32_t* dataReceived);

uint32_t COMM_send(char* msg,uint32_t fd);

char* COMM_receiveAll(uint32_t socket_fd,uint32_t* dataReceived,size_t *msg_len);

void COMM_sendAdvise(uint32_t socket_descriptor,uint32_t data_size,uint32_t msg_size);

void COMM_sendHandshake(uint32_t fd,char* payload,uint32_t payload_len);

char* COMM_receiveHandshake(uint32_t fd,uint32_t* received);

#endif /* COMM_H_ */
