
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ppd_requestList.h"

extern uint32_t Cylinder;
extern uint32_t Head;
extern uint32_t Sector;
extern requestNode_t* first;

uint32_t REQUEST_add(uint32_t* sectorNum){

	 requestNode_t* new = REQUEST_turnToCHS(sectorNum);
	 uint32_t lessDist = Cylinder*Head*Sector;

	 if(first == 0){
		 first = new;
		 return 0;
	 }
	 else {
		requestNode_t* aux = first;

		while (aux->next != 0){
			if(near(new,aux,aux->next)){
				new->next = aux->next;
				aux->next = new;
				return 0;
			}
			else
				aux = aux->next;
		}
		if (aux->next == 0)
			aux->next = new;
	 }
	return 0;
}

requestNode_t* REQUEST_turnToCHS(uint32_t* sectorNum){ //puedo castear directamente de char*  a uint32_t ??

	requestNode_t* new = malloc(sizeof(requestNode_t));

	new->cylinder = *sectorNum / (Sector * Head);
	new->head = (*sectorNum % (Sector * Head)) / Sector;
	new->sector = (*sectorNum % (Sector * Head)) % Sector;

	return new;
}

uint32_t near(requestNode_t* A, requestNode_t* B,requestNode_t* C){

/*	//se fija si la distancia entre  A y B es menor que la de A y C..
	// ademas se fija si la distancia entre A y C es menor que la de B y C
	// si es asi devuelve True

	if((A->cylinder - B->cylinder)<(C->cylinder - B->cylinder)){

	} */
return 0;
}
