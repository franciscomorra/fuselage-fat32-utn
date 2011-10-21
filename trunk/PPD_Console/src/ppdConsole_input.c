

#include "ppdConsole_input.h"

void CONSOLE_getCommand(char* input,char* command,uint32_t* parameters,uint32_t* paramLen){

	char s2[4] = " \n";
	char *ptr;
	uint32_t i = 0;
	size_t len;



	len = strcspn(input,s2);
	memcpy(command,strtok( input, s2 ),len);			// Primera llamada guarda el nombre de comando


	while( (ptr = strtok( NULL, s2 )) != NULL )  {		// Posteriores llamadas guardan los parametros
		*(parameters + i) = atoi (ptr);					// Guarda en el array los patametros
		i++;
	}

	memcpy(paramLen,&i,sizeof(uint32_t));

}

/*
uint32_t getCommand(char* input,char* command){

	char s2[4] = " \n";
	size_t len;
	len = strcspn(input,s2);
	memcpy(command,strtok( input, s2 ),len);


	return 1;
}

uint32_t getCleanSectors(char* input,uint32_t* fstSector,uint32_t* lstSector){
	char s2[4] = " \n";
	char *ptr;

	*fstSector = atoi (strtok( NULL, s2 ));		// Segunda llamada => Primer sector

	*lstSector = atoi (strtok( NULL, s2 ));		// Tercera llamada => Ultimo sector

	return 0;
}

uint32_t getTraceSectors(char* input,uint32_t* traceSectors){
	char s2[4] = " \n";
	char *ptr;
	uint32_t i = 0;

	 while( (ptr = strtok( NULL, s2 )) != NULL )  {		// Posteriores llamadas
		 *(traceSectors + i) = atoi (ptr);				// Guarda en el array los numeros de sectores
		 i++;
	 }


	return 0;
}
*/




