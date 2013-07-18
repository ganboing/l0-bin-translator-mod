/*
 * DecodeI0.h
 *
 *  Created on: Apr 23, 2013
 *      Author: PROGMAN
 */

#ifndef DECODEI0_H_
#define DECODEI0_H_

#include <stdint.h>
#include "DecodeStatus.h"


DECODE_STATUS TranslateI0ToNative(uint8_t** spc, uint8_t** tpc, uint8_t* nativelimit, uint8_t* i0limit, unsigned int is_write) ;

#endif /* DECODEI0_H_ */
