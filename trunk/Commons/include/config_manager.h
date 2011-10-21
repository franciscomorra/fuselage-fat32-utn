/*
 * config_manager.h
 *
 *  Created on: 15/09/2011
 *      Author: utn_so
 */

#ifndef CONFIG_MANAGER_H_
#define CONFIG_MANAGER_H_

#include <stdint.h>


typedef struct config_param
{
	char key[50];
	char value[50];
	struct config_param *nextParam;
} config_param;

uint32_t CONFIG_read(const char *path,config_param **first_param);
char* CONFIG_getValue(config_param *first_param, const char* key);

#endif /* CONFIG_MANAGER_H_ */
