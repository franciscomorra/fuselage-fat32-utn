/*
 * config_manager.h
 *
 *  Created on: 15/09/2011
 *      Author: utn_so
 */

#ifndef CONFIG_MANAGER_H_
#define CONFIG_MANAGER_H_

#include <sys/types.h>


typedef struct param
{
	char key[50];
	char value[50];
	struct config_param *nextParam;
} config_param;

int CONFIG_read(const char* path,config_param **first_param);
char* CONFIG_getValue(config_param *first_param, const char* key);

#endif /* CONFIG_MANAGER_H_ */
