#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "I0Symbol.h"

#define x64_ALU_OP_ID_ADD		0
#define x64_ALU_OP_ID_OR		1
#define x64_ALU_OP_ID_ADC		2
#define x64_ALU_OP_ID_SBB		3
#define x64_ALU_OP_ID_AND		4
#define x64_ALU_OP_ID_SUB		5
#define x64_ALU_OP_ID_XOR		6
#define x64_ALU_OP_ID_CMP		7

#define x64_TTTN_O				0
#define x64_TTTN_NO				1
#define x64_TTTN_B				2
#define x64_TTTN_NB				3
#define x64_TTTN_Z				4
#define x64_TTTN_NZ				5
#define x64_TTTN_BE				6
#define x64_TTTN_NBE			7
#define x64_TTTN_S				8
#define x64_TTTN_NS				9
#define x64_TTTN_P				10
#define x64_TTTN_NP				11
#define x64_TTTN_L				12
#define x64_TTTN_NL				13
#define x64_TTTN_LE				14
#define x64_TTTN_NLE			15

#define x64_AX 0
#define x64_CX 1
#define x64_DX 2
#define x64_BX 3
#define x64_SP 4
#define x64_BP 5
#define x64_SI 6
#define x64_DI 7
#define x64_R8 8
#define x64_R9 9
#define x64_R10 10
#define x64_R11 11
#define x64_R12 12
#define x64_R13 13
#define x64_R14 14
#define x64_R15 15

#define REX_B_BIT 0
#define REX_X_BIT 1
#define REX_R_BIT 2
#define REX_W_BIT 3



uint8_t i0_x64_reg_map[0x08]=
{
	x64_BX,
	x64_CX,
	x64_R15,
	x64_R14,
	x64_R13,
	x64_R10,
	x64_R9,
	x64_R8
};

#define WITHIN64_8BIT(val) \
	(((int64_t)val) == ((int64_t)((int8_t)((uint8_t)((uint64_t)(val))))))

#define WITHIN64_32BIT(val) \
	(((int64_t)val) == ((int64_t)((int32_t)((uint32_t)((uint64_t)(val))))))

#define WITHIN32_8BIT(val) \
	(((int32_t)val) == ((int32_t)((int8_t)((uint8_t)((uint32_t)(val))))))

#define TYPE_ID_BYTE 0
#define TYPE_ID_DWORD 1
#define TYPE_ID_QWORD 2
#define TYPE_ID_OWORD 3

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

#define I0_REGISTER_BASE	0x400000000ULL

__inline int IS_I0_REGISTER_FILE(uint64_t i0addr)
{
	if(i0addr>(I0_REGISTER_BASE))
	{
		if(((i0addr-I0_REGISTER_BASE)/(8ULL))<(8ULL))
		{
			return 1;
		}
	}
	return 0;
}

__inline uint8_t MAP_TO_NATIVE_REGISTER(uint64_t i0addr)
{
	return (i0_x64_reg_map[((i0addr)-I0_REGISTER_BASE)/(8ULL)]);
}

static uint8_t I0OprISize[0x10]=
{
	TYPE_ID_BYTE,
	TYPE_ID_QWORD,
	TYPE_ID_OWORD,
	TYPE_ID_DWORD,
	TYPE_ID_BYTE,
	TYPE_ID_QWORD,
	TYPE_ID_OWORD,
	TYPE_ID_DWORD,
	TYPE_ID_DWORD,
	TYPE_ID_QWORD,
};

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


__inline  uint8_t EncodeModRM(uint8_t mod, uint8_t rm, uint8_t reg)
{
	uint8_t modrm = ((mod<<6)|(reg<<3)|rm);
	return modrm;
}
__inline  uint8_t EncodeSIB(uint8_t ss, uint8_t idx, uint8_t base)
{
	uint8_t sib = ((ss<<6)|(idx<<3)|base);
	return sib;
}
__inline static void x64EncodeFillOpr(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	instr->ModRM_SIB_len = 1;
	if((E.reg)>=0x08)
	{
		instr->REX |= (1<<REX_B_BIT);
		(E.reg) &= 0x07;
	}
	if((G.reg)>=0x08)
	{
		instr->REX |= (1<<REX_R_BIT);
		(G.reg) &= 0x07;
	}
	if(E.type == x64_OPR_TYPE_REG)
	{
		instr->ModRM_SIB[0] = EncodeModRM(0x03, E.reg, G.reg);
	}
	else
	{
		if(E.off32)
		{
			if(WITHIN32_8BIT(E.off32))
			{
				instr->ModRM_SIB[0] = EncodeModRM(1,(E.reg),(G.reg));
				instr->disp_len = 1;
				instr->disp.v8 = ((uint8_t)(E.off32));
			}
			else
			{
				instr->ModRM_SIB[0] = EncodeModRM(2,(E.reg),(G.reg));
				instr->disp_len = 4;
				instr->disp.v32 = E.off32;
			}
		}
		else
		{
			if((E.reg) == x64_BP)
			{
				instr->ModRM_SIB[0] = EncodeModRM(1,(E.reg),(G.reg));
				instr->disp_len = 1;
				instr->disp.v8 = 0;
			}
			else
			{
				instr->ModRM_SIB[0] = EncodeModRM(0,(E.reg),(G.reg));
			}
		}
	}
}

