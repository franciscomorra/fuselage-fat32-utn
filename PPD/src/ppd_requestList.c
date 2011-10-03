
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ppd_requestList.h"

extern uint32_t Head;
extern uint32_t Sector;
extern requestNode_t* first;

uint32_t REQUEST_add(uint32_t* sectorNum){

	 requestNode_t* new = REQUEST_turnToCHS(sectorNum);

	 if(first == 0){
		 first = new;
		 return 0;
	 }
	 else {
		requestNode_t* aux = first;

		while (aux->next != 0){
			if(REQUEST_near(new,aux,aux->next)){
				new->next = aux->next;
				aux->next = new;
				return 0;
			}
			else
				aux = aux->next;
		}
		aux->next = new;
		}
	 return 0;
}

requestNode_t* REQUEST_turnToCHS(uint32_t* sectorNum){

	requestNode_t* new = malloc(sizeof(requestNode_t));

	new->cylinder = *sectorNum / (Sector * Head);
	new->head = (*sectorNum % (Sector * Head)) / Sector;
	new->sector = (*sectorNum % (Sector * Head)) % Sector;

	return new;
}

uint32_t REQUEST_near(requestNode_t* new, requestNode_t* aux,requestNode_t* auxSig){

	//se fija si la distancia entre  new y aux es menor que la de aux y auxSig
	// si es asi devuelve True

	uint32_t distSectorNA = abs(new->cylinder - aux->cylinder);
	uint32_t distSectorAS = abs(auxSig->cylinder - aux->cylinder);

/*	switch (distSectorNA < distSectorAS) {

		case(distSectorNA < distSectorAS):
			return 1;

		case(distSectorNA == distSectorAS):{
			if(REQUEST_sectorDist((aux->sector)+1,new->sector) <= REQUEST_sectorDist((aux->sector)+1,auxSig->sector))
				return 1;
		} */

	if (distSectorNA < distSectorAS)
		return 1;
	else
		if (distSectorNA == distSectorAS){
			if(REQUEST_sectorDist((aux->sector)+1,new->sector) <= REQUEST_sectorDist((aux->sector)+1,auxSig->sector))
				return 1;
	}
	return 0;
}

uint32_t REQUEST_sectorDist(uint32_t fstSector, uint32_t lstSector){

	//devuelve la cantidad de sectores que tiene que recorrer para llegar de fst a lst.

	if (lstSector < fstSector)
		exit (Sector - (fstSector - lstSector));
	else
		exit (lstSector - fstSector);

	return 0;
}
