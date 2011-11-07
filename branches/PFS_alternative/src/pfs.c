/*
 * pfs.c
 *
 *  Created on: 06/10/2011
 *      Author: utn_so
 */


#include "fuselage_main.h"
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>


#include "pfs_fat32.h"
#include <sys/socket.h>
#include "tad_sockets.h"

#include "pfs_comm.h"
#include "tad_direntry.h"

	struct args
	{
		int argc;
		char **argv;
	} args;

char* cmd_received;
pthread_mutex_t signal_lock = PTHREAD_MUTEX_INITIALIZER;
socketPool_t sockets_toPPD;

void finish_him()
{
	fflush(stdout);
	execl("/bin/umount","",(args.argv)[2]);
	exit(0);
}

int main(int argc, char *argv[])
{
	//DIRENTRY_create("santi",5);

	sockets_toPPD = create_connections_pool(10,"127.0.0.1",9034);
	if (sockets_toPPD.size == 0) return 1;

	signal(SIGKILL,finish_him);
	signal(SIGINT,finish_him);
	signal(SIGTERM,finish_him);

	args.argc = argc;
	args.argv = argv;

		pthread_t fuselage_thread;
	//fuselage_main(0);
	pthread_create(&fuselage_thread,NULL,fuselage_main,NULL);

	char cmd[1024];
	memset(cmd,0,1024);
	char new_char;
	uint32_t char_index = 0;


	while(1)
	{
		printf("cmd-fuselage>");
		fflush(stdout);
		char_index = 0;

		while ((new_char = getchar()) != '\n')
		{
			memcpy(cmd+char_index,&new_char,1);
			memset(&new_char,0,1);
			char_index++;
		}

		char* token = strtok(cmd," ");

		if (strcmp(token,"exit") == 0)
		{
			raise(SIGINT);
		}
		else if (strcmp(token,"fsinfo") == 0)
		{
			cmd_received = malloc(sizeof(cmd));
			strcpy(cmd_received,cmd);
			pthread_mutex_lock(&signal_lock);
			pthread_kill(fuselage_thread,SIGUSR2);
			pthread_mutex_lock(&signal_lock);
			pthread_mutex_unlock(&signal_lock);
		}
		else if (strcmp(token,"finfo") == 0)
		{
			if ((token = strtok(NULL," ")) != NULL)
			{
				cmd_received = malloc(strlen("finfo ") + strlen(token));
				strcpy(cmd_received,"finfo ");
				strcat(cmd_received,token);

				pthread_mutex_lock(&signal_lock);
				pthread_kill(fuselage_thread,SIGUSR2);
				pthread_mutex_lock(&signal_lock);
				pthread_mutex_unlock(&signal_lock);
			}
		}


	}

	return 0;
}


