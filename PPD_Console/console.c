/*
 * console.c
 *
 *  Created on: Oct 6, 2011
 *      Author: utn_so
 */

#include "ppdConsole_input.h"

#define CANTMAX (7*5)+6+5

uint32_t main () {
	char* input = malloc(CANTMAX);
	char* command = malloc(6*sizeof(char));

	if (fgets(input,CANTMAX,stdin) !=0 )
		printf("error fgets");
	getCommand(input,command);

/*	while(1){
		switch(command)
		{
		case "exit":
			printf("opcion seleccionada: exit");
			exit;

		case "info":
			printf("opcion seleccionada: info");
			break;

		case "clear":
			printf("opcion seleccionada: clear");
			break;

		case "trace":
			printf("opcion seleccionada: trace");
			break;

		}
	}

*/
	return 0;
}

