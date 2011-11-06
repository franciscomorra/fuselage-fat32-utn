/*
 * comm.h
 *
 *  Created on: Nov 5, 2011
 *      Author: utn_so
 */

#ifndef COMM_H_
#define COMM_H_

char* COMM_recieve(uint32_t currFD,uint32_t* dataRecieved);

uint32_t COMM_send(char* msg,uint32_t fd);

#endif /* COMM_H_ */
