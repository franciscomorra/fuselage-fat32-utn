#include "ppdConsole_input.h"

void CONSOLE_getCommand(char* input,char* command,queue_t* parameters,uint32_t* paramLen)
{

	char delimiters[4] = " \n";
	char *curr_parameter;
	uint32_t paramLenAux = 0;
	strcpy(command,strtok(input,delimiters));

	while	((curr_parameter = strtok(NULL,delimiters)) != NULL ) {
		QUEUE_appendNode(parameters,curr_parameter);
		paramLenAux++;
	}
	memcpy(paramLen,&paramLenAux,4);

}





