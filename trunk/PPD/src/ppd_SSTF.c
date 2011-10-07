
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ppd_SSTF.h"
#include "ppd_common.h"

extern uint32_t Head;
extern uint32_t Sector;
extern uint32_t TrackJumpTime;
extern uint32_t headPosition;
extern requestNode_t* first;


uint32_t SSTF_addRequest(uint32_t* sectorNum){

	 requestNode_t* new = SSTF_turnToCHS(sectorNum);
	 requestNode_t* CHSposition = SSTF_turnToCHS((uint32_t*)headPosition);

	 if(first == 0){
		 first = new;
		 return 0;
	 } else {
		requestNode_t* aux = first;
		if (SSTF_near(new,CHSposition,first)){
			new->next = first;
			first = new;
		} else {
			while (aux->next != 0){
				if(SSTF_near(new,aux,aux->next)){
					new->next = aux->next;
					aux->next = new;
					return 0;
				} else
					aux = aux->next;
			}
		}
		aux->next = new;
	}
	 return 0;
}

requestNode_t* SSTF_turnToCHS(uint32_t* sectorNum){

	requestNode_t* new = malloc(sizeof(requestNode_t));

	new->cylinder = (*sectorNum) / (Sector * Head);
	new->head = (*sectorNum % (Sector * Head)) / Sector;
	new->sector = (*sectorNum % (Sector * Head)) % Sector;

	return new;
}

uint32_t SSTF_near(requestNode_t* new, requestNode_t* aux,requestNode_t* auxSig){

	//se fija si la distancia entre  new y aux es menor que la de aux y auxSig
	// si es asi devuelve True

	uint32_t distTrackNA = abs(new->cylinder - aux->cylinder);
	uint32_t distTrackAS = abs(auxSig->cylinder - aux->cylinder);

	if (distTrackNA < distTrackAS)
		return 1;
	else
		if (distTrackNA == distTrackAS){
			if(SSTF_sectorDist(((aux->sector)+1)+(distTrackNA*TrackJumpTime),new->sector)
			<= SSTF_sectorDist(((aux->sector)+1)+(distTrackNA*TrackJumpTime),auxSig->sector))
				return 1;
	}
	return 0;
}

uint32_t SSTF_sectorDist(uint32_t fstSector, uint32_t lstSector){

	//devuelve la cantidad de sectores que tiene que recorrer para llegar de fst a lst.

	if (lstSector < fstSector)
		return (Sector - (fstSector - lstSector));
	else
		return (lstSector - fstSector);

	return 0;
}


