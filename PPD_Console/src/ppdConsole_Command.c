#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "tad_queue.h"
#include "ppdConsole_Command.h"
#include "nipc.h"

extern uint32_t Head;
extern uint32_t Sector;

uint32_t console_info(uint32_t ppdFD) {

	char* msg = malloc(12+3);
	NIPC_createCharMsg(msg,PPDCONSOLE_INFO,12,NULL);

    if (send(ppdFD, msg, 15, 0)== -1) {
        perror("send");
        exit(1);
    }
    if(recv(ppdFD,msg,15,0) == -1)  {
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
	char* payload = malloc(516);
	char* msg = malloc(519);

	uint32_t firstSector = atoi(parameters.begin->data);
	uint32_t lastSector = atoi(parameters.end->data);

	for(i=firstSector;i<=lastSector;i++){
		memcpy(payload,&i,sizeof(uint32_t));
		memset(payload + sizeof(uint32_t),'\0',512);
		NIPC_createCharMsg(msg,WRITE_SECTORS,516,payload);
		uint32_t sendReturn;
	    if ((sendReturn = send(ppdFD, msg, 519, 0)) == -1) {
	        perror("send");
	        exit(1);
	    }
	}
	free(payload);
	free(msg);
	return 1;
}

uint32_t console_trace(queue_t parameters,uint32_t len,uint32_t ppdFD){
	char* msgOut = malloc(3+sizeof(uint32_t));
	char* msgIn = malloc(3+sizeof(uint32_t)*5);
	queueNode_t* cur_parameter = parameters.begin;
	char* payload = malloc(sizeof(uint32_t)*5);
	uint32_t requestSector = 0;
	uint32_t i;

	while(cur_parameter != 0){
		requestSector = atoi(cur_parameter->data);
		memcpy(payload,&requestSector,4);
		NIPC_createCharMsg(msgOut,PPDCONSOLE_TRACE,sizeof(uint32_t),payload);

		uint32_t sendReturn;
	    if ((sendReturn = send(ppdFD, msgOut, 3+sizeof(uint32_t), 0)) == -1) {
	        perror("send");
	        exit(1);
	    }
	    cur_parameter = cur_parameter->next;
	}
	uint32_t recvLen=0;
	for(i=0;i<len;i++){
		NIPC_createCharMsg(msgIn,PPDCONSOLE_TRACE,sizeof(uint32_t)*5,NULL);
		if((recvLen=recv(ppdFD,msgIn,3+5*sizeof(uint32_t),0)) == -1)  {
			    	perror("recv");
			    	exit(1);
		}
		console_showTrace(msgIn);
	}
	free(payload);
	free(msgOut);
	free(msgIn); //rompe

	return 1;
}

void console_turnToCHS(uint32_t* sectorNum,CHS_t* CHS){

	CHS->cylinder = (*sectorNum) / (Sector * Head);
	CHS->head = (*sectorNum % (Sector * Head)) / Sector;
	CHS->sector = (*sectorNum % (Sector * Head)) % Sector;

}

void console_showTrace(char* msg){
	CHS_t CHS;
	CHS_t headPosition;
	uint32_t distance;
	uint16_t len;

	console_turnToCHS((uint32_t*)(msg+7),&headPosition);
	printf("Posición Actual: %d:%d:%d\n",headPosition.cylinder,headPosition.head,headPosition.sector);
	console_turnToCHS((uint32_t*)(msg+3),&CHS);
	printf("Sector Solicitado: %d:%d:%d\n",CHS.cylinder,CHS.head,CHS.sector);

	printf("Pistas Recorridas Desde: %d Hasta: %d\n",headPosition.cylinder,CHS.cylinder);

	memcpy(&distance,msg+11,4);
	uint32_t sector = (Sector-distance+CHS.sector)%Sector;
	uint32_t i;
	printf("Sectores Recorridos: ");
	for(i=0;i<=distance;i++)
		printf("%d:%d:%d ",CHS.cylinder,CHS.head,(sector+i)%16);

	printf("\nTiempo Consumido: %dms\n",(uint32_t)*(msg+15));

	memcpy(&len,msg+1,2);
	if(len == 20){
	console_turnToCHS((uint32_t*)(msg+19),&CHS);
	printf("Proximo Sector: %d:%d:%d\n",CHS.cylinder,CHS.head,CHS.sector);
	} else
		printf("Proximo Sector: -\n");
	putchar('\n');


}
