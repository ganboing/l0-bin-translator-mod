/*
 * DecodeI0.h
 *
 *  Created on: Apr 23, 2013
 *      Author: PROGMAN
 */

#ifndef DECODEI0_H_
#define DECODEI0_H_

#include <stdint.h>

typedef union _OPRVAL {
	uint8_t v8;
	uint16_t v16;
	uint32_t v32;
	uint64_t v64;
}OPRVAL;

typedef struct _I0OPR {
	OPRVAL val;
	uint32_t addrm;
	uint32_t disp32;
}I0OPR;

typedef struct _I0INSTR {
	uint64_t addr;
	uint32_t addr_size_mode;
	uint32_t opcode;
	uint32_t attr;
	uint32_t attr2;
	uint32_t option;
	uint32_t ra;
	I0OPR opr[5];
}I0INSTR;

typedef union _IMM_TYPE {
	uint8_t v8;
	uint16_t v16;
	uint32_t v32;
	uint64_t v64;
}IMM_TYPE;

typedef struct _x64_OPR{
	uint8_t type;
#define x64_OPR_TYPE_REG		1
#define x64_OPR_TYPE_M			2
#define x64_OPR_I				0
	uint8_t reg;
	uint32_t off32;
	IMM_TYPE imm;
}x64_OPR;

typedef union _OFFSET_TYPE {
	uint8_t v8;
	uint32_t v32;
}OFFSET_TYPE;


typedef struct _x64INSTR{
	uint8_t LegacyPrefix_Len;
	uint8_t opcode_len;
	uint8_t ModRM_SIB_len;
	uint8_t disp_len;
	uint8_t imm_len;
	uint8_t LegacyPrefix[1];
	uint8_t REX;
	uint8_t opcode[2];
	uint8_t ModRM_SIB[2];
	OFFSET_TYPE disp;
	IMM_TYPE imm;

}x64INSTR;

#define MAX_OPR_NUM 5

typedef struct _instr_t {
    // unsigned int l1; // obsolete
    // unsigned int l2; // obsolete
    uint64_t addr;
    uint32_t addr_size_mode;
    uint32_t opcode;
    uint32_t option;
    uint32_t mattr1;
    uint32_t mattr2;
    uint32_t addrm1;
    uint32_t addrm2;
    uint32_t addrm3;
    uint32_t addrm4;
    uint32_t attr;
    uint32_t ra;
    uint64_t opr1;
    uint32_t disp1;
    uint64_t opr2;
    uint32_t disp2;
    uint64_t opr3;
    uint32_t disp3;
    uint64_t opr4;
    uint32_t disp4;
    uint32_t addrms[MAX_OPR_NUM];
    uint64_t oprs[MAX_OPR_NUM];
    uint32_t disps[MAX_OPR_NUM];
} instr_t;

#undef MAX_OPR_NUM


typedef struct _DECODE_STATUS{
	unsigned long status;
	unsigned long detail;
	unsigned long detail2;
}DECODE_STATUS;

DECODE_STATUS TranslateI0ToNative(uint8_t** spc, uint8_t** tpc, uint8_t* nativelimit, uint8_t* i0limit, unsigned int is_write) ;

#endif /* DECODEI0_H_ */