#define x64EncodeModRM_RR(src, dest) \
({\
	if(src>=0x08)\
	{\
		rex |= (1<<REX_R_BIT);\
		src &= 0x07;\
	}\
	if(dest>=0x08)\
	{\
		rex |= (1<<REX_B_BIT);\
		dest &= 0x07;\
	}\
	EncodeModRM(0x03, dest, src);\
})

#define x64EncodeModRM_Prep(base, reg) \
do{\
	if(reg>=0x08)\
	{\
		rex |= (1<<REX_R_BIT);\
		reg &= (0x07);\
	}\
	if(base>=0x08)\
	{\
		rex |= (1<<REX_B_BIT);\
		base &= 0x07;\
	}\
}while(0)
__inline void x64EncodeMovMI64ToAX(x64INSTR* instr, uint64_t moffs,uint8_t oprsize)
{
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_ID_BYTE:
		instr->opcode[0] = 0xa0;
		break;
	case TYPE_ID_DWORD:
		instr->opcode[0] = 0xa1;
		break;
	case TYPE_ID_QWORD:
		instr->opcode[0] = 0xa1;
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	instr->imm.v64 = moffs;
	instr->imm_len = 8;
}
__inline void x64EncodeMovAXToMI64(x64INSTR* instr, uint64_t moffs,uint8_t oprsize)
{
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_ID_BYTE:
		instr->opcode[0] = 0xa2;
		break;
	case TYPE_ID_DWORD:
		instr->opcode[0] = 0xa3;
		break;
	case TYPE_ID_QWORD:
		instr->opcode[0] = 0xa3;
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	instr->imm.v64 = moffs;
	instr->imm_len = 8;
}
__inline void x64EncodeMovGE(x64INSTR* instr, x64_OPR G, x64_OPR E, uint8_t oprsize)
{
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_ID_BYTE:
		instr->opcode[0] = 0x8a;
		break;
	case TYPE_ID_DWORD:
		instr->opcode[0] = 0x8b;
		break;
	case TYPE_ID_QWORD:
		instr->opcode[0] = 0x8b;
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	x64EncodeFillOpr(instr, G, E);
}
__inline void x64EncodeMovEG(x64INSTR* instr, x64_OPR E, x64_OPR G, uint8_t oprsize)
{
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_ID_BYTE:
		instr->opcode[0] = 0x88;
		break;
	case TYPE_ID_DWORD:
		instr->opcode[0] = 0x89;
		break;
	case TYPE_ID_QWORD:
		instr->opcode[0] = 0x89;
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	x64EncodeFillOpr(instr, G, E);
}
__inline void x64EncodeMovEI(x64INSTR* instr,  x64_OPR E, x64_OPR I, uint8_t oprsize)
{
	x64_OPR G;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_ID_BYTE:
		instr->opcode[0] = 0xc6;
		instr->imm_len = 1;
		instr->imm.v8 = I.imm.v8;
		break;
	case TYPE_ID_DWORD:
		instr->opcode[0] = 0xc7;
		instr->imm_len = 4;
		instr->imm.v32 = I.imm.v32;
		break;
	case TYPE_ID_QWORD:
		instr->opcode[0] = 0xc7;
		instr->REX = (1<<REX_W_BIT);
		instr->imm_len = 4;
		instr->imm.v32 = I.imm.v32;
		//sign-extended here!
		break;
	}
	G.reg = 0;
	x64EncodeFillOpr(instr, G, E);
}
__inline void x64EncodeMovGI(x64INSTR* instr, x64_OPR G, x64_OPR I,uint8_t oprsize)
{
	instr->opcode_len = 1;
	if((G.reg)>=0x08)
	{
		instr->REX = (1<<REX_B_BIT);
		(G.reg) &= 0x07;
	}
	switch(oprsize)
	{
	case TYPE_ID_BYTE:
		instr->opcode[0] = (0xb0 | (G.reg) );
		instr->imm_len = 1;
		instr->imm.v8 = I.imm.v8;
		break;
	case TYPE_ID_DWORD:
		instr->opcode[0] = (0xb8 | (G.reg) );
		instr->imm_len = 4;
		instr->imm.v32 = I.imm.v32;
		break;
	case TYPE_ID_QWORD:
		if(WITHIN64_32BIT(I.imm.v64))
		{
			x64EncodeMovEI(instr,G,I,oprsize);
			return;
		}
		else
		{
			instr->opcode[0] = (0xb8 | (G.reg) );
			instr->REX |= (1<<REX_W_BIT);
			instr->imm_len = 8;
			instr->imm.v64 = I.imm.v64;
		}
		break;
	}
}

