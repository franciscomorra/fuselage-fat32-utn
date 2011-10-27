#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "tad_queue.h"
#include "ppdConsole_Command.h"
#include "nipc.h"

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
	uint32_t i;
	nipcMsg_t msg;
	char* payload = malloc(4);
	queueNode_t* cur_parameter = parameters.begin;

	while(cur_parameter != 0){
		memcpy(payload,cur_parameter->data,sizeof(uint32_t));
		msg = NIPC_createMsg(PPDCONSOLE_TRACE,sizeof(uint32_t),payload);
		cur_parameter = cur_parameter->next;
		//Aca hay q enviar el mensaje al ppd
		//Tengo que hacer el free del payload no???
	}


	return 1;
}
