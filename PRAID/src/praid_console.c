/*
 * praid_console.c
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "praid_console.h"


extern uint32_t RAID_CONSOLE;
extern pthread_mutex_t mutex_CONSOLE;

uint32_t print_Console (char *message, uint32_t threadID)
{

	if(RAID_CONSOLE == 1){
		pthread_mutex_lock(&mutex_CONSOLE);
		printf("%s %d\n",message,threadID);
		pthread_mutex_unlock(&mutex_CONSOLE);
	}

	return 0;

}

/*
uint32_t print_ConsoleInt (uint32_t nro)
{

	if(RAID_CONSOLE == 1){
		pthread_mutex_lock(&mutex_CONSOLE);
		printf("%d \n",nro);
		pthread_mutex_unlock(&mutex_CONSOLE);
	}

	return 0;
}*/
