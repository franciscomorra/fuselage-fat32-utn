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

	int pagina,numeroSector,fd,byte,numeroByte;

	fd = open("/home/utn_so/Desktop/FUSELAGE/fat32.disk",O_RDWR);
	if (fd == -1)
		perror(open);

	printf("ingrese numero de pagina a mappear: ");
	scanf("%d",&pagina);
	printf("ingrese numero de sector: ");
	scanf("%d",&numeroSector);
	printf("ingresee numero de byte: ");
	scanf("%d",&numeroByte);

	char* pa = mmap(NULL, LEN, PROT_READ, MAP_PRIVATE, fd,(LEN*pagina));
	if (pa == MAP_FAILED)
		perror(mmap);
	byte = 0x200 * numeroSector + numeroByte;
	pa = pa + byte;
	printf("contenido del byte %d: %X",byte,(unsigned char)*pa);

	close(fd);
	return 0;
}