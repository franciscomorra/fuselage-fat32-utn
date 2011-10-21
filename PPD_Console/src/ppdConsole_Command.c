

#include <stdlib.h>
#include <string.h>
#include "ppdConsole_Command.h"
#include "nipc.h"


uint32_t console_info() {
	nipcMsg_t request;

	request = NIPC_createMsg(PPDCONSOLE_INFO,0,0);
	//Aca hay q enviar el mensaje "request" al ppd y el
	//thread encargado de la consola del ppd va a imprimir el HeadPosition


	return 1;
}

uint32_t console_clean(uint32_t* parameters){
	uint32_t i;
	nipcMsg_t msg;
	char* payload = malloc(516);

	uint32_t firstSector = *parameters;
	uint32_t lastSector = *(parameters + 1);

	for(i=firstSector;i<=lastSector;i++){
		memcpy(payload,&i,sizeof(uint32_t));
		memset(payload + sizeof(uint32_t),0,512);
		msg = NIPC_createMsg(WRITE_SECTORS,516,payload);
		//Aca hay q enviar el mensaje al ppd

		//Tengo que hacer el free del payload no???
	}

	return 1;
}

uint32_t console_trace(uint32_t* traceSectors,uint32_t len){
	uint32_t i;
	nipcMsg_t msg;
	char* payload = malloc(4);

	msg = NIPC_createMsg(PPDCONSOLE_TRACE,4,payload);

	return 1;
}
