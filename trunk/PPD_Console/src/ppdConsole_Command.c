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
	char* msg = NIPC_createCharMsg(PPDCONSOLE_INFO,12,NULL);

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
	char* msg;

	uint32_t firstSector = atoi(parameters.begin->data);
	uint32_t lastSector = atoi(parameters.end->data);

	for(i=firstSector;i<=lastSector;i++){
		memcpy(payload,&i,sizeof(uint32_t));
		memset(payload + sizeof(uint32_t),'\0',512);
		msg = NIPC_createCharMsg(WRITE_SECTORS,516,payload);
		uint32_t sendReturn;
	    if ((sendReturn = send(ppdFD, msg, 519, 0)) == -1) {
	        perror("send");
	        exit(1);
	    }
	}
	free(msg);
	free(payload);
	return 1;
}

uint32_t console_trace(queue_t parameters,uint32_t len,uint32_t ppdFD){
	char* msg;
	queueNode_t* cur_parameter = parameters.begin;
	char* payload = malloc(sizeof(uint32_t)*5);
	uint32_t requestSector = 0;
	uint32_t i;

	while(cur_parameter != 0){
		requestSector = atoi(cur_parameter->data);
		memcpy(payload,&requestSector,4);

		msg = NIPC_createCharMsg(PPDCONSOLE_TRACE,sizeof(uint32_t),payload);
		uint32_t sendReturn;
	    if ((sendReturn = send(ppdFD, msg, 3+sizeof(uint32_t), 0)) == -1) {
	        perror("send");
	        exit(1);
	    }
	    cur_parameter = cur_parameter->next;
	}
	for(i=0;i<=len;i++){
		if(recv(ppdFD,msg,3+5*sizeof(uint32_t),0) == -1)  {
			    	perror("recv");
			    	exit(1);
		}
		console_showTrace(msg);
	}
	free(msg);
	free(payload);
	return 1;
}

void console_turnToCHS(uint32_t* sectorNum,CHS_t CHS){

	CHS.cylinder = (*sectorNum) / (Sector * Head);
	CHS.head = (*sectorNum % (Sector * Head)) / Sector;
	CHS.sector = (*sectorNum % (Sector * Head)) % Sector;

}

void console_showTrace(char* msg){
	CHS_t CHS;
	uint32_t distance;

	console_turnToCHS((uint32_t*)(msg+7),CHS);
	printf("Posición Actual: %d:%d:%d\n",CHS.cylinder,CHS.head,CHS.sector);
	console_turnToCHS((uint32_t*)(msg+3),CHS);
	printf("Sector Solicitado: %d:%d:%d\n",CHS.cylinder,CHS.head,CHS.sector);

	memcpy(&distance,msg+15,4);
	uint32_t sector = (Sector-distance+CHS.sector)%Sector;
	printf("Sectores Recorridos: ");
	for(;sector<=distance;sector++)
		printf("%d:%d:%d",CHS.cylinder,CHS.head,sector);

	printf("\nTiempo Consumido: %dms",*(msg+19));
	console_turnToCHS((uint32_t*)(msg+11),CHS);
	printf("Proximo Sector: %d:%d:%d\n",CHS.cylinder,CHS.head,CHS.sector);
}
