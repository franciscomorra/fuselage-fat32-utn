#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>


int main() {
	int fd = open("/home/utn_so/FUSELAGE/fat32.disk",O_RDWR);
	if (fd == -1)
			perror(open);
	int len = 512;
	char* pa = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd,0x0);
	if (*pa == MAP_FAILED)
		perror(mmap);
	printf("%X ",(unsigned char)*pa);
	pa = pa + 0x1;
	printf("%X",(unsigned char)*pa);
	close(fd);
	return 0;
}
