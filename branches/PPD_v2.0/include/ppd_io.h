/*
 * ppd_io.h
 *
 *  Created on: 07/09/2011
 *      Author: utn_so
 */

#ifndef PPD_IO_H_
#define PPD_IO_H_

#include <stdint.h>

#define SEEK_ERROR 1
#define WRITE_ERROR 2
#define READ_ERROR 3

int32_t read_sector(uint32_t file_descriptor,uint32_t sector,char *buf);
int32_t write_sector(uint32_t file_descriptor, uint32_t sector, char *buf);

#endif /* PPD_IO_H_ */
