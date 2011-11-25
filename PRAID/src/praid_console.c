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
#include <stdbool.h>


extern uint32_t RAID_CONSOLE;
extern pthread_mutex_t mutex_CONSOLE;

uint32_t print_Console (char *message, pthread_t threadID, uint32_t priority, bool display_int)
{
	/*
	 *
	 * ESTADOS DE LA CONSOLA
	 * 0: DISABLE
	 * 1: MENSAJES REQUERIDOS POR CATEDRA
	 * 2: DEBUG
	 *
	 * */

	if(RAID_CONSOLE == 1){//CONSOLE ENABLE
		if(priority > 0){
			if(display_int==true){
				pthread_mutex_lock(&mutex_CONSOLE);
				printf("%s [%u]\n",message,threadID);
				pthread_mutex_unlock(&mutex_CONSOLE);
			}else{
				pthread_mutex_lock(&mutex_CONSOLE);
				printf("%s \n",message);
				pthread_mutex_unlock(&mutex_CONSOLE);
			}
		}
	}else if (RAID_CONSOLE == 2){//DEBUG MODE
		if(display_int==true){
			pthread_mutex_lock(&mutex_CONSOLE);
			printf("%s [%u]\n",message,threadID);
			pthread_mutex_unlock(&mutex_CONSOLE);
		}else{
			pthread_mutex_lock(&mutex_CONSOLE);
			printf("%s \n",message);
			pthread_mutex_unlock(&mutex_CONSOLE);
		}
	}

	return 0;

}
