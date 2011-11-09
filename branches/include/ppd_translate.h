/*
 * ppd_translate.h
 *
 *  Created on: Oct 24, 2011
 *      Author: utn_so
 */

#ifndef PPD_TRANSLATE_H_
#define PPD_TRANSLATE_H_

#include <stdint.h>
#include "ppd_common.h"

request_t* TRANSLATE_fromCharToRequest(char* msg,uint32_t sockFD);

char* TRANSLATE_fromRequestToChar(request_t* request);


#endif /* PPD_TRANSLATE_H_ */
