/*
 * console.c
 *
 *  Created on: Oct 6, 2011
 *      Author: utn_so
 */

#include <string.h>
#include "ppdConsole_input.h"
#include "ppdConsole_Command.h"


#define CANTMAX (7*5)+6+5												//TEMPORAL (va la cantidad maxima de letras q tiene lo ingresado por teclado)

uint32_t main () {
	char* input = malloc(CANTMAX);										//aloco memoria para el ingreso del teclado
	char* command = malloc(6*sizeof(char));								//aloco memoria para guardar el nombre del comando
	uint32_t* traceSectors = (uint32_t*)malloc(5*sizeof(uint32_t)) ;	//aloco memoria para el vector de sectores del trace
	uint32_t fstSector;
	uint32_t lstSector;


	if (fgets(input,CANTMAX,stdin) == 0 )
		printf("error fgets");
	getCommand(input,command);								//funcion que obtiene el comando
	printf("%s",command);

	while((strcmp(command,"exit"))!=0){

		if (!(strcmp(command,"info")))
			console_info();									//funcion que hace el info

		if (!(strcmp(command,"clean")))
			getCleanSectors(input,&fstSector,&lstSector);	//obtiene los parametros del clean y los guarda en las variables
			console_clean(fstSector,lstSector);				//funcion que hace el clean

		if (!(strcmp(command,"trace")))
			getTraceSectors(input,traceSectors);			//obtiene los parametros del trace y los guarda en un array
			console_trace(traceSectors);					//funcion que hace el trace


		if (fgets(input,CANTMAX,stdin) == 0 )				//se ingresa el proximo comando
			printf("error fgets");
		getCommand(input,command);
		printf("%s",command);


	}

	free(input);
	free(command);
	free(traceSectors);
	//hacer frees necesarios

	return 0;
}

