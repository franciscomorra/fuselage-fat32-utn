/*
 * praid_console.h
 *
 *  Created on: 09/10/2011
 *      Author: utn_so
 */

#ifndef PRAID_CONSOLE_H_
#define PRAID_CONSOLE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

uint32_t print_Console (char *message, uint32_t number, uint32_t priority, bool display_int);
uint32_t PRAID_WRITE_LOG(char* msg);

/*
uint32_t print_Console (char *);
uint32_t print_ConsoleInt (uint32_t );
*/
#endif /* PRAID_CONSOLE_H_ */