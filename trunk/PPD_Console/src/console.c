#include <string.h>
#include <sys/socket.h>

#include "ppdConsole_input.h"
#include "ppdConsole_Command.h"
#include "config_manager.h"
#include "tad_sockets.h"
#include "nipc.h"
#include "comm.h"
#include "tad_queue.h"

#define CANTMAX (7*5)+6+5									//TEMPORAL (va la cantidad maxima de letras q tiene lo ingresado por teclado)

uint32_t* Head;
uint32_t* Sector;
uint32_t james;

int main (int argc, char *argv[]) {
	char* input = malloc(CANTMAX);							//aloco memoria para el ingreso del teclado
	char* command = malloc(6*sizeof(char));					//aloco memoria para guardar el nombre del comando
	char* sockUnixPath = malloc(100);
	Head = malloc(sizeof(uint32_t));
	Sector = malloc(sizeof(uint32_t));
	queue_t parameters;
	QUEUE_initialize(&parameters);							//aloco memoria para guardar los parametros
	uint32_t len;
	james = 0;

	*Head = (uint32_t)*argv[0];
	*Sector = (uint32_t)*argv[1];
	strcpy(sockUnixPath,argv[2]);

	socketUnix_t ppd_socket = SOCKET_unix_create(SOCK_STREAM,sockUnixPath,MODE_CONNECT);		//se coneccta al proceso PPD
	printf("Connected.\n");

/*
	*Head = 1;
	*Sector = 16;
	socketUnix_t ppd_socket = SOCKET_unix_create(SOCK_STREAM,"/home/utn_so/CONSOLE_socket",MODE_CONNECT);		//se coneccta al proceso PPD
		printf("Connected.\n");
*/
	printf("Ingrese un Comando\n");
	if (fgets(input,CANTMAX,stdin) == 0 )
		printf("error fgets\n");


	CONSOLE_getCommand(input,command,&parameters,&len);

	while((strcmp(command,"exit")) != 0){

		if ((strcmp(command,"info")) == 0)
			console_info(ppd_socket.descriptor);									//funcion que hace el info

		if ((strcmp(command,"clean")) == 0){
			if((len == 2)&&(atoi(parameters.begin->data) < atoi(parameters.begin->next->data)))
				console_clean(parameters,ppd_socket.descriptor);					//funcion que hace el clean
			else if(len < 2 || len > 2)
				printf("Cantidad de parametros erronea \n");
			else if(atoi(parameters.begin->data) > atoi(parameters.begin->next->data))
				printf("El primer parametro debe ser menor que el segundo\n");
		}

		if ((strcmp(command,"trace")) == 0)
			console_trace(parameters,len,ppd_socket.descriptor);					//funcion que hace el trace

		if((strcmp(command,"james")) ==  0){
			james = 1;
			console_info(ppd_socket.descriptor);
			james = 0;
		}

		//TODO Si no recosnoce el comando entonces lo informa.

		memset(command,0,6);
		QUEUE_cleanQueue(&parameters);

		printf("Ingrese un Comando\n");
		if (fgets(input,CANTMAX,stdin) == 0 )				//se ingresa el proximo comando
			printf("error fgets\n");

		CONSOLE_getCommand(input,command,&parameters,&len);
	}

	char* exitMsg = malloc(sizeof(char)*3);
	NIPC_createCharMsg(exitMsg,PPDCONSOLE_EXIT,0,NULL);

	if (COMM_send(exitMsg,ppd_socket.descriptor) == -1) {
		perror("send");
		exit(1);
	}
	free(exitMsg);
	free(input);
	free(command);
	free(sockUnixPath);
	free(Head);
	free(Sector);
	free(ppd_socket.path);
	//hacer frees necesarios
	return EXIT_SUCCESS;
}

