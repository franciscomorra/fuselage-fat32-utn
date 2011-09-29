#include "ppd_io.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>

uint32_t page_size,sectors_perPage,bytes_perSector;

int32_t read_sector(uint32_t file_descriptor,uint32_t sector, char* buf)
{
	bytes_perSector = 512; //bytes_perSector tendria que ser variable global al PPD ya que es información importante
	page_size = 4096;
	sectors_perPage = 8;
	uint32_t page = floor(sector / sectors_perPage);


	//Mapeo solo la pagina que contiene el sector buscado
	//ERROR
	char* map = mmap(NULL,page_size , PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, page_size*page);
	if(map==MAP_FAILED)
	    {
	        perror("mmap");
	        exit(1);
	    }

	/*
	 * Aviso al SO sobre el uso de la memoria ? Podria ser util cuando el sector buscado esta en una pagina que
	 * ya fue mapeada anteriormente
	 */
	posix_madvise(map+(sector-(sectors_perPage*page))*bytes_perSector,bytes_perSector,POSIX_MADV_WILLNEED);
	/*
	 * Calculo el numero de sector (0-7) dentro de la pagina con la formula "sector-(sectors_perPage*page)" y
	 * copio los datos a buf
	 */
	memcpy(buf,map+((sector-(sectors_perPage*page))*bytes_perSector),bytes_perSector);

	if (munmap(map, page_size) == -1) {
		perror("Error un-mmapping the file");
		/* Decide here whether to close(fd) and exit() or not. Depends... */
	    }
	return 0;

}

int32_t write_sector(uint32_t file_descriptor,uint32_t sector, char *buf)
{
	bytes_perSector = 512; //bytes_perSector tendria que ser variable global al PPD ya que es información importante
	page_size = getpagesize();
	uint32_t page = floor(sector / sectors_perPage);


	//Mapeo solo la pagina que contiene el sector buscado
	char* map = mmap(NULL,page_size , PROT_WRITE, MAP_SHARED, file_descriptor ,page_size*page);

	/*
	 * Aviso al SO sobre el uso de la memoria ?
	 */
	posix_madvise(map+(sector-(sectors_perPage*page))*bytes_perSector,bytes_perSector,POSIX_MADV_RANDOM);

	/*
	 * Calculo el numero de sector (0-7) dentro de la pagina con la formula "sector-(sectors_perPage*page)"
	 * y copio en él los datos de buf
	 *  */
	memcpy(map+(sector-(sectors_perPage*page))*bytes_perSector,buf,bytes_perSector);

	munmap(map,page_size);

	return 0;
}
