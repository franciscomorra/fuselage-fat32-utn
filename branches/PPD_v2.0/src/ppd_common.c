
#include "ppd_common.h"
#include <string.h>
#include <stdlib.h>

extern uint32_t Sector;
extern uint32_t Head;

CHS_t* COMMON_turnToCHS(uint32_t sectorNum){
	CHS_t* CHS = malloc(sizeof(CHS_t));

	CHS->cylinder = (sectorNum) / (Sector * Head);
	CHS->head = (sectorNum % (Sector * Head)) / Sector;
	CHS->sector = (sectorNum % (Sector * Head)) % Sector;
	return CHS;
}
