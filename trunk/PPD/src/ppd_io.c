#include "ppd_io.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>



int32_t leer_sector(int32_t file_descriptor, int32_t sector, char* buf)
{
	if (lseek(file_descriptor,512*sector,SEEK_SET) == 0)
	{
		if (read(file_descriptor,buf,512) == 512)
		{
			return 0;
		}
		return READ_ERROR;
	}
	return SEEK_ERROR;

}

int32_t escribir_sector(int32_t file_descriptor, int32_t sector, char *buf)
{
	if (lseek(file_descriptor,512*sector,SEEK_SET) == 0)
	{
		if (write(file_descriptor,buf,512) == 512)
		{
			return 0;
		}
		return WRITE_ERROR;
	}
	return SEEK_ERROR;
}
