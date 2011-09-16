/*
 * test.c
 *
 *  Created on: 15/09/2011
 *      Author: utn_so
 */
#include "config_manager.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

int main(int argc,char **argv[])
{
	printf("%d",sizeof(uint32_t));
	config_param *first_param;
	CONFIG_read("config/pfs.config",&first_param);

	char* value = CONFIG_getValue(first_param,"HOLA");

	return 0;


}
