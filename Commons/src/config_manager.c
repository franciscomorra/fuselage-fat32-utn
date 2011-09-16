/*
 * config_manager.c
 *
 *  Created on: 15/09/2011
 *      Author: utn_so
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "config_manager.h"



uint32_t CONFIG_read(const char *path,config_param **first_param)
{
	int config_filed = open(path,O_RDONLY);
	char *chr = malloc(1);
	char* tmp = malloc(100);

	config_param *last_param;
	config_param *new_param;

	int first = 1;
	 while (read(config_filed,chr,1) != 0)
	 {
		 if (strcmp(chr,";") == 0)
		 {
			 char* key;
			 char* p = strtok(tmp,"=");

			 new_param = malloc(sizeof(config_param));

			 strcpy(new_param->key,p);
			 p = strtok(NULL," ,");
			 strcpy(new_param->value,p);


			 if (first)
			 {
				 *first_param = new_param;
				  first=0;
			 }
			 else
			 {
				 last_param->nextParam = new_param;
			 }

			 	 last_param = new_param;
			 	 memset(tmp,0,100);

		 }
		 else if (strcmp(chr,"\n") != 0)
		 {
			 strcat(tmp,chr);
		 }
	 }

	 last_param->nextParam=NULL;
	 return 1;

}

char* CONFIG_getValue(config_param *first_param, const char* key)
{
		config_param *cur = first_param;
		while (1)
		{
			if (cur->nextParam != NULL)
			{
				if (strcmp(cur->key,key) == 0)
				{
					return cur->value;
				}
				else
				{
					cur = cur->nextParam;
				}
			}
			else if (strcmp(cur->key,key) == 0)
			{
				return cur->value;
			}
			else
			{
				return NULL;
			}

		}
		return NULL;
}