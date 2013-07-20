#include <stdint.h>
#include <string.h>
#include "x64Encode.h"

__inline static void Writex64Instrs(x64INSTR* instrs,uint32_t instr_cnt, uint8_t* nativeblock, uint64_t* nativelimit, int is_write)
{
	uint32_t i;
	for(i=0;i<instr_cnt;i++)
	{
		x64INSTR* instr = (&(instrs[i]));
		if(instr->REX)
		{
			if(is_write)
			{
				nativeblock[(*nativelimit)++] =( (instr->REX) | 0x40);
			}
			else
			{
				(*nativelimit) ++;
			}
		}
		if(is_write)
		{
			memcpy(nativeblock + (*nativelimit),(&(instr->opcode[0])),instr->opcode_len);
		}
		(*nativelimit) += instr->opcode_len;
		if(is_write)
		{
			memcpy(nativeblock + (*nativelimit), (&(instr->ModRM_SIB[0])),instr->ModRM_SIB_len);
		}
		(*nativelimit) += instr->ModRM_SIB_len;
		if(is_write)
		{
			memcpy(nativeblock + (*nativelimit), (&(instr->disp)),instr->disp_len);
		}
		(*nativelimit) += instr->disp_len;
		if(is_write)
		{
			memcpy(nativeblock + (*nativelimit), (&(instr->imm.v8)),instr->imm_len);
		}
		(*nativelimit) += instr->imm_len;
	}
}
__inline static uint8_t EncodeModRM(uint8_t mod, uint8_t rm, uint8_t reg)
{
	uint8_t modrm = ((mod<<6)|(reg<<3)|rm);
	return modrm;
}
__inline static  uint8_t EncodeSIB(uint8_t ss, uint8_t idx, uint8_t base)
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
__inline static void x64EncodeMovMI64ToAX(x64INSTR* instr, uint64_t moffs,uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_LEN_BYTE:
		instr->opcode[0] = 0xa0;
		break;
	case TYPE_LEN_DWORD:
		instr->opcode[0] = 0xa1;
		break;
	case TYPE_LEN_QWORD:
		instr->opcode[0] = 0xa1;
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	instr->imm.v64 = moffs;
	instr->imm_len = 8;
}
__inline static void x64EncodeMovAXToMI64(x64INSTR* instr, uint64_t moffs,uint8_t oprsize)
{	
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_LEN_BYTE:
		instr->opcode[0] = 0xa2;
		break;
	case TYPE_LEN_DWORD:
		instr->opcode[0] = 0xa3;
		break;
	case TYPE_LEN_QWORD:
		instr->opcode[0] = 0xa3;
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	instr->imm.v64 = moffs;
	instr->imm_len = 8;
}
__inline static void x64EncodeMovGE(x64INSTR* instr, x64_OPR G, x64_OPR E, uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_LEN_BYTE:
		instr->opcode[0] = 0x8a;
		break;
	case TYPE_LEN_DWORD:
		instr->opcode[0] = 0x8b;
		break;
	case TYPE_LEN_QWORD:
		instr->opcode[0] = 0x8b;
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	x64EncodeFillOpr(instr, G, E);
}
__inline static void x64EncodeMovEG(x64INSTR* instr, x64_OPR E, x64_OPR G, uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_LEN_BYTE:
		instr->opcode[0] = 0x88;
		break;
	case TYPE_LEN_DWORD:
		instr->opcode[0] = 0x89;
		break;
	case TYPE_LEN_QWORD:
		instr->opcode[0] = 0x89;
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	x64EncodeFillOpr(instr, G, E);
}
__inline static void x64EncodeMovEI(x64INSTR* instr,  x64_OPR E, x64_OPR I, uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	x64_OPR G;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_LEN_BYTE:
		instr->opcode[0] = 0xc6;
		instr->imm_len = 1;
		instr->imm.v8 = I.imm.v8;
		break;
	case TYPE_LEN_DWORD:
		instr->opcode[0] = 0xc7;
		instr->imm_len = 4;
		instr->imm.v32 = I.imm.v32;
		break;
	case TYPE_LEN_QWORD:
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
__inline static void x64EncodeMovGI(x64INSTR* instr, x64_OPR G, x64_OPR I,uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 1;
	if((G.reg)>=0x08)
	{
		instr->REX = (1<<REX_B_BIT);
		(G.reg) &= 0x07;
	}
	switch(oprsize)
	{
	case TYPE_LEN_BYTE:
		instr->opcode[0] = (0xb0 | (G.reg) );
		instr->imm_len = 1;
		instr->imm.v8 = I.imm.v8;
		break;
	case TYPE_LEN_DWORD:
		instr->opcode[0] = (0xb8 | (G.reg) );
		instr->imm_len = 4;
		instr->imm.v32 = I.imm.v32;
		break;
	case TYPE_LEN_QWORD:
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
__inline static void x64EncodeAluGE(x64INSTR* instr, uint8_t op, x64_OPR G, x64_OPR E, uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	op <<= 3;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_LEN_BYTE:
		instr->opcode[0] = op + 2;
		break;
	case TYPE_LEN_DWORD:
		instr->opcode[0] = op + 3;
		break;
	case TYPE_LEN_QWORD:
		instr->REX = (1<<REX_W_BIT);
		instr->opcode[0] = op + 3;
	}
	x64EncodeFillOpr(instr, G, E);
}
__inline static void x64EncodeAluEG(x64INSTR* instr, uint8_t op, x64_OPR E, x64_OPR G, uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	op <<= 3;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_LEN_BYTE:
		instr->opcode[0] = op;
		break;
	case TYPE_LEN_DWORD:
		instr->opcode[0] = op + 1;
		break;
	case TYPE_LEN_QWORD:
		instr->REX = (1<<REX_W_BIT);
		instr->opcode[0] = op + 1;
	}
	x64EncodeFillOpr(instr, G, E);
}
__inline static void x64EncodeAluIToAX(x64INSTR* instr, uint8_t op, x64_OPR I, uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	op <<= 3;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_LEN_BYTE:
		instr->opcode[0] = op + 4;
		instr->imm_len = 1;
		instr->imm.v8 = I.imm.v8;
		break;
	case TYPE_LEN_DWORD:
		instr->opcode[0] = op + 5;
		instr->imm_len = 4;
		instr->imm.v32 = I.imm.v32;
		break;
	case TYPE_LEN_QWORD:
		instr->opcode[0] = op + 5;
		instr->REX = (1<<REX_W_BIT);
		instr->imm_len = 4;
		instr->imm.v32 = I.imm.v32;
		//sign-extended here!
	}
}
__inline static void x64EncodeAluEI(x64INSTR* instr, uint8_t op, x64_OPR E,x64_OPR I, uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	x64_OPR G;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_LEN_BYTE:
		instr->opcode[0] = 0x80;
		instr->imm_len = 1;
		instr->imm.v8 = I.imm.v8;
		break;
	case TYPE_LEN_DWORD:
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
	case TYPE_LEN_QWORD:
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
__inline static void x64EncodeAlu(x64INSTR* instr, uint8_t op, x64_OPR dest, x64_OPR src, uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	switch(src.type)
	{
	case x64_OPR_I:
		x64EncodeAluEI(instr, op, dest, src, oprsize);
		return;
	case x64_OPR_TYPE_M:
		x64EncodeAluGE(instr, op, dest, src, oprsize);
		return;
	case x64_OPR_TYPE_REG:
		x64EncodeAluEG(instr, op, dest, src, oprsize);
		return;
	}
}
__inline static void x64EncodeMulDivE(x64INSTR* instr, uint8_t op, x64_OPR E, uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	x64_OPR G;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_LEN_BYTE:
		instr->opcode[0] = 0xf6;
		break;
	case TYPE_LEN_DWORD:
		instr->opcode[0] = 0xf7;
		break;
	case TYPE_LEN_QWORD:
		instr->opcode[0] = 0xf7;
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	G.reg = op;
	x64EncodeFillOpr(instr,G,E);
}
__inline static void x64EncodeIMulGE(x64INSTR* instr, x64_OPR G, x64_OPR E, uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 2;
	instr->opcode[0] = 0x0f;
	instr->opcode[1] = 0xaf;
	switch(oprsize)
	{
	case TYPE_LEN_QWORD:
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	x64EncodeFillOpr(instr, G, E);
}
__inline static void x64EncodeIMulGEI(x64INSTR* instr, x64_OPR G, x64_OPR E, x64_OPR I, uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
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
	case TYPE_LEN_QWORD:
		instr->REX = (1<<REX_W_BIT);
		break;
	}
	x64EncodeFillOpr(instr, G, E);
}
__inline static void x64EncodeJmpRel32(x64INSTR* instr, uint32_t off32)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 1;
	instr->opcode[0] = 0xe9;
	instr->imm_len = 4;
	instr->imm.v32 = off32;
}
__inline static void x64EncodeJmpDirE(x64INSTR* instr,x64_OPR E)
{
	ZEROOUT_x64_INSTR(instr);
	x64_OPR G;
	instr->opcode_len = 1;
	instr->opcode[0] = 0xff;
	G.reg = 4;
	x64EncodeFillOpr(instr,G,E);
}
__inline static void x64EncodeCallDirE(x64INSTR* instr, x64_OPR E)
{
	ZEROOUT_x64_INSTR(instr);
	x64_OPR G;
	instr->opcode_len = 1;
	instr->opcode[0] = 0xff;
	G.reg = 2;
	x64EncodeFillOpr(instr, G, E);
}
__inline static void x64EncodeJmpCcRel32(x64INSTR* instr, uint8_t tttn, uint32_t off32)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 2;
	instr->opcode[0] = 0x0f;
	instr->opcode[1] = (0x80 | tttn);
	instr->imm_len = 4;
	instr->imm.v32 = off32;
}
__inline static void x64EncodeJmpCcRel8(x64INSTR* instr, uint8_t tttn, uint8_t off8)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 1;
	instr->opcode[0] = (0x70 | tttn);
	instr->imm_len = 1;
	instr->imm.v8 = off8;
}
__inline static void x64EncodeCallAbsImm64(uint8_t* nativeblock, uint64_t* nativelimit, uint64_t addr, int is_write)
{
	x64INSTR x64instrs[2];
	uint32_t instr_cnt = 0;
	x64_OPR x64oprs_tmp[4];
	x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
	x64oprs_tmp[0].reg = x64_AX;
	x64oprs_tmp[1].type = x64_OPR_I;
	x64oprs_tmp[1].imm.v64 = addr;
	x64EncodeMovGI(x64instrs+ (instr_cnt++), x64oprs_tmp[0], x64oprs_tmp[1], TYPE_LEN_QWORD);
	x64EncodeCallDirE(x64instrs + (instr_cnt++), x64oprs_tmp[0]);
	Writex64Instrs(x64instrs, instr_cnt, nativeblock, nativelimit, is_write);
}
__inline static void x64EncodeMovSx8To32GE(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 2;
	instr->opcode[0] = 0x0f;
	instr->opcode[1] = 0xbe;
	x64EncodeFillOpr(instr,G,E);
}
__inline static void x64EncodeMovSx8To64GE(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 2;
	instr->REX = (1<<REX_W_BIT);
	instr->opcode[0] = 0x0f;
	instr->opcode[1] = 0xbe;
	x64EncodeFillOpr(instr,G,E);
}
__inline static void x64EncodeMovSx32To64GE(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 1;
	instr->REX = (1<<REX_W_BIT);
	instr->opcode[0] = 0x63;
	x64EncodeFillOpr(instr,G,E);
}
__inline static void x64EncodeMovZx8To32GE(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 2;
	instr->opcode[0] = 0x0f;
	instr->opcode[1] = 0xb6;
	x64EncodeFillOpr(instr,G,E);
}
__inline static void x64EncodeMovZx8To64GE(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	ZEROOUT_x64_INSTR(instr);
	instr->opcode_len = 2;
	instr->REX = (1<<REX_W_BIT);
	instr->opcode[0] = 0x0f;
	instr->opcode[1] = 0xb6;
	x64EncodeFillOpr(instr,G,E);
}
__inline static void x64EncodeMovZx32To64GE(x64INSTR* instr, x64_OPR G, x64_OPR E)
{
	x64EncodeMovGE(instr,G,E,TYPE_LEN_DWORD);
}
__inline static void x64EncodeNegE(x64INSTR* instr, x64_OPR E, uint8_t oprsize)
{
	ZEROOUT_x64_INSTR(instr);
	x64_OPR G;
	instr->opcode_len = 1;
	switch(oprsize)
	{
	case TYPE_LEN_BYTE:
		instr->opcode[0] = 0xf6;
		break;
	case TYPE_LEN_DWORD:
		instr->opcode[0] = 0xf7;
		break;
	case TYPE_LEN_QWORD:
		instr->REX = (1<<REX_W_BIT);
		instr->opcode[0] = 0xf7;
		break;
	}
	G.reg = 0x03;
	x64EncodeFillOpr(instr,G,E);
}
__inline static uint8_t Getx64InstrSize(x64INSTR* instr)
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
