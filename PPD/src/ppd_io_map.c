#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>

#include "ppd_io.h"
#include "log.h"

#define MAX_WRITINGS_PERSYNC 5000

extern uint32_t bytes_perSector;
extern uint32_t ReadTime;
extern uint32_t WriteTime;
extern uint32_t Sector;
extern uint32_t Cylinder;
extern t_log* Log;
char* Map;
uint32_t  page_size = 4096;
uint32_t sectors_perPage = 8;
uint32_t writings;

uint32_t IO_openDisk(char* diskFilePath){
	uint32_t file_descriptor;
	uint32_t diskSize = Cylinder * Sector * bytes_perSector;
	writings = 0;

	if((file_descriptor = open(diskFilePath,O_RDWR)) == -1){
		log_error(Log,"Principal","Error al asociar el disco con el proceso");
		exit(1);
	}
	Map = mmap(NULL,diskSize, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor,0);
	if(Map==MAP_FAILED){
		log_error(Log,"Principal","Error al mappear el disco a memoria");
		exit(1);
	}
	posix_madvise(Map,diskSize,POSIX_MADV_RANDOM);
	return file_descriptor;
}

void IO_readDisk(uint32_t sector,char* buf){
//	uint32_t page = floor(sector / sectors_perPage);

	memcpy(buf,Map+(sector*bytes_perSector),bytes_perSector);

	sleep(ReadTime/1000);
}

void IO_writeDisk(uint32_t sector,char* buf){
//	uint32_t page = floor(sector / sectors_perPage);
	uint32_t diskSize = Cylinder * Sector * bytes_perSector;

	memcpy(Map+(sector*bytes_perSector),buf,bytes_perSector);
	writings++;
	if(writings >= MAX_WRITINGS_PERSYNC){
		msync(Map,diskSize,MS_ASYNC);
		writings = 0;
	}
	sleep(WriteTime/1000);
}

void IO_closeDisk(uint32_t file_descriptor){
	uint32_t diskSize = Cylinder * Sector * bytes_perSector;

	if (munmap(Map, diskSize) == -1) {
		log_error(Log,"Principal","Error al des-mappear el disco a memoria");
		exit(1);
	}
	if(close(file_descriptor)<0){
		log_error(Log,"Principal","Error al cerrar el disco");
		exit(1);
	}
}
/*
int32_t read_sector(uint32_t file_descriptor,uint32_t sector, char* buf)
{
	uint32_t page = floor(sector / sectors_perPage);


	char* map = mmap(NULL,page_size , PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, page_size*page);
	if(map==MAP_FAILED)
	    {
	        perror("mmap");
	        exit(1);
	    }

	posix_madvise(map+(sector-(sectors_perPage*page))*bytes_perSector,bytes_perSector,POSIX_MADV_WILLNEED);

	memcpy(buf,map+((sector-(sectors_perPage*page))*bytes_perSector),bytes_perSector);

	if (munmap(map, page_size) == -1) {
		perror("Error un-mmapping the file");
	    }

	sleep(ReadTime/1000);

	return 0;

}
*/
/*
int32_t write_sector(uint32_t file_descriptor,uint32_t sector, char *buf)
{
	page_size = getpagesize();
	uint32_t page = floor(sector / sectors_perPage);

	char* map = mmap(NULL,page_size , PROT_WRITE, MAP_SHARED, file_descriptor ,page_size*page);


	posix_madvise(map+(sector-(sectors_perPage*page))*bytes_perSector,bytes_perSector,POSIX_MADV_RANDOM);


	memcpy(map+(sector-(sectors_perPage*page))*bytes_perSector,buf,bytes_perSector);

	munmap(map,page_size);

	sleep(WriteTime/1000);

	return 0;
}

*/
