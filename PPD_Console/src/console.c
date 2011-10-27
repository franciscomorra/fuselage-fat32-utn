
#include <string.h>
#include "ppdConsole_input.h"
#include "ppdConsole_Command.h"
#include "ppdConsole_connect.h"

#define CANTMAX (7*5)+6+5									//TEMPORAL (va la cantidad maxima de letras q tiene lo ingresado por teclado)
#define SOCK_PATH "/home/utn_so/CONSOLE_socket"

void main () {
	char* input = malloc(CANTMAX);							//aloco memoria para el ingreso del teclado
	char* command = malloc(6*sizeof(char));					//aloco memoria para guardar el nombre del comando
	queue_t parameters;
	QUEUE_initialize(&parameters);							//aloco memoria para guardar los parametros
	uint32_t len;
	uint32_t ppdFD;

    CONNECT_toProcess(&ppdFD);

	printf("Ingrese un Comando\n");
	if (fgets(input,CANTMAX,stdin) == 0 )
		printf("error fgets\n");


	CONSOLE_getCommand(input,command,&parameters,&len);

	while((strcmp(command,"exit")) != 0){

		if ((strcmp(command,"info")) == 0)
			console_info(ppdFD);									//funcion que hace el info

		if ((strcmp(command,"clean")) == 0){
			if(len == 2)
				console_clean(parameters,ppdFD);					//funcion que hace el clean
			else
				printf("Cantidad de parametros erronea \n");
		}

		if ((strcmp(command,"trace")) == 0)
			console_trace(parameters,len,ppdFD);					//funcion que hace el trace


		//TODO Si no reconoce el comando entonces lo informa.

		memset(command,0,6);
		QUEUE_cleanQueue(&parameters,0);

		printf("Ingrese un Comando\n");
		if (fgets(input,CANTMAX,stdin) == 0 )				//se ingresa el proximo comando
			printf("error fgets\n");

		CONSOLE_getCommand(input,command,&parameters,&len);
	}

	free(input);
	free(command);
	//free(parameters);
	//hacer frees necesarios

}

