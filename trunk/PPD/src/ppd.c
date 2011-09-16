/*
 * ppd.c
 *
 *  Created on: 06/09/2011
 *      Author: utn_so
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "config_manager.h"

uint32_t Cylinder;
uint32_t Head;
uint32_t Sector;


int main(int argc, char *argv[])
{
	config_param *ppd_config;
	CONFIG_read("config/ppd.config",&ppd_config);

	Cylinder = atoi(CONFIG_getValue(ppd_config,"Cylinder"));
	Head =  atoi(CONFIG_getValue(ppd_config,"Head"));
	Sector =  atoi(CONFIG_getValue(ppd_config,"Sector"));

	return 1;
}


