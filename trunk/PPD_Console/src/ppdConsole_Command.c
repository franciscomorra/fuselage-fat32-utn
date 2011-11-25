#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "tad_queue.h"
#include "ppdConsole_Command.h"
#include "nipc.h"
#include "comm.h"
#include "log.h"

extern uint32_t* Head;
extern uint32_t* Sector;

uint32_t console_info(uint32_t ppdFD) {

	char* msg = malloc(12+3);
	uint32_t recvLen = 0;
	NIPC_createCharMsg(msg,PPDCONSOLE_INFO,12,NULL);

    if (COMM_send(msg,ppdFD) == -1) {
        perror("send");
        exit(1);
    }
	msg = COMM_receive(ppdFD,&recvLen);
		if(recvLen == -1){
			perror("recv");
			exit(1);
		}

    uint32_t C,H,S;
    memcpy(&C,msg+3,4);
    memcpy(&H,msg+7,4);
    memcpy(&S,msg+11,4);
    printf("La posicion actual del cabezal es: (%d,%d,%d).\n",C,H,S);
    free(msg);

	return 1;
}

uint32_t console_clean(queue_t parameters,uint32_t ppdFD){
	uint32_t i;
	char* payload = malloc(520);
	char* msg = malloc(523);
	fd_set readFDs;
	uint32_t recvs = 0;

	uint32_t firstSector = atoi(parameters.begin->data);
	uint32_t lastSector = atoi(parameters.end->data);
	uint32_t totalSectors = lastSector - firstSector;

	for(i=firstSector;i<=lastSector;i++){
		uint32_t recvLen=0;
		struct timeval val;
		val.tv_sec = 0;
		val.tv_usec = 0;
		FD_ZERO(&readFDs);
		FD_SET(ppdFD,&readFDs);
		select(ppdFD+1, &readFDs,NULL,NULL,&val);
			if(FD_ISSET(ppdFD,&readFDs)){
				recvs++;
				COMM_receive(ppdFD,&recvLen);
					if(recvLen == -1){
						perror("recv");
						exit(1);
					}
			}
		memset(payload,0,sizeof(uint32_t));
		memcpy(payload+4,&i,sizeof(uint32_t));
		memset(payload+8,'\0',512);
		NIPC_createCharMsg(msg,WRITE_SECTORS,520,payload);
	    if (COMM_send(msg,ppdFD) == -1) {
	        perror("send");
	        exit(1);
	    }
	}
	uint32_t recvLen=0;
	for(;recvs<totalSectors;recvs++){
		msg = COMM_receive(ppdFD,&recvLen);
			if(recvLen == -1){
				perror("recv");
				exit(1);
			}

		uint32_t numero;
		memcpy(&numero,msg+7,4);
		printf("consola: %d\n",numero);
		fflush(0);

	}

	printf("Se borro desde el sector: %d hasta el: %d\n",firstSector,lastSector);
	free(payload);
	free(msg);
	return 1;
}

uint32_t console_trace(queue_t parameters,uint32_t len,uint32_t ppdFD){
	char* msgOut = malloc(3+sizeof(uint32_t)*2);
	char* msgIn = malloc(3+sizeof(uint32_t)*5);
	queueNode_t* cur_parameter = parameters.begin;
	char* payload = malloc(sizeof(uint32_t)*5);
	uint32_t requestSector = 0;
	uint32_t i;

	while(cur_parameter != 0){
		requestSector = atoi(cur_parameter->data);
		memset(payload,0,sizeof(uint32_t));
		memcpy(payload+4,&requestSector,4);
		NIPC_createCharMsg(msgOut,PPDCONSOLE_TRACE,8,payload);

	    if (COMM_send(msgOut,ppdFD) == -1) {
	        perror("send");
	        exit(1);
	    }
	    cur_parameter = cur_parameter->next;
	}
	uint32_t recvLen=0;
	for(i=0;i<len;i++){
		NIPC_createCharMsg(msgIn,PPDCONSOLE_TRACE,sizeof(uint32_t)*5,NULL);
		msgIn = COMM_receive(ppdFD,&recvLen);
		if(recvLen == -1){
			perror("recv");
			exit(1);
		}
		log_showTrace(msgIn,stdout,*Sector,*Head,NULL);
	}
	free(payload);
	free(msgOut);
	free(msgIn); //rompe

	return 1;
}
