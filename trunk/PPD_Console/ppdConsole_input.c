/*
 * ppdConsole_input.c
 *
 *  Created on: Oct 6, 2011
 *      Author: utn_so
 */

#include "ppdConsole_input.h"

uint32_t getCommand(char* input,char* command){
	uint32_t i;

	for(i = 0;(input[i] != '\0') & (input[i] != ' ' );i++)
		command[i] = input[i];

	return 1;
}
