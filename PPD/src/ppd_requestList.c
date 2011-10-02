
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ppd_requestList.h"

extern uint32_t Cylinder;
extern uint32_t Head;
extern uint32_t Sector;

uint32_t REQUEST_add(char* sectorNum){

	// requestNode_t* new = REQUEST_getCHS(sectorNum);


	return 0;
}


/*requestNode_t* REQUEST_getCHS(char* sectorNum){

	requestNode_t* new = malloc(sizeof(requestNode_t));

	new->cylinder = sectorNum / (Sector * Head);
	new->head = (sectorNum % (Sector * Head)) / Sector;
	new->sector = (sectorNum % (Sector * Head)) % Sector;

	return new;
} */