__inline void x64EncodeAluGE(x64INSTR* instr, uint8_t op, x64_OPR G, x64_OPR E, uint8_t oprsize)
{
	op <<= 3;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_ID_BYTE:
		instr->opcode[0] = op + 2;
		break;
	case TYPE_ID_DWORD:
		instr->opcode[0] = op + 3;
		break;
	case TYPE_ID_QWORD:
		instr->REX = (1<<REX_W_BIT);
		instr->opcode[0] = op + 3;
	}
	x64EncodeFillOpr(instr, G, E);
}
__inline void x64EncodeAluEG(x64INSTR* instr, uint8_t op, x64_OPR E, x64_OPR G, uint8_t oprsize)
{
	op <<= 3;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_ID_BYTE:
		instr->opcode[0] = op;
		break;
	case TYPE_ID_DWORD:
		instr->opcode[0] = op + 1;
		break;
	case TYPE_ID_QWORD:
		instr->REX = (1<<REX_W_BIT);
		instr->opcode[0] = op + 1;
	}
	x64EncodeFillOpr(instr, G, E);
}
__inline void x64EncodeAluIToAX(x64INSTR* instr, uint8_t op, x64_OPR I, uint8_t oprsize)
{
	op <<= 3;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_ID_BYTE:
		instr->opcode[0] = op + 4;
		instr->imm_len = 1;
		instr->imm.v8 = I.imm.v8;
		break;
	case TYPE_ID_DWORD:
		instr->opcode[0] = op + 5;
		instr->imm_len = 4;
		instr->imm.v32 = I.imm.v32;
		break;
	case TYPE_ID_QWORD:
		instr->opcode[0] = op + 5;
		instr->REX = (1<<REX_W_BIT);
		instr->imm_len = 4;
		instr->imm.v32 = I.imm.v32;
		//sign-extended here!
	}
}
__inline void x64EncodeAluEI(x64INSTR* instr, uint8_t op, x64_OPR E,x64_OPR I, uint8_t oprsize)
{
	x64_OPR G;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_ID_BYTE:
		instr->opcode[0] = 0x80;
		instr->imm_len = 1;
		instr->imm.v8 = I.imm.v8;
		break;
	case TYPE_ID_DWORD:
		if(WITHIN32_8BIT(I.imm.v32))
		{
			instr->opcode[0] = 0x83;
			instr->imm_len = 1;
			instr->imm.v8 = I.imm.v8;
		}
		else
		{
			if( (E.type == x64_OPR_TYPE_REG) && (E.reg == x64_AX))
			{
				x64EncodeAluIToAX(instr,op,I,oprsize);
				return;
			}
			else
			{
				instr->opcode[0] = 0x81;
				instr->imm_len = 4;
				instr->imm.v32 = I.imm.v32;
			}
		}
		break;
	case TYPE_ID_QWORD:
		instr->REX = (1<<REX_W_BIT);
		if(WITHIN32_8BIT(I.imm.v32))
		{
			instr->opcode[0] = 0x83;
			instr->imm_len = 1;
			instr->imm.v8 = I.imm.v8;
		}
		else
		{
			if( (E.type == x64_OPR_TYPE_REG) && (E.reg == x64_AX))
			{
				x64EncodeAluIToAX(instr,op,I,oprsize);
				return;
			}
			else
			{
				instr->opcode[0] = 0x81;
				instr->imm_len = 4;
				instr->imm.v32 = I.imm.v32;
			}
		}
		break;
	}
	G.reg = op;
	x64EncodeFillOpr(instr, G, E);
}

