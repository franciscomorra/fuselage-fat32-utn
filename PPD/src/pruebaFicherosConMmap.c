#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#define LEN 4096

int main() {

	int pagina,numeroSector,fd,byte;
	int numeroBytes,primerByte,i;

	fd = open("/home/utn_so/Desktop/FUSELAGE/fat32.disk",O_RDWR);
	if (fd == -1)
		perror(open);

	printf("ingrese numero de sector: ");
	scanf("%d",&numeroSector);
	printf("ingrese cuantos bytes seguidos desea leer: ");
	scanf("%d",&numeroBytes);
	printf("ingresee numero del primer byte que desea leer: ");
	scanf("%X",&primerByte);

	pagina = numeroSector / 8;
	char* pa = mmap(NULL, LEN, PROT_READ, MAP_PRIVATE, fd,(LEN*pagina));
	if (pa == MAP_FAILED)
		perror(mmap);

	byte = 0x200 * numeroSector + primerByte;
	pa = pa + byte;

	printf("contenido del byte %d:",byte);
	for(i = 0; i < numeroBytes; i++)
		printf(" %X",(unsigned char)*(pa+i));

	close(fd);
	return 0;
}
