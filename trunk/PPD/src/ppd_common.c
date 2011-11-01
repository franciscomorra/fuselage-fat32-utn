
#include <ppd_common.h>
#include <string.h>
#include <stdlib.h>

void COMMON_turnToCHS(uint32_t sectorNum,requestNode_t* new){

	new->cylinder = (sectorNum) / (Sector * Head);
	new->head = (sectorNum % (Sector * Head)) / Sector;
	new->sector = (sectorNum % (Sector * Head)) % Sector;
}