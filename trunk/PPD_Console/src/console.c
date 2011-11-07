#include <string.h>
#include <sys/socket.h>
#include "ppdConsole_input.h"
#include "ppdConsole_Command.h"
#include "config_manager.h"
#include "tad_sockets.h"

#define CANTMAX (7*5)+6+5									//TEMPORAL (va la cantidad maxima de letras q tiene lo ingresado por teclado)
#define SOCK_PATH "/home/utn_so/CONSOLE_socket"

uint32_t Head;
uint32_t Sector;

void main () {
	char* input = malloc(CANTMAX);							//aloco memoria para el ingreso del teclado
	char* command = malloc(6*sizeof(char));					//aloco memoria para guardar el nombre del comando
	queue_t parameters;
	QUEUE_initialize(&parameters);							//aloco memoria para guardar los parametros
	uint32_t len;

	config_param *ppd_config;
	CONFIG_read("/home/utn_so/Desktop/trabajos/PPD/config/ppd.config",&ppd_config);

	Head =  atoi(CONFIG_getValue(ppd_config,"Head"));					//
	Sector =  atoi(CONFIG_getValue(ppd_config,"Sector"));				//	leer archivo de configuración

	socketUnix_t ppd_socket = SOCKET_unix_create(SOCK_STREAM,SOCK_PATH,MODE_CONNECT);		//se coneccta al proceso PPD
	printf("Connected.\n");

	printf("Ingrese un Comando\n");
	if (fgets(input,CANTMAX,stdin) == 0 )
		printf("error fgets\n");


	CONSOLE_getCommand(input,command,&parameters,&len);

	while((strcmp(command,"exit")) != 0){

		if ((strcmp(command,"info")) == 0)
			console_info(ppd_socket.descriptor);									//funcion que hace el info

		if ((strcmp(command,"clean")) == 0){
			if(len == 2)
				console_clean(parameters,ppd_socket.descriptor);					//funcion que hace el clean
			else
				printf("Cantidad de parametros erronea \n");
		}

		if ((strcmp(command,"trace")) == 0)
			console_trace(parameters,len,ppd_socket.descriptor);					//funcion que hace el trace


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
	//hacer frees necesarios

}