__inline void x64EncodeMulDivE(x64INSTR* instr, uint8_t op, x64_OPR E, uint8_t oprsize)
{
	x64_OPR G;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_ID_BYTE:
		instr->opcode[0] = 0xf6;
		break;
	case TYPE_ID_DWORD:
		instr->opcode[0] = 0xf7;
		break;
	case TYPE_ID_QWORD:
		instr->opcode[0] = 0xf7;
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	G.reg = op;
	x64EncodeFillOpr(instr,G,E);
}
__inline void x64EncodeIMulGE(x64INSTR* instr, x64_OPR G, x64_OPR E, uint8_t oprsize)
{
	instr->opcode_len = 2;
	instr->opcode[0] = 0x0f;
	instr->opcode[1] = 0xaf;
	switch(oprsize)
	{
	case TYPE_ID_QWORD:
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	x64EncodeFillOpr(instr, G, E);
}
__inline void x64EncodeIMulGEI(x64INSTR* instr, x64_OPR G, x64_OPR E, x64_OPR I, uint8_t oprsize)
{
	instr->opcode_len = 1;
	if(WITHIN32_8BIT(I.imm.v32))
	{
		instr->opcode[0] = 0x6b;
		instr->imm_len = 1;
		instr->imm.v8 = I.imm.v8;
	}
	else
	{
		instr->opcode[0] = 0x69;
		instr->imm_len = 4;
		instr->imm.v32 = I.imm.v32;
	}
	switch(oprsize)
	{
	case TYPE_ID_QWORD:
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	x64EncodeFillOpr(instr, G, E);
}
__inline void x64EncodeJmpRel32(x64INSTR* instr, uint32_t off32)
{
	instr->opcode_len = 1;
	instr->opcode[0] = 0xe9;
	instr->imm_len = 4;
	instr->imm.v32 = off32;
}
__inline void x64EncodeJmpDirE(x64INSTR* instr,x64_OPR E)
{
	x64_OPR G;
	instr->opcode_len = 1;
	instr->opcode[0] = 0xff;
	G.reg = 4;
	x64EncodeFillOpr(instr,G,E);
}
__inline void x64EncodeJmpCcRel32(x64INSTR* instr, uint8_t tttn, uint32_t off32)
{
	instr->opcode_len = 2;
	instr->opcode[0] = 0xf0;
	instr->opcode[1] = 0x80 | tttn;
	instr->imm_len = 4;
	instr->imm.v32 = off32;
}
__inline void x64EncodeMovSx8To32GE(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	instr->opcode_len = 2;
	instr->opcode[0] = 0x0f;
	instr->opcode[1] = 0xbe;
	x64EncodeFillOpr(instr,G,E);
}
__inline void x64EncodeMovSx8To64GE(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	instr->opcode_len = 2;
	instr->REX = (1<<REX_W_BIT);
	instr->opcode[0] = 0x0f;
	instr->opcode[1] = 0xbe;
	x64EncodeFillOpr(instr,G,E);
}
__inline void x64EncodeMovSx32To64GE(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	instr->opcode_len = 1;
	instr->REX = (1<<REX_W_BIT);
	instr->opcode[0] = 0x63;
	x64EncodeFillOpr(instr,G,E);
}
__inline void x64EncodeMovZx8To32GE(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	instr->opcode_len = 2;
	instr->opcode[0] = 0x0f;
	instr->opcode[1] = 0xb6;
	x64EncodeFillOpr(instr,G,E);
}
__inline void x64EncodeMovZx8To64GE(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	instr->opcode_len = 2;
	instr->REX = (1<<REX_W_BIT);
	instr->opcode[0] = 0x0f;
	instr->opcode[1] = 0xb6;
	x64EncodeFillOpr(instr,G,E);
}
__inline void x64EncodeMovZx32To64GE(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	x64EncodeMovGE(instr,G,E,TYPE_ID_DWORD);
}
__inline uint8_t Getx64InstrSize(x64INSTR* instr)
{
	uint8_t rex_len = 0;
	if(instr->REX)
	{
		rex_len = 1;
	}
	else
	{
		rex_len = 0;
	}
	return(rex_len + (instr->LegacyPrefix_Len) + (instr->opcode_len) + (instr->ModRM_SIB_len) + (instr->disp_len) + (instr->imm_len));
}
__inline void x64EncodeNegE(x64INSTR* instr, x64_OPR E, uint8_t oprsize)
{
	x64_OPR G;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_ID_BYTE:
		instr->opcode[0] = 0xf6;
		break;
	case TYPE_ID_DWORD:
		instr->opcode[0] = 0xf7;
		break;
	case TYPE_ID_QWORD:
		instr->REX = (1<<REX_W_BIT);
		instr->opcode[0] = 0xf7;
		break;
	}
	G.reg = 0x03;
	x64EncodeFillOpr(instr,G,E);
}
__inline void Writex64Instr(x64INSTR* instr, uint8_t** tpc)
{
	if(instr->REX)
	{
		(*((*tpc)++)) =instr->REX;
	}
	memcpy((*tpc),(&(instr->opcode[0])),instr->opcode_len);
	(*tpc) += instr->opcode_len;
	memcpy((*tpc), (&(instr->ModRM_SIB[0])),instr->ModRM_SIB_len);
	(*tpc) += instr->ModRM_SIB_len;
	memcpy((*tpc), (&(instr->disp)),instr->disp_len);
	(*tpc) += instr->disp_len;
	memcpy((*tpc), (&(instr->imm.v8)),instr->imm_len);
	(*tpc) += instr->imm_len;
}
typedef struct _IMM_CACHE_REG{
	uint8_t reg;
	uint64_t imm;
}IMM_CACHE_REG;
typedef struct _MEM_CACHE_REG{
	uint8_t reg;
	uint64_t addr;
}MEM_CACHE_REG;

static __inline int FindImmCache(uint64_t imm,uint8_t* reg, IMM_CACHE_REG* imm_cache,uint8_t num)
{
	uint8_t i;
	for(i=0;i<num;i++)
	{
		if(imm_cache[i].imm == imm)
		{
			(*reg) = imm_cache[i].reg;
			return 1;
		}
	}
	return 0;
}

static __inline int FindMemCache(uint64_t addr,uint8_t* reg, MEM_CACHE_REG* mem_cache,uint8_t num)
{
	uint8_t i;
	for(i=0;i<num;i++)
	{
		if(mem_cache[i].addr == addr)
		{
			(*reg) = mem_cache[i].reg;
			return 1;
		}
	}
	return 0;
}

static __inline int FindMemOperandFromCache(uint64_t addr,x64_OPR* M, MEM_CACHE_REG* mem_cache,uint8_t mem_cache_num, IMM_CACHE_REG* imm_cache,uint8_t imm_cache_num)
{

}

#define OPR3_ADDRM_ID(addrm1,addrm2,addrm3) \
	((addrm1)|((addrm2)<<2)|((addrm3)<<4))

#define ZEROOUT_x64_INSTR() \
	do{memset((char*)(x64instrs + instr_cnt),0,sizeof(x64INSTR));}while(0)

#define ENCODE_OPR(i0opr,x64opr,reg_encoded) \
	do{\
		switch((i0opr).addrm)\
		{\
		case ADDRM_IMMEDIATE:\
			(x64opr).type = x64_OPR_I;\
			(x64opr).imm.v64 = (i0opr).val.v64;\
			break;\
		case ADDRM_ABSOLUTE:\
			if(IS_I0_REGISTER_FILE((i0opr).val.v64))\
			{\
				(x64opr).reg=MAP_TO_NATIVE_REGISTER((i0opr).val.v64);\
				(x64opr).type = x64_OPR_TYPE_REG;\
				(reg_encoded) = 1;\
			}\
			else\
			{\
				(x64opr).type = x64_OPR_TYPE_M;\
			}\
			break;\
		case ADDRM_DISPLACEMENT:\
			(x64opr).type = x64_OPR_TYPE_M;\
			if(IS_I0_REGISTER_FILE((i0opr).val.v64))\
			{\
				(x64opr).reg=MAP_TO_NATIVE_REGISTER((i0opr).val.v64);\
				(reg_encoded) = 1;\
			}\
			(x64opr).off32 = (i0opr).disp32;\
			break;\
		case ADDRM_INDIRECT:\
			(x64opr).type = x64_OPR_TYPE_M;\
			if(IS_I0_REGISTER_FILE((i0opr).val.v64))\
			{\
				(x64opr).reg=MAP_TO_NATIVE_REGISTER((i0opr).val.v64);\
				(reg_encoded) = 1;\
			}\
			(x64opr).off32 = 0;\
			break;\
		}\
	}while(0)

#define IDT_BASE 0x0

void TranslateINT(I0INSTR* i0instr, x64INSTR* x64instrs, uint32_t* _x64_ins_cnt)
{
	unsigned int instr_cnt = 0;
	x64_OPR x64oprs_tmp[4];
	x64oprs_tmp[0].type = x64_OPR_I;
	x64oprs_tmp[0].imm.v64 = IDT_BASE;
	x64oprs_tmp[1].type = x64_OPR_I;
	x64oprs_tmp[1].imm.v64 = (i0instr->opr[0].val.v64);
	x64oprs_tmp[2].type = x64_OPR_TYPE_REG;
	x64oprs_tmp[2].reg = x64_AX;
	x64oprs_tmp[3].type = x64_OPR_TYPE_REG;
	x64oprs_tmp[3].reg = x64_DX;
	ZEROOUT_x64_INSTR();
	x64EncodeMovGI(x64instrs+(instr_cnt++), x64oprs_tmp[3],x64oprs_tmp[0],TYPE_ID_QWORD);
	ZEROOUT_x64_INSTR();
	x64EncodeMovGI(x64instrs+(instr_cnt++), x64oprs_tmp[2],x64oprs_tmp[1],TYPE_ID_QWORD);
	ZEROOUT_x64_INSTR();
	x64instrs[instr_cnt].opcode_len = 1;
	x64instrs[instr_cnt].opcode[0] = 0xff;
	x64instrs[instr_cnt].ModRM_SIB_len = 2;
	x64instrs[instr_cnt].ModRM_SIB[0] = 0x14;
	x64instrs[instr_cnt].ModRM_SIB[1] = 0xc2;
	//callq *(%rdx, %rax, 8)
	instr_cnt++;
	return;
}

void TranslateAluOp(I0INSTR* instr, uint8_t** tpc, uint8_t op) {
	x64_OPR x64oprs[3];
	x64_OPR x64oprs_tmp[3];
	x64INSTR x64instrs[10];
	unsigned int opr_swapped = 0;
	unsigned int instr_cnt = 0;
	unsigned int opr_encoded_reg[3] = {0,0,0};
	unsigned int tmp;

	I0OPR* i0_opr0 = (&(instr->opr[0]));
	I0OPR* i0_opr1 = (&(instr->opr[1]));
	I0OPR* i0_opr2 = (&(instr->opr[2]));

	x64_OPR* x64_opr0 = (&(x64oprs[0]));
	x64_OPR* x64_opr1 = (&(x64oprs[1]));
	x64_OPR* x64_opr2 = (&(x64oprs[2]));

	if ((instr->opr[0].addrm) < (instr->opr[1].addrm)) {
		i0_opr0 = (&(instr->opr[1]));
		i0_opr1 = (&(instr->opr[0]));
		opr_swapped = 1;
	}

	ENCODE_OPR((*i0_opr0), (*x64_opr0), opr_encoded_reg[0]);
	ENCODE_OPR((*i0_opr1), (*x64_opr1), opr_encoded_reg[1]);

	if ((x64_opr0->type) < (x64_opr1->type)) {
		i0_opr0 = (&(instr->opr[1]));
		i0_opr1 = (&(instr->opr[0]));
		x64_opr0 = (&(x64oprs[1]));
		x64_opr1 = (&(x64oprs[0]));
		opr_swapped = 1;
		tmp = opr_encoded_reg[1];
		opr_encoded_reg[1] = opr_encoded_reg[0];
		opr_encoded_reg[0] = tmp;
	}

	ENCODE_OPR((*i0_opr2), (*x64_opr2), opr_encoded_reg[2]);

	switch (((x64_opr1->type) << 2) | (x64_opr0->type)) {
	case (((x64_OPR_I) << 2) | x64_OPR_I):
		x64_opr0->type = x64_OPR_I;
		switch (I0OprISize[instr->attr]) {
		case TYPE_ID_BYTE:
			switch (op) {
			case x64_ALU_OP_ID_ADD:
				x64_opr0->imm.v8 = (x64_opr0->imm.v8) + (x64_opr1->imm.v8);
				break;
			case x64_ALU_OP_ID_SUB:
				x64_opr0->imm.v8 = (x64_opr0->imm.v8) - (x64_opr1->imm.v8);
				break;
			case x64_ALU_OP_ID_AND:
				x64_opr0->imm.v8 = (x64_opr0->imm.v8) & (x64_opr1->imm.v8);
				break;
			case x64_ALU_OP_ID_OR:
				x64_opr0->imm.v8 = (x64_opr0->imm.v8) | (x64_opr1->imm.v8);
				break;
			case x64_ALU_OP_ID_XOR:
				x64_opr0->imm.v8 = (x64_opr0->imm.v8) ^ (x64_opr1->imm.v8);
				break;
			}
			break;
		case TYPE_ID_DWORD:
			switch (op) {
			case x64_ALU_OP_ID_ADD:
				x64_opr0->imm.v32 = (x64_opr0->imm.v32) + (x64_opr1->imm.v32);
				break;
			case x64_ALU_OP_ID_SUB:
				x64_opr0->imm.v32 = (x64_opr0->imm.v32) - (x64_opr1->imm.v32);
				break;
			case x64_ALU_OP_ID_AND:
				x64_opr0->imm.v32 = (x64_opr0->imm.v32) & (x64_opr1->imm.v32);
				break;
			case x64_ALU_OP_ID_OR:
				x64_opr0->imm.v32 = (x64_opr0->imm.v32) | (x64_opr1->imm.v32);
				break;
			case x64_ALU_OP_ID_XOR:
				x64_opr0->imm.v32 = (x64_opr0->imm.v32) ^ (x64_opr1->imm.v32);
				break;
			}
			break;
		case TYPE_ID_QWORD:
			switch (op) {
			case x64_ALU_OP_ID_ADD:
				x64_opr0->imm.v64 = (x64_opr0->imm.v64) + (x64_opr1->imm.v64);
				break;
			case x64_ALU_OP_ID_SUB:
				x64_opr0->imm.v64 = (x64_opr0->imm.v64) + (x64_opr1->imm.v64);
				break;
			case x64_ALU_OP_ID_AND:
				x64_opr0->imm.v64 = (x64_opr0->imm.v64) + (x64_opr1->imm.v64);
				break;
			case x64_ALU_OP_ID_OR:
				x64_opr0->imm.v64 = (x64_opr0->imm.v64) + (x64_opr1->imm.v64);
				break;
			case x64_ALU_OP_ID_XOR:
				x64_opr0->imm.v64 = (x64_opr0->imm.v64) + (x64_opr1->imm.v64);
				break;
			}
			break;
		}
		if (x64_opr2->type == x64_OPR_TYPE_REG) {
			ZEROOUT_x64_INSTR();
			x64EncodeMovGI(x64instrs + (instr_cnt++), (*x64_opr2), (*x64_opr0), I0OprISize[instr->attr]);
		} else {
			//x64_opr2->type == x64_OPR_TYPE_M
			if (i0_opr2->addrm == ADDRM_ABSOLUTE) {
				x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
				x64oprs_tmp[0].reg = x64_AX;
				ZEROOUT_x64_INSTR();
				x64EncodeMovGI(x64instrs + (instr_cnt++), x64oprs_tmp[0], (*x64_opr0), I0OprISize[instr->attr]);
				ZEROOUT_x64_INSTR();
				x64EncodeMovAXToMI64(x64instrs + (instr_cnt++), (i0_opr2->val.v64), I0OprISize[instr->attr]);
			} else {
				if (!(opr_encoded_reg[2])) {
					x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
					x64oprs_tmp[0].reg = x64_AX;
					ZEROOUT_x64_INSTR();
					x64EncodeMovMI64ToAX(x64instrs + (instr_cnt++), (i0_opr2->val.v64), TYPE_ID_QWORD);
					x64_opr2->reg = x64_AX;
				}
				if (((I0OprISize[instr->attr]) == TYPE_ID_QWORD) && (!(WITHIN64_32BIT(x64_opr0->imm.v64)))) {
					x64oprs_tmp[1].type = x64_OPR_TYPE_REG;
					x64oprs_tmp[1].reg = x64_SI;
					ZEROOUT_x64_INSTR();
					x64EncodeMovGI(x64instrs + (instr_cnt++), x64oprs_tmp[1], (*x64_opr0), I0OprISize[instr->attr]);
					ZEROOUT_x64_INSTR();
					x64EncodeMovEG(x64instrs + (instr_cnt++), (*x64_opr2), x64oprs_tmp[1], I0OprISize[instr->attr]);
				} else {
					ZEROOUT_x64_INSTR();
					x64EncodeMovEI(x64instrs + (instr_cnt++), (*x64_opr2), (*x64_opr0), I0OprISize[instr->attr]);
				}
			}
		}
		break;
	case ((x64_OPR_I << 2) | x64_OPR_TYPE_REG):
		if(((op)==(x64_ALU_OP_ID_SUB)) && (opr_swapped))
		{
			if(x64_opr2->type == x64_OPR_TYPE_REG)
			{
				if((x64_opr2->reg) == (x64_opr0->reg))
				{
					x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
					x64oprs_tmp[0].reg = x64_AX;
					ZEROOUT_x64_INSTR();
					x64EncodeMovEG(x64instrs + (instr_cnt++), (x64oprs_tmp[0]),(*x64_opr0), I0OprISize[instr->attr]);
					x64_opr0 = (&(x64oprs_tmp[0]));
				}
				ZEROOUT_x64_INSTR();
				x64EncodeMovGI(x64instrs + (instr_cnt++), (*x64_opr2), (*x64_opr1),I0OprISize[instr->attr]);
				ZEROOUT_x64_INSTR();
				x64EncodeAluEG(x64instrs + (instr_cnt++), op,(*x64_opr2), (*x64_opr0),I0OprISize[instr->attr]);
			}
			else
			{
				if(i0_opr2->addrm == ADDRM_ABSOLUTE)
				{
					x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
					x64oprs_tmp[0].reg = x64_AX;
				}
				else
				{
					x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
					x64oprs_tmp[0].reg = x64_SI;
					if(!(opr_encoded_reg[2]))
					{
						ZEROOUT_x64_INSTR();
						x64EncodeMovMI64ToAX(x64instrs + (instr_cnt++), i0_opr2->val.v64, I0OprISize[instr->attr]);
						x64_opr2->reg = x64_AX;
					}
				}
				ZEROOUT_x64_INSTR();
				x64EncodeMovGI(x64instrs + (instr_cnt++), (x64oprs_tmp[0]), (*x64_opr1), I0OprISize[instr->attr]);
				ZEROOUT_x64_INSTR();
				x64EncodeAluEG(x64instrs + (instr_cnt++), op, (x64oprs_tmp[0]), (*x64_opr0), I0OprISize[instr->attr]);
				ZEROOUT_x64_INSTR();
				if(i0_opr2->addrm == ADDRM_ABSOLUTE)
				{
					x64EncodeMovAXToMI64(x64instrs + (instr_cnt++), i0_opr2->val.v64, I0OprISize[instr->attr]);
				}
				else
				{
					x64EncodeMovEG(x64instrs + (instr_cnt++), (*x64_opr2), (x64oprs_tmp[0]), I0OprISize[instr->attr]);
				}
			}
		}
		else
		{
			if((op)==(x64_ALU_OP_ID_SUB))
			{
				(x64_opr1->imm.v64) = (uint64_t)(-((int64_t)(x64_opr1->imm.v64)));
				op = x64_ALU_OP_ID_ADD;
			}
			if(x64_opr2->type == x64_OPR_TYPE_REG)
			{
				if (((I0OprISize[instr->attr]) == TYPE_ID_QWORD) && (!(WITHIN64_32BIT(x64_opr1->imm.v64))))
				{
					if((x64_opr2->reg) == (x64_opr0->reg))
					{
						x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
						x64oprs_tmp[0].reg = x64_AX;
						ZEROOUT_x64_INSTR();
						x64EncodeMovGI(x64instrs + (instr_cnt++),x64oprs_tmp[0],(*x64_opr1),I0OprISize[instr->attr]);
						x64_opr0 = (&(x64oprs_tmp[0]));
					}
					else
					{
						ZEROOUT_x64_INSTR();
						x64EncodeMovGI(x64instrs + (instr_cnt++),(*x64_opr2),(*x64_opr1),I0OprISize[instr->attr]);
					}
					ZEROOUT_x64_INSTR();
					x64EncodeAluEG(x64instrs + (instr_cnt++),op,(*x64_opr2),(*x64_opr0),I0OprISize[instr->attr]);
				}
				else
				{
					if(!((x64_opr2->reg) == (x64_opr0->reg)))
					{
						ZEROOUT_x64_INSTR();
						x64EncodeMovEG(x64instrs + (instr_cnt++),(*x64_opr2),(*x64_opr0),I0OprISize[instr->attr]);
					}
					ZEROOUT_x64_INSTR();
					x64EncodeAluEI(x64instrs + (instr_cnt++),op,(*x64_opr2),(*x64_opr1),I0OprISize[instr->attr]);
				}
			}
			else
			{
				if(i0_opr2->addrm == ADDRM_ABSOLUTE)
				{
					x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
					x64oprs_tmp[0].reg = x64_AX;
				}
				else
				{
					if(!(opr_encoded_reg[2]))
					{
						ZEROOUT_x64_INSTR();
						x64EncodeMovMI64ToAX(x64instrs + (instr_cnt++), i0_opr2->val.v64, TYPE_ID_QWORD);
						x64_opr2->reg = x64_AX;
					}
					x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
					x64oprs_tmp[0].reg = x64_SI;
				}
				if (((I0OprISize[instr->attr]) == TYPE_ID_QWORD) && (!(WITHIN64_32BIT(x64_opr1->imm.v64))))
				{
					ZEROOUT_x64_INSTR();
					x64EncodeMovGI(x64instrs + (instr_cnt++),(x64oprs_tmp[0]),(*x64_opr1),I0OprISize[instr->attr]);
					ZEROOUT_x64_INSTR();
					x64EncodeAluEG(x64instrs + (instr_cnt++),op,(x64oprs_tmp[0]),(*x64_opr0),I0OprISize[instr->attr]);
				}
				else
				{
					ZEROOUT_x64_INSTR();
					x64EncodeMovEG(x64instrs + (instr_cnt++),(x64oprs_tmp[0]),(*x64_opr0),I0OprISize[instr->attr]);
					ZEROOUT_x64_INSTR();
					x64EncodeAluEI(x64instrs + (instr_cnt++),op,(x64oprs_tmp[0]),(*x64_opr1),I0OprISize[instr->attr]);
				}
				if(i0_opr2->addrm == ADDRM_ABSOLUTE)
				{
					ZEROOUT_x64_INSTR();
					x64EncodeMovAXToMI64(x64instrs + (instr_cnt++), i0_opr2->val.v64, I0OprISize[instr->attr]);
				}
				else
				{
					ZEROOUT_x64_INSTR();
					x64EncodeMovEG(x64instrs + (instr_cnt++), (*x64_opr2), (x64oprs_tmp[0]), I0OprISize[instr->attr]);
				}
			}
		}
		break;
	case ((x64_OPR_TYPE_REG<<2) | x64_OPR_TYPE_REG):
		if(x64_opr2->type == x64_OPR_TYPE_REG)
		{
			if( (x64_opr2->reg) == (x64_opr0->reg))
			{
				ZEROOUT_x64_INSTR();
				x64EncodeAluEG(x64instrs + (instr_cnt++), op, (*x64_opr2), (*x64_opr1), I0OprISize[instr->attr]);
			}
			else if((x64_opr2->reg) == (x64_opr1->reg))
			{
				if((op) == x64_ALU_OP_ID_SUB)
				{
					ZEROOUT_x64_INSTR();
					x64EncodeNegE(x64instrs + (instr_cnt++), (*x64_opr2), I0OprISize[instr->attr]);
					op = x64_ALU_OP_ID_ADD;
				}
				ZEROOUT_x64_INSTR();
				x64EncodeAluEG(x64instrs + (instr_cnt++),op, (*x64_opr2), (*x64_opr0), I0OprISize[instr->attr]);
			}
			else
			{
				ZEROOUT_x64_INSTR();
				x64EncodeMovEG(x64instrs + (instr_cnt++), (*x64_opr2), (*x64_opr0), I0OprISize[instr->attr]);
				ZEROOUT_x64_INSTR();
				x64EncodeAluEG(x64instrs + (instr_cnt++),op, (*x64_opr2), (*x64_opr1), I0OprISize[instr->attr]);
			}
		}
		else
		{
			if(i0_opr2->addrm == ADDRM_ABSOLUTE)
			{
				x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
				x64oprs_tmp[0].reg = x64_AX;
			}
			else
			{
				if(!(opr_encoded_reg[2]))
				{
					ZEROOUT_x64_INSTR();
					x64EncodeMovMI64ToAX(x64instrs + (instr_cnt++), i0_opr2->val.v64, TYPE_ID_QWORD);
					x64_opr2->reg = x64_AX;
				}
				x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
				x64oprs_tmp[0].reg = x64_SI;
			}
			ZEROOUT_x64_INSTR();
			x64EncodeMovEG(x64instrs + (instr_cnt++), (x64oprs_tmp[0]), (*x64_opr0), I0OprISize[instr->attr]);
			ZEROOUT_x64_INSTR();
			x64EncodeAluEG(x64instrs + (instr_cnt++), op, (x64oprs_tmp[0]), (*x64_opr1), I0OprISize[instr->attr]);
			ZEROOUT_x64_INSTR();
			if(i0_opr2->addrm == ADDRM_ABSOLUTE)
			{
				x64EncodeMovAXToMI64(x64instrs + (instr_cnt++), i0_opr2->val.v64, I0OprISize[instr->attr]);
			}
			else
			{
				x64EncodeMovEG(x64instrs + (instr_cnt++), (*x64_opr2), (x64oprs_tmp[0]),  I0OprISize[instr->attr]);
			}
		}
		break;
	case ((x64_OPR_I<<2)| x64_OPR_TYPE_M):
		if((op == x64_ALU_OP_ID_SUB) && opr_swapped)
		{

		}
		else
		{
			if((op) == x64_ALU_OP_ID_SUB)
			{
				(x64_opr1->imm.v64) = (uint64_t)(-((int64_t)(x64_opr1->imm.v64)));
				op = x64_ALU_OP_ID_ADD;
			}
			if((x64_opr2->type) == x64_OPR_TYPE_REG)
			{
				if((i0_opr0->addrm) == ADDRM_ABSOLUTE)
				{
					x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
					x64oprs_tmp[0].reg = x64_AX;
					x64oprs_tmp[1].type = x64_OPR_I;
					x64oprs_tmp[1].imm.v64 = (i0_opr0->val.v64);
					ZEROOUT_x64_INSTR();
					x64EncodeMovGI(x64instrs + (instr_cnt++), (x64oprs_tmp[0]), (x64oprs_tmp[1]), TYPE_ID_QWORD);
					x64_opr0->reg = x64_AX;
					x64_opr0->off32 = 0;
				}
				else
				{
					if(!(opr_encoded_reg[0]))
					{
						ZEROOUT_x64_INSTR();
						x64EncodeMovMI64ToAX(x64instrs + (instr_cnt++), (i0_opr0->val.v64), TYPE_ID_QWORD);
						x64_opr0->reg = x64_AX;
					}
				}
				if (((I0OprISize[instr->attr]) == TYPE_ID_QWORD) && (!(WITHIN64_32BIT(x64_opr1->imm.v64))))
				{
					if((x64_opr2->reg) == (x64_opr0->reg))
					{
						ZEROOUT_x64_INSTR();

					}
				}
				else
				{
				}
		break;
	}
	return;
}

