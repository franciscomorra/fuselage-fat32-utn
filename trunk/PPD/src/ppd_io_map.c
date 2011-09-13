#include "ppd_io.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <math.h>
#include <sys/mman.h>

uint32_t page_size,sectors_perPage,bytes_perSector;

int32_t leer_sector(int32_t sector, char* buf)
{
	bytes_perSector = 512; //bytes_perSector tendria que ser variable global al PPD ya que es información importante
	page_size = getpagesize();
	sectors_perPage = page_size / bytes_perSector;
	uint32_t page = floor(sector/sectors_perPage);
	uint32_t file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDONLY);

	//Mapeo solo la pagina que contiene el sector buscado
	char* map = mmap(NULL,page_size , PROT_READ, MAP_PRIVATE, file_descriptor , page_size*page);

	/*
	 * Calculo el numero de sector (0-7) dentro de la pagina con la formula "sector-(sectors_perPage*page)" y
	 * copio los datos a buf
	 */
	memcpy(buf,map+(sector-(sectors_perPage*page))*bytes_perSector,bytes_perSector);

	munmap(map,page_size);
	close(file_descriptor);
	return 0;

}

int32_t escribir_sector(int32_t sector, char *buf)
{
	bytes_perSector = 512; //bytes_perSector tendria que ser variable global al PPD ya que es información importante
	page_size = getpagesize();
	uint32_t page = floor(sector/sectors_perPage);
	uint32_t file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR);

	//Mapeo solo la pagina que contiene el sector buscado
		char* map = mmap(NULL,page_size , PROT_WRITE, MAP_SHARED, file_descriptor ,page_size*page);

		/*
	 * Calculo el numero de sector (0-7) dentro de la pagina con la formula "sector-(sectors_perPage*page)"
	 * y copio en él los datos de buf
	 *  */
	memcpy(map+(sector-(sectors_perPage*page))*bytes_perSector,buf,bytes_perSector);

	munmap(map,page_size);
	close(file_descriptor);
	return 0;
}
