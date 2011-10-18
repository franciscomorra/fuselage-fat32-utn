
#include <ppd_common.h>

requestNode_t* COMMON_turnToCHS(uint32_t* sectorNum){

	requestNode_t* new = malloc(sizeof(requestNode_t));

	new->cylinder = (*sectorNum) / (Sector * Head);
	new->head = (*sectorNum % (Sector * Head)) / Sector;
	new->sector = (*sectorNum % (Sector * Head)) % Sector;

	return new;
}
