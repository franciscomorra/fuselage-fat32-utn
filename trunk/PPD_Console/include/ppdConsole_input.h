/*
 * ppdConsole_input.h
 *
 *  Created on: Oct 6, 2011
 *      Author: utn_so
 */

#ifndef PPDCONSOLE_INPUT_H_
#define PPDCONSOLE_INPUT_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


uint32_t getCommand(char* input, char* command);

uint32_t getCleanSectors(char* input,uint32_t* fstSector,uint32_t* lstSector);

uint32_t getTraceSectors(char* input,uint32_t* traceSectors);

#endif /* PPDCONSOLE_INPUT_H_ */
