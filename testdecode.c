#include <stdint.h>
#include "I0Symbol.h"
#include "ASM_MACROS.h"
#include "DecodeStatus.h"

#ifdef MSVC
#define typeof(a) uint8_t*
#endif

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

#define DECODE_OPR_I_ZX(opr, spc, attr) \
	do { \
		switch(attr) \
		{ \
		case ATTR_SB:\
			(opr).val.v64 = (*((uint8_t*)(spc)));\
			(spc) = (typeof(spc))(((uint8_t*)(spc))+1);\
			break;\
		case ATTR_SE:\
			(opr).val.v64 = (*((uint64_t*)(spc)));\
			(spc) = (typeof(spc))(((uint64_t*)(spc))+1);\
			break;\
		case ATTR_SF:\
			(opr).val.v64 = (*((uint32_t*)(spc)));\
			(spc) = (typeof(spc))(((uint32_t*)(spc))+1);\
			break;\
		case ATTR_UB:\
			(opr).val.v64 = (*((uint8_t*)(spc)));\
			(spc) = (typeof(spc))(((uint8_t*)(spc))+1);\
			break;\
		case ATTR_UE:\
			(opr).val.v64 = (*((uint64_t*)(spc)));\
			(spc) = (typeof(spc))(((uint64_t*)(spc))+1);\
			break;\
		case ATTR_UF:\
			(opr).val.v64 = (*((uint32_t*)(spc)));\
			(spc) = (typeof(spc))(((uint32_t*)(spc))+1);\
			break;\
		case ATTR_FS:\
			(opr).val.v64 = (*((uint32_t*)(spc)));\
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
			DECODE_OPR_I_ZX(opr, spc, attr);\
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
	unsigned long detail2;
}DECODE_STATUS;

#define RETURN_DECODE_STATUS(status, detail, detail2) \
	do{ DECODE_STATUS result= {(status), (detail), (detail2)}; return result;}while(0)

//I0OprISize[attr]
static uint8_t I0OprISize[0x10]=
{
	0x01, 0x08, 0x00, 0x04,
	0x01, 0x08, 0x00, 0x04,
	0x04, 0x08, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
};

//I0OprMSize[addrm]
static uint8_t I0OprMSize[0x08]=
{
	0x00, 0x08, 0x08, 0x0c,
	0x00, 0x00, 0x00, 0x00
};

//I0OprDSize[attr][addrm]
static uint8_t I0OprDSize[0x10][0x08]=
{
	{0x01, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x08, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x04, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x01, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x08, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x04, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x04, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x08, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

enum X86_64_REG{
	x64_AX,  x64_CX,  x64_DX,  x64_BX,
	x64_SP,  x64_BP,  x64_SI,  x64_DI,
	x64_R8,  x64_R9,  x64_R10, x64_R11,
	x64_R12, x64_R13, x64_R14, x64_R15
};

#define REX_B_BIT 0
#define REX_X_BIT 1
#define REX_R_BIT 2
#define REX_W_BIT 3

inline static uint8_t EncodeModRM(uint8_t* tpc, uint8_t mod, uint8_t rm, uint8_t reg)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(rm>=0x08)
	{
		rex |= (1<<REX_B_BIT);
	}
	uint8_t modrm = ((mod<<6)|(reg<<3)|rm);
	(*tpc) = modrm;
	return rex;
}

inline static uint8_t EncodeSIB(uint8_t* tpc, uint8_t ss, uint8_t idx, uint8_t base)
{
	uint8_t rex = 0;
	if(idx>=0x08)
	{
		rex |= (1<<REX_X_BIT);
		idx &= 0x07;
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	uint8_t sib = ((ss<<6)|(idx<<3)|base);
	(*tpc) = sib;
	return rex;
}


inline static uint8_t EncodeRM_0(uint8_t** tpc, uint8_t reg, uint8_t base, uint8_t idx, uint8_t ss)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(ss)
	{
		if(idx>=0x08)
		{
			rex |= (1<<REX_X_BIT);
			idx &= 0x07;
		}
		ss--;
		if(base == x64_BP)
		{
			(*((*tpc)++)) = EncodeModRM_NoCheck(1, 4, reg);
			(*((*tpc)++)) = EncodeSIB_NoCheck(ss,idx,base);
			(*((*tpc)++)) = 0;
		}
		else
		{
			(*((*tpc)++)) = EncodeModRM_NoCheck(0, 4, reg);
			(*((*tpc)++)) = EncodeSIB_NoCheck(ss,idx,base);
		}
	}
	else
	{
		if(base == x64_BP)
		{
			(*((*tpc)++)) = EncodeModRM_NoCheck(1, base, reg);
			(*((*tpc)++)) = 0;
		}
		else
		{
			(*((*tpc)++)) = EncodeModRM_NoCheck(0, base, reg);
		}
	}
	return rex;
}

inline static uint8_t EncodeRM_8(uint8_t** tpc, uint8_t reg, uint8_t base, uint8_t idx, uint8_t ss, uint8_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(ss)
	{
		if(idx>=0x08)
		{
			rex |= (1<<REX_X_BIT);
			idx &= 0x07;
		}
		ss--;
		(*((*tpc)++)) = EncodeModRM_NoCheck(1, 4, reg);
		(*((*tpc)++)) = EncodeSIB_NoCheck(ss,idx,base);
		(*((*tpc)++)) = offset;
	}
	else
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(1, base, reg);
		(*((*tpc)++)) = offset;
	}
	return rex;
}

inline static uint8_t EncodeRM_32(uint8_t** tpc, uint8_t reg, uint8_t base, uint8_t idx, uint8_t ss, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(ss)
	{
		if(idx>=0x08)
		{
			rex |= (1<<REX_X_BIT);
			idx &= 0x07;
		}
		ss--;
		(*((*tpc)++)) = EncodeModRM_NoCheck(2, 4, reg);
		(*((*tpc)++)) = EncodeSIB_NoCheck(ss,idx,base);
		(*((uint32_t*)(*tpc))) = offset;
		(*tpc) += 4;
	}
	else
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(2, base, reg);
		(*((uint32_t*)(*tpc))) = offset;
		(*tpc) += 4;
	}
	return rex;
}

