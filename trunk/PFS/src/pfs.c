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

	struct args
	{
		int argc;
		char **argv;
	} args;

char* cmd_received;
pthread_mutex_t signal_lock = PTHREAD_MUTEX_INITIALIZER;

void finish_him()
{
	fflush(stdout);
	execl("/bin/umount","",(args.argv)[2]);
	exit(0);
}

int main(int argc, char *argv[])
{


	signal(SIGKILL,finish_him);
	signal(SIGINT,finish_him);
	signal(SIGTERM,finish_him);
	args.argc = argc;
	args.argv = argv;


	pthread_t fuselage_thread;
	//fuselage_main(0);
	pthread_create(&fuselage_thread,NULL,fuselage_main,NULL);

	char cmd[300];

	while(1)
	{

		printf("cmd-fuselage>");
		fflush(stdout);
		scanf("%s",cmd);

		if (strcmp(cmd,"exit") == 0)
		{
			raise(SIGINT);
		}
		else if (strcmp(cmd,"fsinfo") == 0)
		{
			cmd_received = malloc(sizeof(cmd));
			strcpy(cmd_received,cmd);
			pthread_kill(fuselage_thread,SIGUSR2);
			sleep(1);
			pthread_mutex_lock(&signal_lock);
			free(cmd_received);
			pthread_mutex_unlock(&signal_lock);
		}


	}

	return 0;
}


