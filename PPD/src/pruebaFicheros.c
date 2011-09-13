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

	int pagina,numeroSector,fd;

	fd = open("/home/utn_so/Desktop/FUSELAGE/fat32.disk",O_RDWR);
	if (fd == -1)
		perror(open);

	printf("ingrese numero de pagina a mappear: ");
	pagina = ((int)getchar() - 48);
	printf("%d\n",pagina);
	printf("ingrese numero de sector: ");
	numeroSector = (int)getchar() - 48;

	char* pa = mmap(NULL, LEN, PROT_READ, MAP_PRIVATE, fd,(LEN*pagina));
	if (pa == MAP_FAILED)
		perror(mmap);

	pa = pa + (0x200 * numeroSector);
	printf("%X",(unsigned char)*pa);

	close(fd);
	return 0;
}