inline static uint8_t EncodeModRM_NoCheck(uint8_t mod, uint8_t rm, uint8_t reg)
{
	uint8_t modrm = ((mod<<6)|(reg<<3)|rm);
	return modrm;
}
inline static uint8_t EncodeSIB_NoCheck(uint8_t ss, uint8_t idx, uint8_t base)
{
	uint8_t sib = ((ss<<6)|(idx<<3)|base);
	return sib;
}
inline static uint8_t EncodeRR_NoCheck(uint8_t src, uint8_t dest)
{
	return EncodeModRM_NoCheck(0x03, dest, src);
}
inline static void EncodeMovMI64ToAX_BYTE(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = 0xa0;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline static void EncodeMovMI64ToAX_WORD(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = 0x66;
	(*((*tpc)++)) = 0xa1;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline static void EncodeMovMI64ToAX_DWORD(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = 0xa1;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline static void EncodeMovMI64ToAX_QWORD(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = (0x40|(1<<REX_W_BIT));
	(*((*tpc)++)) = 0xa1;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline static void EncodeMovAxToMI64_BYTE(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = 0xa2;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline static void EncodeMovAxToMI64_WORD(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = 0x66;
	(*((*tpc)++)) = 0xa3;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline static void EncodeMovAxToMI64_DWORD(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = 0xa3;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline static void EncodeMovAxToMI64_QWORD(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = (0x40|(1<<REX_W_BIT));
	(*((*tpc)++)) = 0xa3;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline static void EncodeMovRR_BYTE(uint8_t** tpc, uint8_t src, uint8_t dest)
{
	uint8_t rex = 0;
	if(src>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		src &= 0x07;
	}
	if(dest>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		dest &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x88;
	(*((*tpc)++)) = EncodeRR_NoCheck(src, dest);
}
inline static void EncodeMovRR_WORD(uint8_t** tpc, uint8_t src, uint8_t dest)
{
	uint8_t rex = 0;
	(*((*tpc)++)) = 0x66;
	if(src>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		src &= 0x07;
	}
	if(dest>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		dest &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x89;
	(*((*tpc)++)) = EncodeRR_NoCheck(src, dest);
}
inline static void EncodeMovRR_DWORD(uint8_t** tpc, uint8_t src, uint8_t dest)
{
	uint8_t rex = 0;
	if(src>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		src &= 0x07;
	}
	if(dest>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		dest &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x89;
	(*((*tpc)++)) = EncodeRR_NoCheck(src, dest);
}
inline static void EncodeMovRR_QWORD(uint8_t** tpc, uint8_t src, uint8_t dest)
{
	uint8_t rex = 0;
	if(src>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		src &= 0x07;
	}
	if(dest>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		dest &= 0x07;
	}
	(*((*tpc)++)) = (0x40|rex|(1<<REX_W_BIT));
	(*((*tpc)++)) = 0x89;
	(*((*tpc)++)) = EncodeRR_NoCheck(src, dest);
}
inline static void EncodeMovMOff32ToR_BYTE(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x8a;
	(*((*tpc)++)) = EncodeModRM_NoCheck(2, base, reg);
	(*((uint32_t*)(*tpc))) = offset;
	(*tpc) += 4;
}
inline static void EncodeMovMOff32ToR_WORD(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x66;
	(*((*tpc)++)) = 0x8b;
	(*((*tpc)++)) = EncodeModRM_NoCheck(2, base, reg);
	(*((uint32_t*)(*tpc))) = offset;
	(*tpc) += 4;
}
inline static void EncodeMovMOff32ToR_DWORD(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x8b;
	(*((*tpc)++)) = EncodeModRM_NoCheck(2, base, reg);
	(*((uint32_t*)(*tpc))) = offset;
	(*tpc) += 4;
}
inline static void EncodeMovMOff32ToR_QWORD(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	(*((*tpc)++)) = (0x40|rex|(1<<REX_W_BIT));
	(*((*tpc)++)) = 0x8b;
	(*((*tpc)++)) = EncodeModRM_NoCheck(2, base, reg);
	(*((uint32_t*)(*tpc))) = offset;
	(*tpc) += 4;
}
inline static void EncodeMovMToR_BYTE(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x8a;
	if(base==x64_BP)
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(1, base, reg);
		(*((*tpc)++)) = 0x00;
	}
	else
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(0, base, reg);
	}
}
inline static void EncodeMovMToR_WORD(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x66;
	(*((*tpc)++)) = 0x8b;
	if(base==x64_BP)
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(1, base, reg);
		(*((*tpc)++)) = 0x00;
	}
	else
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(0, base, reg);
	}
}
inline static void EncodeMovMToR_DOWRD(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x8b;
	if(base==x64_BP)
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(1, base, reg);
		(*((*tpc)++)) = 0x00;
	}
	else
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(0, base, reg);
	}
}
inline static void EncodeMovMToR_QWORD(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	(*((*tpc)++)) = (0x40|rex|(1<<REX_W_BIT));
	(*((*tpc)++)) = 0x8b;
	if(base==x64_BP)
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(1, base, reg);
		(*((*tpc)++)) = 0x00;
	}
	else
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(0, base, reg);
	}
}

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
static DECODE_STATUS TranslateEXIT_NW(I0INSTR*, uint8_t**, uint8_t*);

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
static DECODE_STATUS TranslateEXIT_WR(I0INSTR*, uint8_t**, uint8_t*);

void sys_back_runner_wrapper(void);

static inline DECODE_STATUS TranslateEXIT_WR(I0INSTR* instr, uint8_t** tpc, uint8_t* nativelimit)
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

static inline DECODE_STATUS TranslateEXIT_NW(I0INSTR* instr, uint8_t** tpc, uint8_t* nativelimit)
{
	(void)instr;
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

inline DECODE_STATUS TranslateALU_WR(I0INSTR* i0instr, uint8_t** tpc, uint8_t* nativelimit)
{

}

DECODE_STATUS TranslateI0ToNative(uint8_t** spc, uint8_t** tpc, uint8_t* nativelimit, uint8_t* i0limit, unsigned int is_write) {
	I0INSTR instr;
	unsigned int op;
	unsigned long i0instrlen = 0;
	unsigned long i0len = (((unsigned long) i0limit) - ((unsigned long) (*spc)));
	if (i0len >= ((BIT_LEN_ADDR_SIZE_MODE + BIT_LEN_OPCODE + 8 - 1) / 8)) {
		LOAD_OP_WORD0(op, (*spc));
	} else {
		RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
	}
	instr.addr = (uint64_t) (*spc);
	GET_INST_FIELD_ZO(instr.opcode, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
	(instr.addr_size_mode) = ((instr.opcode) >> (BIT_LEN_OPCODE));
	(instr.opcode) &= ((1 << (BIT_LEN_OPCODE)) - 1);
	switch (instr.opcode) {
	case OP_NOP:
		i0instrlen = BYTE_OP_NOP;
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
		}
		(*spc) += i0instrlen;
		return TranslateNOP(&instr, tpc, nativelimit, is_write);
		break;
	case OP_ADD:
	case OP_SUB:
	case OP_MUL:
	case OP_DIV:
	case OP_AND:
	case OP_OR:
	case OP_XOR:
		i0instrlen = BYTE_OP_ALU;
		if (i0len >= i0instrlen) {
			LOAD_OP_BYTE2(op, (*spc));
			GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
		} else {
			RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
		}
		if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
		if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
		}
		(*spc) += BYTE_OP_ALU;
		DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
		DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
		DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
		return TranslateALU(&instr, tpc, nativelimit, is_write);
		break;
	case OP_B:
		GET_INST_FIELD_ZO(instr.option, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
		switch (instr.option) {
		case OPT_B_J:
			GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_ADDR_SIZE_MODE + BIT_LEN_OPCODE + BIT_LEN_OPT_B + BIT_LEN_RA);
			i0instrlen = BYTE_OP_BJ;
			i0instrlen += 8;
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
			}
			(*spc) += BYTE_OP_BJ;
			GET_INST_OPR_I64((instr.opr[0]), (*spc));
			if((instr.ra) == RA_B_JUMP_R)
			{
				(instr.opr[0].val.v64) += ((uint64_t)(*spc));
			}
			return TranslateBJ(&instr, tpc, nativelimit, is_write);
			break;
		case OPT_B_L:
		case OPT_B_LE:
		case OPT_B_E:
		case OPT_B_NE:
		case OPT_B_SL:
			i0instrlen = BYTE_OP_BCMP;
			if (i0len >= i0instrlen) {
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
			} else {
				RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			i0instrlen += 8;
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
			}
			(*spc) += BYTE_OP_BCMP;
			DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
			DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
			GET_INST_OPR_I64((instr.opr[2]), (*spc));
			if((instr.ra) == RA_B_JUMP_R)
			{
				(instr.opr[2].val.v64) += ((uint64_t)(*spc));
			}
			return TranslateBCMP(&instr, tpc, nativelimit, is_write);
			break;
		case OPT_B_Z:
		case OPT_B_NZ:
			i0instrlen = BYTE_OP_BZNZ;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_RA);
			} else {
				RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += (I0OprDSize[instr.attr][instr.opr[0].addrm] + 8U);
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
			}
			(*spc) += BYTE_OP_BZNZ;
			DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
			GET_INST_OPR_I64((instr.opr[2]), (*spc));
			if((instr.ra) == RA_B_JUMP_R)
			{
				(instr.opr[2].val.v64) += ((uint64_t)(*spc));
			}
			return TranslateBZNZ(&instr, tpc, nativelimit, is_write);
			break;
		case OPT_B_IJ:
			i0instrlen = BYTE_OP_BIJ;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_NW((instr.opr[0]).addrm, op, BIT_LEN_ADDR_SIZE_MODE + BIT_LEN_OPCODE + BIT_LEN_OPT_B + BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
			}
			if (!I0OprMSize[instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[0].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
			}
			(*spc) += BYTE_OP_BIJ;
			DECODE_OPR_M(instr.opr[0], *spc, instr.opr[0].addrm);
			return TranslateBIJ(&instr, tpc, nativelimit, is_write);
			break;
		default:
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPCODE_B_UNDEFINED, 0);
			break;
		}
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
			RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
		}
		if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
		if (!I0OprMSize[instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[1].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
		}
		(*spc) += BYTE_OP_CONV;
		DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
		DECODE_OPR_M(instr.opr[1], *spc, instr.opr[1].addrm);
		return TranslateCONV(&instr, tpc, nativelimit, is_write);
		break;
	case OP_INT:
		i0instrlen = BYTE_OP_INT;
		i0instrlen += 8;
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
		}
		(*spc) += BYTE_OP_INT;
		GET_INST_OPR_I8((instr.opr[0]), (*spc));
		return TranslateINT(&instr, tpc, nativelimit, is_write);
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
			RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
		}
		if (!I0OprMSize[instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[0].addrm];
		if (!I0OprMSize[instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (!I0OprMSize[instr.opr[3].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[3].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
		}
		(*spc) += BYTE_OP_SPAWN;
		DECODE_OPR_M(instr.opr[0], *spc, instr.opr[0].addrm);
		DECODE_OPR_M(instr.opr[1], *spc, instr.opr[1].addrm);
		DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
		DECODE_OPR_M(instr.opr[3], *spc, instr.opr[3].addrm);
		return TranslateSPAWN(&instr, tpc, nativelimit, is_write);
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
			RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
		}
		if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
		if (!I0OprDSize[MATTR_UB][instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[MATTR_UB][instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
		}
		(*spc) += BYTE_OP_SHIFT;
		DECODE_OPR_D(instr.opr[0], *spc, instr.opr[0].addrm, instr.attr);
		DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, MATTR_UB);
		DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
		return TranslateSHIFT(&instr, tpc, nativelimit, is_write);
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
			RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
		}
		if (!I0OprMSize[instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[0].addrm];
		if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (!I0OprDSize[instr.attr][instr.opr[3].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[3].addrm];
		if (!I0OprMSize[instr.opr[4].addrm]) {
			RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[4].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_DECODE_SEGMENT_LIMIT, 0, 0);
		}
		(*spc) += BYTE_OP_SCMP;
		DECODE_OPR_M(instr.opr[0], *spc, instr.opr[0].addrm);
		DECODE_OPR_D(instr.opr[1], *spc, instr.opr[1].addrm, instr.attr);
		DECODE_OPR_M(instr.opr[2], *spc, instr.opr[2].addrm);
		DECODE_OPR_D(instr.opr[3], *spc, instr.opr[3].addrm, instr.attr);
		DECODE_OPR_M(instr.opr[4], *spc, instr.opr[4].addrm);
		return TranslateSCMP(&instr, tpc, nativelimit, is_write);
		break;
	case OP_EXIT:
		i0instrlen = BYTE_OP_EXIT;
		GET_INST_FIELD_NW(instr.option, op, BIT_LEN_ADDR_SIZE_MODE + BIT_LEN_OPCODE + BIT_LEN_OPT_EXIT);
		(*spc) += i0instrlen;
		return TranslateEXIT(&instr, tpc, nativelimit, is_write);
		break;
	default:
		RETURN_DECODE_STATUS(I0_DECODE_INVALID_INSTRUCTION, I0_DECODE_OPCODE_UNDEFINED, 0)
		;
		break;
	}

}
