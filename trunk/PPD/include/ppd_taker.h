/*
 * ppd_taker.h
 *
 *  Created on: 07/10/2011
 *      Author: utn_so
 */

#ifndef PPD_TAKER_H_
#define PPD_TAKER_H_

// definida en el ppd_main por un tema de threads
uint32_t TAKER_main();

// saca el request que este asociado al first de la lista y lo envia por sockets
nipcMsg_t TAKER_getRequest(requestNode_t* first);

// cambia de CHS a numero de sector para poder enviarlo en el payload del nipcMsg_t
uint32_t TAKER_turnToSectorNum(requestNode_t* CHSnode);

#endif /* PPD_TAKER_H_ */

