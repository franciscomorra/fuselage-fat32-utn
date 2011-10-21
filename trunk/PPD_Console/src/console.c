

#include <string.h>
#include "ppdConsole_input.h"
#include "ppdConsole_Command.h"


#define CANTMAX (7*5)+6+5												//TEMPORAL (va la cantidad maxima de letras q tiene lo ingresado por teclado)

uint32_t main () {
	char* input = malloc(CANTMAX);										//aloco memoria para el ingreso del teclado
	char* command = malloc(6*sizeof(char));					//aloco memoria para guardar el nombre del comando
	uint32_t* parameters = malloc(5*sizeof(uint32_t));			//aloco memoria para guardar los parametros
	uint32_t len;

	printf("Ingrese un Comando\n");
	if (fgets(input,CANTMAX,stdin) == 0 )
		printf("error fgets\n");

	CONSOLE_getCommand(input,command,parameters,&len);

	while((strcmp(command,"exit")) != 0){

		if ((strcmp(command,"info")) == 0)
			console_info();										//funcion que hace el info

		if ((strcmp(command,"clean")) == 0){
			if(len == 2)
				console_clean(parameters);						//funcion que hace el clean
			else
				printf("Cantidad erronea de parametros para el comando clean\n");
		}

		if ((strcmp(command,"trace")) == 0)
			console_trace(parameters,len);						//funcion que hace el trace


		//TODO Si no reconoce el comando entonces lo informa.

		printf("Ingrese un Comando\n");
		if (fgets(input,CANTMAX,stdin) == 0 )				//se ingresa el proximo comando
			printf("error fgets\n");

		CONSOLE_getCommand(input,command,parameters,&len);

	}

	free(input);
	free(command);
	free(parameters);
	//hacer frees necesarios

	return 0;
}

