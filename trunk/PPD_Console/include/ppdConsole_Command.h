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

uint32_t console_info(uint32_t);

uint32_t console_clean(queue_t,uint32_t);

uint32_t console_trace(queue_t,uint32_t,uint32_t);

#endif /* PPDCONSOLE_COMMAND_H_ */
