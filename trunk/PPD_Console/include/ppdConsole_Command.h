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
#include "log.h"

uint32_t console_info(uint32_t);

uint32_t console_clean(queue_t,uint32_t);

uint32_t console_trace(queue_t,uint32_t,uint32_t);

void console_turnToCHS(uint32_t* sectorNum,t_CHS* CHS);

//void console_showTrace(char* msg);

#endif /* PPDCONSOLE_COMMAND_H_ */
