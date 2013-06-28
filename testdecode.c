#include <stdint.h>
#include "I0Symbol.h"
#include "ASM_MACROS.h"


#define GET_INST_OPR_I64(opr,t) \
	do{ \
		(opr).val.v64 = *((uint64_t*)(t)); \
		(t) = (typeof(t))(((uint64_t*)(t))+1); \
	}while(0)

#define GET_INST_OPR_I8(opr,t) \
	do{ \
		(opr).val.v8 = *((uint8_t*)(t)); \
		(t) = (typeof(t))(((uint8_t*)(t))+1); \
	}while(0)


#define DECODE_OPR_I(opr, spc, attr) \
	do { \
		switch(attr) \
		{ \
		case ATTR_SB:\
			(opr).val.v8 = (*((uint8_t*)(spc)));\
			(spc) = (typeof(spc))(((uint8_t*)(spc))+1);\
			break;\
		case ATTR_SE:\
			(opr).val.v64 = (*((uint64_t*)(spc)));\
			(spc) = (typeof(spc))(((uint64_t*)(spc))+1);\
			break;\
		case ATTR_SF:\
			(opr).val.v32 = (*((uint32_t*)(spc)));\
			(spc) = (typeof(spc))(((uint32_t*)(spc))+1);\
			break;\
		case ATTR_UB:\
			(opr).val.v8 = (*((uint8_t*)(spc)));\
			(spc) = (typeof(spc))(((uint8_t*)(spc))+1);\
			break;\
		case ATTR_UE:\
			(opr).val.v64 = (*((uint64_t*)(spc)));\
			(spc) = (typeof(spc))(((uint64_t*)(spc))+1);\
			break;\
		case ATTR_UF:\
			(opr).val.v32 = (*((uint32_t*)(spc)));\
			(spc) = (typeof(spc))(((uint32_t*)(spc))+1);\
			break;\
		case ATTR_FS:\
			(opr).val.v32 = (*((uint32_t*)(spc)));\
			(spc) = (typeof(spc))(((uint32_t*)(spc))+1);\
			break;\
		case ATTR_FD:\
			(opr).val.v64 = (*((uint64_t*)(spc)));\
			(spc) = (typeof(spc))(((uint64_t*)(spc))+1);\
			break;\
		} \
	}while(0)

#define DECODE_OPR_M(opr, spc, addrm)\
	do{\
		switch(addrm)\
		{\
		case ADDRM_DISPLACEMENT:\
			(opr).disp32 = (*((uint32_t*)(spc)));\
			(spc) = (typeof(spc))(((uint32_t*)(spc))+1);\
		default:\
			(opr).val.v64 = (*((uint64_t*)(spc)));\
			(spc) = (typeof(spc))(((uint64_t*)(spc))+1);\
			break;\
		}\
	}while(0)

#define DECODE_OPR_D(opr, spc, addrm, attr)\
	do{\
		switch(addrm)\
		{\
		case ADDRM_IMMEDIATE:\
			DECODE_OPR_I(opr, spc, attr);\
			break;\
		default:\
			DECODE_OPR_M(opr, spc, addrm);\
			break;\
		}\
	}while(0)

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

typedef struct _DECODE_STATUS{
	unsigned long status;
	unsigned long detail;
}DECODE_STATUS;

#define RETURN_DECODE_STATUS(status, detail) \
	do{ DECODE_STATUS result= {status, detail}; return result;}while(0);

static uint8_t I0OprISize[0x10]=
{
	0x08, 0x40, 0x00, 0x20,
	0x08, 0x40, 0x00, 0x20,
	0x20, 0x40, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
};

static uint8_t I0OprMSize[0x08]=
{
	0x00, 0x08, 0x08, 0x0c,
	0x00, 0x00, 0x00, 0x00
};

