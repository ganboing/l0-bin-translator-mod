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
#include "I0Types.h"

DECODE_STATUS TranslateI0ToNative(uint8_t** spc, uint8_t* tpc, uint64_t* nativelimit, uint8_t* i0limit,unsigned int is_write) ;
DECODE_STATUS TranslateINT(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write);
DECODE_STATUS TranslateBZNZ(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write);
DECODE_STATUS TranslateBCMP(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write);
DECODE_STATUS TranslateNOP(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write);
DECODE_STATUS TranslateEXIT(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write);
DECODE_STATUS TranslateALU(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write);
DECODE_STATUS TranslateSPAWN(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write);
DECODE_STATUS TranslateSCMP(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write);
DECODE_STATUS TranslateSHIFT(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write);
DECODE_STATUS TranslateCONV(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write);
DECODE_STATUS TranslateBJ(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write);
DECODE_STATUS TranslateBIJ(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write);

#endif /* DECODEI0_H_ */
