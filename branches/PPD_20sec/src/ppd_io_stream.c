#include "ppd_io.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>

uint32_t bytes_perSector;

int32_t leer_sector(int32_t sector, char* buf)
{
	bytes_perSector = 512; //bytes_perSector tendria que ser variable global al PPD ya que es información importante
	uint32_t file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDONLY);
	/*
	 * Aviso al SO sobre el uso del archivo ?
	 */
	posix_fadvise(file_descriptor,bytes_perSector*sector,bytes_perSector,POSIX_FADV_SEQUENTIAL);

	if (lseek(file_descriptor,bytes_perSector*sector,SEEK_SET) == 0) //Posiciono el cursor dentro del archivo al inicio del sector buscado
	{
		if (read(file_descriptor,buf,bytes_perSector) == bytes_perSector) //Copio a buf el sector buscado
		{
			close(file_descriptor);
			return 0;
		}
		close(file_descriptor);;
		return READ_ERROR;
	}
	close(file_descriptor);
	return SEEK_ERROR;

}

int32_t escribir_sector(int32_t sector, char *buf)
{
	bytes_perSector = 512; //bytes_perSector tendria que ser variable global al PPD ya que es información importante
	uint32_t file_descriptor = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR);

	/*
	* Aviso al SO sobre el uso del archivo ?
	*/
	posix_fadvise(file_descriptor,bytes_perSector*sector,bytes_perSector,POSIX_FADV_SEQUENTIAL);

	if (lseek(file_descriptor,bytes_perSector*sector,SEEK_SET) == 0) //Posiciono el cursor dentro del archivo al inicio del sector buscado
	{
		if (write(file_descriptor,buf,bytes_perSector) == bytes_perSector) //Copio en el archivo el contenido de buf o sea el sector
		{
			close(file_descriptor);
			return 0;
		}
		close(file_descriptor);
		return WRITE_ERROR;
	}
	close(file_descriptor);
	return SEEK_ERROR;
}
