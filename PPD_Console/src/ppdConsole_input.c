

#include "ppdConsole_input.h"

uint32_t getCommand(char* input,char* command){
	uint32_t i;

	for(i = 0;(input[i] != '\0') & (input[i] != ' ' );i++)
		command[i] = input[i];

	return 1;
}

uint32_t getCleanSectors(char* input,uint32_t* fstSector,uint32_t* lstSector){
	char s2[4] = " \n\t";
	char *ptr;

	ptr = strtok( input, s2 );    				// Primera llamada => Primer token (clean)

	*fstSector = atoi (strtok( NULL, s2 ));		// Segunda llamada => Primer sector

	*lstSector = atoi (strtok( NULL, s2 ));		// Tercera llamada => Ultimo sector

	return 0;
}

uint32_t getTraceSectors(char* input,uint32_t* traceSectors){
	char s2[4] = " \n\t";
	char *ptr;
	uint32_t i = 0;

	ptr = strtok( input, s2 );    						// Primera llamada => Primer token (trace)

	 while( (ptr = strtok( NULL, s2 )) != NULL )  {		// Posteriores llamadas
		 *(traceSectors + i) = atoi (ptr);				// Guarda en el array los numeros de sectores
		 i++;
	 }


	return 0;
}



