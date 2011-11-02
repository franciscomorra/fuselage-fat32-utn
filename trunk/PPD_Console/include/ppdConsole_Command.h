/*
 * ppdConsole_Command.h
 *
 *  Created on: Oct 6, 2011
 *      Author: utn_so
 */

#ifndef PPDCONSOLE_COMMAND_H_
#define PPDCONSOLE_COMMAND_H_

#include <stdio.h>
#include <stdint.h>

typedef struct CHS_t {
	uint32_t cylinder;
	uint32_t head;
	uint32_t sector;
}__attribute__((__packed__)) CHS_t;

uint32_t console_info(uint32_t);

uint32_t console_clean(queue_t,uint32_t);

uint32_t console_trace(queue_t,uint32_t,uint32_t);

void console_turnToCHS(uint32_t* sectorNum,CHS_t* CHS);

void console_showTrace(char* msg);

#endif /* PPDCONSOLE_COMMAND_H_ */
