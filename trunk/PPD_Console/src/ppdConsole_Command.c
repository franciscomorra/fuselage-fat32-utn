

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

uint32_t console_clean(uint32_t firstSector, uint32_t lastSector){
	uint32_t i;
	char* payload = malloc(516);


	for(i=firstSector;i<=lastSector;i++){
		memcpy(payload,(uint32_t*)i,sizeof(uint32_t));
		memset(payload + sizeof(uint32_t),0,512);
		NIPC_createMsg(WRITE_SECTORS,516,payload);
		//Aca hay q enviar el mensaje al ppd
	}

	return 1;
}

uint32_t console_trace(uint32_t* traceSectors){

	return 1;
}
