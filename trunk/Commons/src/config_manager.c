/*
 * config_manager.c
 *
 *  Created on: 15/09/2011
 *      Author: utn_so
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "config_manager.h"



uint32_t CONFIG_read(const char *path,config_param **first_param)
{
	int config_filed = open(path,O_RDONLY);
	char* chr = malloc(1);
	char* tmp = malloc(200);

	config_param *last_param;
	config_param *new_param;

	int first = 1;
	 while (read(config_filed,chr,1) != 0)
	 {
		 if (strncmp(chr,";",1) == 0)
		 {
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
			 	 memset(tmp,0,200);

		 }
		 else if (strncmp(chr,"\n",1) != 0)
		 {
			 strncat(tmp,chr,1);
		 }
	 }
	 free(chr);
	 free(tmp);
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

void CONFIG_destroyList(config_param *first_param){
	config_param* aux = first_param;
	config_param* ant = first_param;
	while(aux != NULL){
		ant = aux;
		aux = aux->nextParam;
		free(ant);
	}
}