static uint8_t I0OprDSize[0x10][0x08]=
{
	{0x08, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x40, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x20, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x08, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x40, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x20, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x20, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x40, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

DECODE_STATUS TranslateNOP_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateADD_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateSUB_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateMUL_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateDIV_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBJ_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBL_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBLE_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBE_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBNE_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBZ_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBNZ_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBSL_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBIJ_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateAND_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateOR_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateXOR_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateCONV_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateINT_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateSPAWN_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateSHIFT_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateSCMP_NW(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateEXIT_NW(I0INSTR*, uint8_t**, uint8_t*);

DECODE_STATUS TranslateNOP_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateADD_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateSUB_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateMUL_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateDIV_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBJ_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBL_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBLE_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBE_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBNE_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBZ_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBNZ_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBSL_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateBIJ_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateAND_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateOR_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateXOR_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateCONV_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateINT_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateSPAWN_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateSHIFT_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateSCMP_WR(I0INSTR*, uint8_t**, uint8_t*);
DECODE_STATUS TranslateEXIT_WR(I0INSTR*, uint8_t**, uint8_t*);

inline DECODE_STATUS TranslateEXIT_WR(I0INSTR* instr, uint8_t** tpc, uint8_t* nativelimit)
{
	//movl $option, %edi
	//movl $back_runner_wrapper, %eax
	//jmp *%rax
	unsigned long nativelen = (((unsigned long)nativelimit)-((unsigned long)(*tpc)));
	if(nativelen<0x0c)
	{
		RETURN_DECODE_STATUS(NATIVE_CODE_SEGMENT_LIMIT, 0);
	}
	else
	{
		(*((*tpc)++)) = 0xbf;
		(*((uint32_t*)(*tpc))) = instr->option;
		(*tpc) += 4;
		(*((*tpc)++)) = 0xb8;
		(*((uint32_t*)(*tpc))) = ((uint32_t)sys_back_runner_wrapper);
		(*tpc) += 4;
		(*((uint16_t*)(*tpc))) = 0xe0ff;
		(*tpc) += 2;
		RETURN_DECODE_STATUS(I0_DECODE_SUCCESSFUL, 0);
	}
}

inline DECODE_STATUS TranslateEXIT_NW(I0INSTR* instr, uint8_t** tpc, uint8_t* nativelimit)
{
	unsigned long nativelen = (((unsigned long)nativelimit)-((unsigned long)(*tpc)));
	if(nativelen<0x0c)
	{
		RETURN_DECODE_STATUS(NATIVE_CODE_SEGMENT_LIMIT, 0);
	}
	else
	{
		(*tpc) += 0x0c;
		RETURN_DECODE_STATUS(I0_DECODE_SUCCESSFUL, 0);
	}

}

DECODE_STATUS TranslateI0ToNative(uint8_t** spc, uint8_t** tpc, uint8_t* nativelimit, uint8_t* i0limit, unsigned int is_write) {
	I0INSTR instr;
	unsigned int op;
	unsigned long i0instrlen = 0;
	unsigned long i0len = (((unsigned long) i0limit) - ((unsigned long) (*spc)));
	if (i0len >= ((BIT_LEN_ADDR_SIZE_MODE + BIT_LEN_OPCODE+8-1) / 8)) {
		LOAD_OP_WORD0(op, (*spc));
	} else {
		RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
	}
	instr.addr = (uint64_t) (*spc);
	GET_INST_FIELD_ZO(instr.opcode, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);

	if (is_write) {
		switch (instr.opcode) {
		case OP_NOP:
			i0instrlen = BYTE_OP_NOP;
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateNOP_WR(&instr, tpc, nativelimit);
			break;
		case OP_ADD:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += BYTE_OP_ALU;
			DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
			DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
			DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
			return TranslateADD_WR(&instr, tpc, nativelimit);
			break;
		case OP_SUB:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += BYTE_OP_ALU;
			DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
			DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
			DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
			return TranslateSUB_WR(&instr, tpc, nativelimit);
			break;
		case OP_MUL:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += BYTE_OP_ALU;
			DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
			DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
			DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
			return TranslateMUL_WR(&instr, tpc, nativelimit);
			break;
		case OP_DIV:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += BYTE_OP_ALU;
			DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
			DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
			DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
			return TranslateDIV_WR(&instr, tpc, nativelimit);
			break;
		case OP_B:
			GET_INST_FIELD_ZO(instr.option, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
			switch (instr.option) {
			case OPT_B_J:
				GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_ADDR_SIZE_MODE + BIT_LEN_OPCODE + BIT_LEN_OPT_B + BIT_LEN_RA);
				i0instrlen = BYTE_OP_BJ;
				i0instrlen += 8;
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += (i0instrlen - 8);
				GET_INST_OPR_I64((instr.opr[0]), (*spc));
				return TranslateBJ_WR(&instr, tpc, nativelimit);
				break;
			case OPT_B_L:
				i0instrlen = BYTE_OP_BCMP;
				if (i0len >= i0instrlen) {
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += BYTE_OP_BCMP;
				DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
				DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBL_WR(&instr, tpc, nativelimit);
				break;
			case OPT_B_LE:
				i0instrlen = BYTE_OP_BCMP;
				if (i0len >= i0instrlen) {
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += BYTE_OP_BCMP;
				DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
				DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBLE_WR(&instr, tpc, nativelimit);
				break;
			case OPT_B_E:
				i0instrlen = BYTE_OP_BCMP;
				if (i0len >= i0instrlen) {
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += BYTE_OP_BCMP;
				DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
				DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBE_WR(&instr, tpc, nativelimit);
				break;
			case OPT_B_NE:
				i0instrlen = BYTE_OP_BCMP;
				if (i0len >= i0instrlen) {
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += BYTE_OP_BCMP;
				DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
				DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBNE_WR(&instr, tpc, nativelimit);
				break;
			case OPT_B_Z:
				i0instrlen = BYTE_OP_BZNZ;
				if (i0len >= i0instrlen) {
					LOAD_OP_BYTE2(op, (*spc));
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += (I0OprDSize[instr.attr][instr.opr[0].addrm] + 8U);
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += BYTE_OP_BZNZ;
				DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBZ_WR(&instr, tpc, nativelimit);
				break;
			case OPT_B_NZ:
				i0instrlen = BYTE_OP_BZNZ;
				if (i0len >= i0instrlen) {
					LOAD_OP_BYTE2(op, (*spc));
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += (I0OprDSize[instr.attr][instr.opr[0].addrm] + 8U);
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += BYTE_OP_BZNZ;
				DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBNZ_WR(&instr, tpc, nativelimit);
				break;
			case OPT_B_SL:
				i0instrlen = BYTE_OP_BCMP;
				if (i0len >= i0instrlen) {
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += BYTE_OP_BCMP;
				DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
				DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBSL_WR(&instr, tpc, nativelimit);
				break;
			case OPT_B_IJ:
				i0instrlen = BYTE_OP_BIJ;
				if (i0len >= i0instrlen) {
					LOAD_OP_BYTE2(op, (*spc));
					GET_INST_FIELD_NW((instr.opr[0]).addrm, op, BIT_LEN_ADDR_SIZE_MODE + BIT_LEN_OPCODE + BIT_LEN_OPT_B + BIT_LEN_ADDRM);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprMSize[instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprMSize[instr.opr[0].addrm];
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += BYTE_OP_BIJ;
				DECODE_OPR_M(instr.opr[0], *spc, instr.opr[0].addrm);
				return TranslateBIJ_WR(&instr, tpc, nativelimit);
				break;
			default:
				RETURN_DECODE_STATUS(OPCODE_B_UNDEFINED, 0)
				;
				break;
			}
			break;
		case OP_AND:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += BYTE_OP_ALU;
			DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
			DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
			DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
			return TranslateAND_WR(&instr, tpc, nativelimit);
			break;
		case OP_OR:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += BYTE_OP_ALU;
			DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
			DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
			DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
			return TranslateOR_WR(&instr, tpc, nativelimit);
			break;
		case OP_XOR:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += BYTE_OP_ALU;
			DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
			DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
			DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
			return TranslateXOR_WR(&instr, tpc, nativelimit);
			break;
		case OP_CONV:
			i0instrlen = BYTE_OP_CONV;
			if (i0len >= i0instrlen) {
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH(instr.attr2, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprMSize[instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[1].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += BYTE_OP_CONV;
			DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
			DECODE_OPR_M(instr.opr[1], *spc, instr.opr[1].addrm);
			return TranslateCONV_WR(&instr, tpc, nativelimit);
			break;
		case OP_INT:
			i0instrlen = BYTE_OP_INT;
			i0instrlen += 8;
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += BYTE_OP_INT;
			GET_INST_OPR_I8((instr.opr[0]), (*spc));
			return TranslateINT_WR(&instr, tpc, nativelimit);
			break;
		case OP_SPAWN:
			i0instrlen = BYTE_OP_SPAWN;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[3]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprMSize[instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[0].addrm];
			if (!I0OprMSize[instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (!I0OprMSize[instr.opr[3].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[3].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += BYTE_OP_SPAWN;
			DECODE_OPR_M(instr.opr[0], *spc, instr.opr[0].addrm);
			DECODE_OPR_M(instr.opr[1], *spc, instr.opr[1].addrm);
			DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
			DECODE_OPR_M(instr.opr[3], *spc, instr.opr[3].addrm);
			return TranslateSPAWN_WR(&instr, tpc, nativelimit);
			break;
		case OP_SHIFT:
			i0instrlen = BYTE_OP_SHIFT;
			if (i0len >= i0instrlen) {
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
				GET_INST_FIELD_SH(instr.option, op, BIT_LEN_OPT_SHIFT);
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[MATTR_UB][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[MATTR_UB][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += BYTE_OP_SHIFT;
			DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
			DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, MATTR_UB);
			DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
			return TranslateSHIFT_WR(&instr, tpc, nativelimit);
			break;
		case OP_SCMP:
			i0instrlen = BYTE_OP_SCMP;
			if (i0len >= i0instrlen) {
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[3]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[4]).addrm, op, BIT_LEN_ADDRM);
				instr.attr = MATTR_UE;
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprMSize[instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[3].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[3].addrm];
			if (!I0OprMSize[instr.opr[4].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[4].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += BYTE_OP_SCMP;
			DECODE_OPR_M(instr.opr[0], *spc, instr.opr[0].addrm);
			DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
			DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
			DECODE_OPR_D(instr.opr[3], *spc, instr.opr[3].addrm, instr.attr);
			DECODE_OPR_M(instr.opr[4], *spc, instr.opr[4].addrm);
			return TranslateSCMP_WR(&instr, tpc, nativelimit);
			break;
		case OP_EXIT:
			i0instrlen = BYTE_OP_EXIT;
			GET_INST_FIELD_NW(instr.option, op, BIT_LEN_ADDR_SIZE_MODE + BIT_LEN_OPCODE + BIT_LEN_OPT_EXIT);
			(*spc) += i0instrlen;
			return TranslateEXIT_WR(&instr, tpc, nativelimit);
			break;
		default:
			RETURN_DECODE_STATUS(OPCODE_UNDEFINED, 0)
			;
			break;
		}
	} else {
		switch (instr.opcode) {
		case OP_NOP:
			i0instrlen = BYTE_OP_NOP;
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateNOP_NW(&instr, tpc, nativelimit);
			break;
		case OP_ADD:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateADD_NW(&instr, tpc, nativelimit);
			break;
		case OP_SUB:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateSUB_NW(&instr, tpc, nativelimit);
			break;
		case OP_MUL:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateMUL_NW(&instr, tpc, nativelimit);
			break;
		case OP_DIV:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateDIV_NW(&instr, tpc, nativelimit);
			break;
		case OP_B:
			GET_INST_FIELD_ZO(instr.option, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
			switch (instr.option) {
			case OPT_B_J:
				GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_RA);
				i0instrlen = BYTE_OP_BJ;
				i0instrlen += 8;
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += (i0instrlen - 8);
				GET_INST_OPR_I64((instr.opr[0]), (*spc));
				return TranslateBJ_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_L:
				i0instrlen = BYTE_OP_BCMP;
				if (i0len >= i0instrlen) {
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += (i0instrlen - 8);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBL_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_LE:
				i0instrlen = BYTE_OP_BCMP;
				if (i0len >= i0instrlen) {
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += (i0instrlen - 8);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBLE_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_E:
				i0instrlen = BYTE_OP_BCMP;
				if (i0len >= i0instrlen) {
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += (i0instrlen - 8);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBE_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_NE:
				i0instrlen = BYTE_OP_BCMP;
				if (i0len >= i0instrlen) {
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += (i0instrlen - 8);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBNE_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_Z:
				i0instrlen = BYTE_OP_BZNZ;
				if (i0len >= i0instrlen) {
					LOAD_OP_BYTE2(op, (*spc));
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += (I0OprDSize[instr.attr][instr.opr[0].addrm] + 8U);
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += (i0instrlen - 8);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBZ_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_NZ:
				i0instrlen = BYTE_OP_BZNZ;
				if (i0len >= i0instrlen) {
					LOAD_OP_BYTE2(op, (*spc));
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += (I0OprDSize[instr.attr][instr.opr[0].addrm] + 8U);
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += (i0instrlen - 8);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBNZ_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_SL:
				i0instrlen = BYTE_OP_BCMP;
				if (i0len >= i0instrlen) {
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += (i0instrlen - 8);
				GET_INST_OPR_I64((instr.opr[2]), (*spc));
				return TranslateBSL_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_IJ:
				i0instrlen = BYTE_OP_BIJ;
				if (i0len >= i0instrlen) {
					LOAD_OP_BYTE2(op, (*spc));
					GET_INST_FIELD_NW((instr.opr[0]).addrm, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_ADDRM);
				} else {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				if (!I0OprMSize[instr.opr[0].addrm]) {
					RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
				}
				i0instrlen += I0OprMSize[instr.opr[0].addrm];
				if (i0instrlen > i0len) {
					RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
				}
				(*spc) += i0instrlen;
				return TranslateBIJ_NW(&instr, tpc, nativelimit);
				break;
			default:
				RETURN_DECODE_STATUS(OPCODE_B_UNDEFINED, 0)
				;
				break;
			}
			break;
		case OP_AND:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateAND_NW(&instr, tpc, nativelimit);
			break;
		case OP_OR:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateOR_NW(&instr, tpc, nativelimit);
			break;
		case OP_XOR:
			i0instrlen = BYTE_OP_ALU;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateXOR_NW(&instr, tpc, nativelimit);
			break;
		case OP_CONV:
			i0instrlen = BYTE_OP_CONV;
			if (i0len >= i0instrlen) {
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH(instr.attr2, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprMSize[instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[1].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateCONV_NW(&instr, tpc, nativelimit);
			break;
		case OP_INT:
			i0instrlen = BYTE_OP_INT;
			i0instrlen += 8;
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateINT_NW(&instr, tpc, nativelimit);
			break;
		case OP_SPAWN:
			i0instrlen = BYTE_OP_SPAWN;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[3]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprMSize[instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[0].addrm];
			if (!I0OprMSize[instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (!I0OprMSize[instr.opr[3].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[3].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateSPAWN_NW(&instr, tpc, nativelimit);
			break;
		case OP_SHIFT:
			i0instrlen = BYTE_OP_SHIFT;
			if (i0len >= i0instrlen) {
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
				GET_INST_FIELD_SH(instr.option, op, BIT_LEN_OPT_SHIFT);
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[MATTR_UB][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[MATTR_UB][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateSHIFT_NW(&instr, tpc, nativelimit);
			break;
		case OP_SCMP:
			i0instrlen = BYTE_OP_SCMP;
			if (i0len >= i0instrlen) {
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[3]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[4]).addrm, op, BIT_LEN_ADDRM);
				instr.attr = MATTR_UE;
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprMSize[instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if (!I0OprMSize[instr.opr[2].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[3].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[3].addrm];
			if (!I0OprMSize[instr.opr[4].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[4].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateSCMP_NW(&instr, tpc, nativelimit);
			break;
		case OP_EXIT:
			i0instrlen = BYTE_OP_EXIT;
			GET_INST_FIELD_NW(instr.option, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_EXIT);
			(*spc) += i0instrlen;
			return TranslateEXIT_NW(&instr, tpc, nativelimit);
			break;
		default:
			RETURN_DECODE_STATUS(OPCODE_UNDEFINED, 0)
			;
			break;
		}
	}
}
