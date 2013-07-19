#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "I0Symbol.h"
#include "I0Types.h"
#include "DecodeStatus.h"
#include "sys_config.h"
#include "asm_func.h"
#include "DecodeI0.h"
#include "asm_func.h"
#include "zlog_mod.h"
#include "PrintDisasm.h"
#include "x64Encode.h"
#include "x64Encode.c"

extern void* IndirJmpHashTab;

void error(char*);

int translate2x86_64_alu_op(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl);
int translate2x86_64_div(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl);
int translate2x86_64_exit(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl);
int translate2x86_64_nop(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl);
int translate2x86_64_mov(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl);
int translate2x86_64_shift(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl);
int translate2x86_64_spawn(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl);
int translate2x86_64_strcmp(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl);

static uint8_t i0_x64_reg_map[0x08]=
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

__inline static int IS_I0_REGISTER_FILE(uint64_t i0addr)
{
	if(i0addr>(REG_FILE_BEGIN))
	{
		if(((i0addr-REG_FILE_BEGIN)/(8ULL))<(8ULL))
		{
			return 1;
		}
	}
	return 0;
}

__inline static uint8_t MAP_TO_NATIVE_REGISTER(uint64_t i0addr)
{
	return (i0_x64_reg_map[((i0addr)-REG_FILE_BEGIN)/(8ULL)]);
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
				(x64opr).off32 = 0;\
				(reg_encoded) = 0;\
			}\
			break;\
		case ADDRM_DISPLACEMENT:\
			(x64opr).type = x64_OPR_TYPE_M;\
			if(IS_I0_REGISTER_FILE((i0opr).val.v64))\
			{\
				(x64opr).reg=MAP_TO_NATIVE_REGISTER((i0opr).val.v64);\
				(reg_encoded) = 1;\
			}\
			else\
			{\
				(reg_encoded) = 0;\
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
			else\
			{\
				(reg_encoded) = 0;\
			}\
			(x64opr).off32 = 0;\
			break;\
		}\
	}while(0)


DECODE_STATUS TranslateINT(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write)
{
	x64INSTR x64instrs[5];
	uint32_t instr_cnt = 0;
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
	Writex64Instrs(x64instrs, instr_cnt, nativeblock, nativelimit, is_write);
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH, I0_DECODE_INT, (*nativelimit));
}

//typedef struct DECODE_STATUS ;
DECODE_STATUS TranslateBJ(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write)
{
	x64EncodeCallAbsImm64(nativeblock, nativelimit, (i0instr->opr[0].val.v64), is_write);
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH, I0_DECODE_JMP, (*nativelimit) -12);
}

DECODE_STATUS TranslateBIJ(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write)
{
	//25 ?? ?? ?? ??		andl $0x(cnt_of_ijtab-1), %eax 
	//c1 e0 03				shll $0x03, %eax
	//48 8d bc 40 ??x4		leaq ijtab_addr(%rax, %rax, 2), %rdi
	//48 3b 37				cmpq (%rdi), %rsi
	//75 05					jne .+5
	//ff 67 08				jmpq *0x08(%rdi)
	//eb 07					jmp .+7
	//b8 ??x4				movl hash_tab_miss_handler, %eax
	//ff e0					jmpq *%rax
	static const uint8_t and_eax_opcode[1] = {0x25};
	static const uint8_t shl_eax_3_lea_3rax_op[7] = {0xc1, 0xe0, 0x03, 0x48, 0x8d, 0xbc, 0x40};
	static const uint8_t rest1[11] = {0x48, 0x3b, 0x37, 0x75, 0x05, 
		0xff, 0x67, 0x08, 0xeb, 0x07, 0xb8};
	static const uint8_t jmpq_rax[2] = {0xff, 0xe0};
	//read dest addr to %rax
	I0OPR* i0_opr = (&(i0instr->opr[0]));
	x64INSTR x64instrs[20];
	uint32_t instr_cnt = 0;
	x64_OPR x64oprs[3];
	x64_OPR x64oprs_tmp[3];
	x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
	x64oprs_tmp[0].reg = x64_AX;
	x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
	x64oprs_tmp[0].reg = x64_SI;
	uint32_t opr_encoded_reg[1] = {0};
	ENCODE_OPR((*i0_opr), x64oprs[0], opr_encoded_reg[0]);
	do{
		if(!(opr_encoded_reg[0]))
		{
			ZEROOUT_x64_INSTR();
			x64EncodeMovMI64ToAX(x64instrs+(instr_cnt++), (i0_opr->val.v64), TYPE_ID_QWORD);
			x64oprs[0].reg = x64_AX;
			if((i0_opr->addrm) == ADDRM_ABSOLUTE)
			{
				x64oprs[0].type = x64_OPR_TYPE_REG;
				break;
			}
		}
		ZEROOUT_x64_INSTR();
		x64EncodeMovGE(x64instrs+(instr_cnt++), x64oprs_tmp[0], x64oprs[0], TYPE_ID_QWORD);
	}while(0);
	ZEROOUT_x64_INSTR();
	x64EncodeMovEG(x64instrs+(instr_cnt++), x64oprs_tmp[1], x64oprs_tmp[0], TYPE_ID_QWORD);
	Writex64Instrs(x64instrs, instr_cnt, nativeblock, nativelimit, is_write);
	if(is_write)
	{
		memcpy(nativeblock+(*nativelimit), and_eax_opcode, 1);
		(*nativeblock) += 1;
		(*((uint32_t*)(nativeblock+(*nativelimit)))) = ( IJ_TABLE_SIZE - 1);
		(*nativeblock) += 4;
		memcpy(nativeblock+(*nativelimit),shl_eax_3_lea_3rax_op, 7);
		(*nativeblock) += 7;
		(*((uint32_t*)(nativeblock+(*nativelimit)))) = ((uint32_t) ((uint64_t)IndirJmpHashTab) );
		(*nativeblock) += 4;
		memcpy(nativeblock+(*nativelimit), rest1, 11);
		(*nativeblock) += 11;
		(*((uint32_t*)(nativeblock+(*nativelimit)))) = ((uint32_t) ((uint64_t)MapSpcToTpc_Thunk) );
		(*nativeblock) += 4;
		memcpy(nativeblock+(*nativelimit), jmpq_rax, 2);
		(*nativeblock) += 2;
	}
	else
	{
		(*nativelimit) += (5+3+4+4+3+2+5+7);
	}
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH, I0_DECODE_JMP_INDIR, (*nativelimit));
}

DECODE_STATUS TranslateBZNZ(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write)
{
	x64INSTR x64instrs[10];
	uint32_t instr_cnt = 0;
	x64_OPR x64oprs[1];
	x64_OPR x64oprs_tmp[1];
	unsigned int opr_encoed_reg[1] = {0};
	ENCODE_OPR((i0instr->opr[0]), x64oprs[0], opr_encoed_reg[0]);
	x64oprs_tmp[0].type = x64_OPR_I;
	x64oprs_tmp[0].imm.v64 = 0;
	int cmp_imm_z = 0;
	uint8_t tttn;
	if((i0instr->option) == OPT_B_Z)
	{
		tttn = x64_TTTN_NZ;
	}
	else
	{
		tttn = x64_TTTN_NZ;
	}
	switch(x64oprs[0].type)
	{
	case x64_OPR_I:
		switch(I0OprISize[i0instr->attr])
		{
		case TYPE_ID_BYTE:
			cmp_imm_z = ((x64oprs[0].imm.v8) == 0);
			break;
		case TYPE_ID_DWORD:
			cmp_imm_z = ((x64oprs[0].imm.v32) == 0);
			break;
		case TYPE_ID_QWORD:
			cmp_imm_z = ((x64oprs[0].imm.v64) == 0);
			break;
		}
		if((cmp_imm_z && ((i0instr->option) == OPT_B_Z)) || 
			((!cmp_imm_z) && ((i0instr->option) == OPT_B_NZ)))
		{
			x64EncodeCallAbsImm64(nativeblock, nativelimit,(i0instr->opr[2].val.v64),is_write);
			RETURN_DECODE_STATUS(I0_DECODE_BRANCH,I0_DECODE_JMP,(*nativelimit)-12);
		}
		else
		{
			RETURN_DECODE_STATUS(I0_TRANS_SUCCESS_NO_FURTHER_PROC,0,0);
		}
		break;
	case x64_OPR_TYPE_M:
		if(!(opr_encoed_reg[0]))
		{
			ZEROOUT_x64_INSTR();
			x64EncodeMovMI64ToAX(x64instrs + (instr_cnt++),(i0instr->opr[0].val.v64), I0OprISize[i0instr->attr]);	
			x64oprs[0].reg = x64_AX;
			if((i0instr->opr[0].addrm) == ADDRM_ABSOLUTE)
			{
				x64oprs[0].type = x64_OPR_TYPE_REG;
			}
		}
		break;
	}
	ZEROOUT_x64_INSTR();
	x64EncodeAluEI(x64instrs + (instr_cnt++), x64_ALU_OP_ID_CMP, x64oprs[0], x64oprs_tmp[0], I0OprISize[i0instr->attr]);
	ZEROOUT_x64_INSTR();
	x64EncodeJmpCcRel32(x64instrs + (instr_cnt++), tttn, 12);
	Writex64Instrs(x64instrs,instr_cnt, nativeblock, nativelimit, is_write);
	x64EncodeCallAbsImm64(nativeblock, nativelimit, (i0instr->opr[2].val.v64), is_write);
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH, I0_DECODE_JCC, (*nativelimit) - 12);
}

DECODE_STATUS TranslateBCMP(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write) {
	x64_OPR x64oprs[2];
	x64_OPR x64oprs_tmp[3];
	x64INSTR x64instrs[10];
	uint32_t instr_cnt = 0;
	I0OPR* i0_opr0 = (&(i0instr->opr[0]));
	I0OPR* i0_opr1 = (&(i0instr->opr[1]));

	uint8_t tttn;

	x64_OPR* x64_opr0 = (&(x64oprs[0]));
	x64_OPR* x64_opr1 = (&(x64oprs[1]));
	x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
	x64oprs_tmp[0].reg = x64_AX;

	int opr_encoded_reg[2] = {0,0};

	int* opr0_encoded_reg = (&(opr_encoded_reg[0]));
	int* opr1_encoded_reg = (&(opr_encoded_reg[1]));

	int is_less = 0;
	int is_equal = 0;

	int is_jmp = 0;

	ENCODE_OPR((*i0_opr0), (*x64_opr0), (*opr0_encoded_reg));
	ENCODE_OPR((*i0_opr1), (*x64_opr1), (*opr1_encoded_reg));

	switch (i0instr->option) {
	case OPT_B_L:
		switch (i0instr->attr) {
		case ATTR_UB:
		case ATTR_UF:
		case ATTR_UE:
			tttn = x64_TTTN_NB;
			break;
		case ATTR_SB:
		case ATTR_SF:
		case ATTR_SE:
			tttn = x64_TTTN_NL;
			break;
		}
		break;
	case OPT_B_LE:
		switch (i0instr->attr) {
		case ATTR_UB:
		case ATTR_UF:
		case ATTR_UE:
			tttn = x64_TTTN_NBE;
			break;
		case ATTR_SB:
		case ATTR_SF:
		case ATTR_SE:
			tttn = x64_TTTN_NLE;
			break;
		}
		break;
	case OPT_B_E:
		tttn = x64_TTTN_NZ;
		break;
	case OPT_B_NE:
		tttn = x64_TTTN_Z;
		break;
	case OPT_B_SL:
		error("opt_b_sl not implemented!");
		break;
	}

	if (((i0_opr0->addrm)<(i0_opr1->addrm)) || ((x64_opr0->type) < (x64_opr1->type))) {
		x64_opr0 = (&(x64oprs[1]));
		x64_opr1 = (&(x64oprs[0]));
		x64_opr0 = (&(x64oprs[1]));
		x64_opr1 = (&(x64oprs[0]));
		opr0_encoded_reg = (&(opr_encoded_reg[1]));
		opr1_encoded_reg = (&(opr_encoded_reg[0]));
		if ((tttn != x64_TTTN_Z) && (tttn != x64_TTTN_NZ)) {
			tttn ^= 1;
		}
	}
	if ((x64_opr0->type) == x64_OPR_I) {
		switch (i0instr->attr) {
		case ATTR_SB:
			is_less = (((int8_t) (x64_opr0->imm.v8)) < ((int8_t) (x64_opr1->imm.v8)));
			is_equal = (((int8_t) (x64_opr0->imm.v8)) == ((int8_t) (x64_opr1->imm.v8)));
			break;
		case ATTR_UB:
			is_less = (((uint8_t) (x64_opr0->imm.v8)) < ((uint8_t) (x64_opr1->imm.v8)));
			is_equal = (((uint8_t) (x64_opr0->imm.v8)) == ((uint8_t) (x64_opr1->imm.v8)));
			break;
		case ATTR_SF:
			is_less = (((int32_t) (x64_opr0->imm.v32)) < ((int32_t) (x64_opr1->imm.v32)));
			is_equal = (((int32_t) (x64_opr0->imm.v32)) == ((int32_t) (x64_opr1->imm.v32)));
			break;
		case ATTR_UF:
			is_less = (((uint32_t) (x64_opr0->imm.v32)) < ((uint32_t) (x64_opr1->imm.v32)));
			is_equal = (((uint32_t) (x64_opr0->imm.v32)) == ((uint32_t) (x64_opr1->imm.v32)));
			break;
		case ATTR_SE:
			is_less = (((int64_t) (x64_opr0->imm.v64)) < ((int64_t) (x64_opr1->imm.v64)));
			is_equal = (((int64_t) (x64_opr0->imm.v64)) == ((int64_t) (x64_opr1->imm.v64)));
			break;
		case ATTR_UE:
			is_less = (((uint64_t) (x64_opr0->imm.v64)) < ((uint64_t) (x64_opr1->imm.v64)));
			is_equal = (((uint64_t) (x64_opr0->imm.v64)) == ((uint64_t) (x64_opr1->imm.v64)));
			break;
		}
		switch (i0instr->option) {
		case OPT_B_LE:
			if (is_equal) {
				is_jmp = 1;
				break;
			}
			//fall through
		case OPT_B_L:
			if (is_less) {
				is_jmp = 1;
			}
			break;
		case OPT_B_E:
			is_jmp = is_equal;
			break;
		case OPT_B_NE:
			is_jmp = (!is_equal);
			break;
		}
		if (is_jmp) {
			x64EncodeCallAbsImm64(nativeblock, nativelimit, (i0instr->opr[2].val.v64), is_write);
			RETURN_DECODE_STATUS(I0_DECODE_BRANCH, I0_DECODE_JMP, (*nativelimit) - 12);
		} else {
			RETURN_DECODE_STATUS(I0_TRANS_SUCCESS_NO_FURTHER_PROC, 0, 0);
		}
	}

	if(!(*opr0_encoded_reg))
	{
		ZEROOUT_x64_INSTR();
		switch(i0_opr0->addrm)
		{
		case ADDRM_ABSOLUTE:
			if((x64_opr1->type) != x64_OPR_I)
			{
				x64oprs_tmp[1].type = x64_OPR_I;
				x64oprs_tmp[1].imm.v64 = (i0_opr0->val.v64);
				x64EncodeMovGI(x64instrs + (instr_cnt++), x64oprs_tmp[0], x64oprs_tmp[1], TYPE_ID_QWORD);
				break;
			}
			(x64_opr1->type) = x64_OPR_TYPE_REG;
		default:
			x64EncodeMovMI64ToAX(x64instrs + (instr_cnt++), (i0_opr0->val.v64), TYPE_ID_QWORD);
			break;
		}
		(x64_opr0->reg) = x64_AX;
	}
	x64oprs_tmp[2].type = x64_OPR_TYPE_REG;
	x64oprs_tmp[2].reg = x64_DX;
	switch(x64_opr1->type)
	{
	case x64_OPR_I:
		if ((I0OprISize[i0instr->attr] == TYPE_ID_QWORD) && (!(WITHIN64_32BIT(x64_opr1->imm.v64))))
		{
			ZEROOUT_x64_INSTR();
			x64EncodeMovGI(x64instrs + (instr_cnt++), x64oprs_tmp[2], (*x64_opr1), I0OprISize[i0instr->attr]);
			(x64_opr1->type) = x64_OPR_TYPE_REG;
			(x64_opr1->reg) = x64_DX;
		}
		break;
	case x64_OPR_TYPE_M:
		ZEROOUT_x64_INSTR();
		x64EncodeMovGE(x64instrs + (instr_cnt++), x64oprs_tmp[2], (*x64_opr0), I0OprISize[i0instr->attr]);
		(x64_opr0->type) = x64_OPR_TYPE_REG;
		(x64_opr0->reg) = x64_DX;
		if(!(*opr1_encoded_reg))
		{
			ZEROOUT_x64_INSTR();
			if((i0_opr1->addrm) == ADDRM_ABSOLUTE)
			{
				x64oprs_tmp[1].type = x64_OPR_I;
				x64oprs_tmp[1].imm.v64 = (i0_opr1->val.v64);
				x64EncodeMovGI(x64instrs + (instr_cnt++), x64oprs_tmp[0], x64oprs_tmp[1], TYPE_ID_QWORD);
			}
			else
			{
				x64EncodeMovMI64ToAX(x64instrs + (instr_cnt++), (i0_opr1->val.v64), TYPE_ID_QWORD);
			}
			(x64_opr1->reg) = x64_AX;
		}
		break;
	}
	ZEROOUT_x64_INSTR();
	x64EncodeAlu(x64instrs+(instr_cnt++), x64_ALU_OP_ID_CMP, (*x64_opr0), (*x64_opr1), I0OprISize[i0instr->attr]);
	ZEROOUT_x64_INSTR();
	x64EncodeJmpCcRel32(x64instrs+(instr_cnt++), tttn, 12);
	Writex64Instrs(x64instrs, instr_cnt, nativeblock, nativelimit, is_write);
	x64EncodeCallAbsImm64(nativeblock, nativelimit, i0instr->opr[2].val.v64, is_write);
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH, I0_DECODE_JCC, (*nativelimit) -12);
}

DECODE_STATUS TranslateNOP(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	instr_t __instr;
	translate2x86_64_nop((&__instr), is_write, ((char*)tpc), nativelimit, 0, 0);
	RETURN_DECODE_STATUS(I0_TRANS_SUCCESS_NO_FURTHER_PROC,0,0);
}

DECODE_STATUS TranslateCONV(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	instr_t __instr;
	__instr.addr = (instr->addr);
	__instr.mattr1 = (instr->attr);
	__instr.mattr2 = (instr->attr2);
	__instr.addrm1 = (instr->opr[0].addrm);
	__instr.opr1 = (instr->opr[0].val.v64);
	__instr.disp1 = (instr->opr[0].disp32);
	__instr.addrm2 = (instr->opr[1].addrm);
	__instr.opr2 = (instr->opr[1].val.v64);
	__instr.disp2 = (instr->opr[1].disp32);
	translate2x86_64_mov((&__instr), is_write, ((char*)tpc), nativelimit, 0, 0);
	RETURN_DECODE_STATUS(I0_TRANS_SUCCESS_NO_FURTHER_PROC,0,0);
}

DECODE_STATUS TranslateALU(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	instr_t __instr;
	__instr.addr = (instr->addr);
	__instr.opcode = (instr->opcode);
	__instr.attr = (instr->attr);
	__instr.addrm1 = (instr->opr[0].addrm);
	__instr.opr1 = (instr->opr[0].val.v64);
	__instr.disp1 = (instr->opr[0].disp32);
	__instr.addrm2 = (instr->opr[1].addrm);
	__instr.opr2 = (instr->opr[1].val.v64);
	__instr.disp2 = (instr->opr[1].disp32);
	__instr.addrm3 = (instr->opr[2].addrm);
	__instr.opr3 = (instr->opr[2].val.v64);
	__instr.disp3 = (instr->opr[2].disp32);
	if((instr->opcode) == OP_DIV)
	{
		translate2x86_64_div((&__instr), is_write, ((char*)tpc), nativelimit, 0, 0);
	}
	else
	{
		translate2x86_64_alu_op((&__instr), is_write, ((char*)tpc), nativelimit, 0, 0);
	}
	RETURN_DECODE_STATUS(I0_TRANS_SUCCESS_NO_FURTHER_PROC,0,0);
}

/*DECODE_STATUS TranslateINT(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	instr_t __instr;
	__instr.opr1 =((uint64_t)(instr->opr[0].val.v8));
	translate2x86_64_int((&__instr), is_write, ((char*)tpc), nativelimit, 0, 0);
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH,I0_DECODE_INT,__instr.opr1);
}*/

DECODE_STATUS TranslateEXIT(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	instr_t __instr;
	__instr.option = (instr->option);
	translate2x86_64_exit((&__instr), is_write, ((char*)tpc), nativelimit, 0, 0);
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH,I0_DECODE_EXIT,(*nativelimit));
}

DECODE_STATUS TranslateSPAWN(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	instr_t __instr;
	__instr.addrm1 = (instr->opr[0].addrm);
	__instr.opr1 = (instr->opr[0].val.v64);
	__instr.disp1 = (instr->opr[0].disp32);
	__instr.addrm2 = (instr->opr[1].addrm);
	__instr.opr2 = (instr->opr[1].val.v64);
	__instr.disp2 = (instr->opr[1].disp32);
	__instr.addrm3 = (instr->opr[2].addrm);
	__instr.opr3 = (instr->opr[2].val.v64);
	__instr.disp3 = (instr->opr[2].disp32);
	__instr.addrm4 = (instr->opr[3].addrm);
	__instr.opr4 = (instr->opr[3].val.v64);
	__instr.disp4 = (instr->opr[3].disp32);
	translate2x86_64_spawn((&__instr), is_write, ((char*)tpc), nativelimit, 0, 0);
	RETURN_DECODE_STATUS(I0_TRANS_SUCCESS_NO_FURTHER_PROC,0,0);
}

DECODE_STATUS TranslateSHIFT(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	instr_t __instr;
	__instr.option = (instr->option);
	__instr.attr = (instr->attr);
	__instr.addrm1 = (instr->opr[0].addrm);
	__instr.opr1 = (instr->opr[0].val.v64);
	__instr.disp1 = (instr->opr[0].disp32);
	__instr.addrm2 = (instr->opr[1].addrm);
	__instr.opr2 = (instr->opr[1].val.v64);
	__instr.disp2 = (instr->opr[1].disp32);
	__instr.addrm3 = (instr->opr[2].addrm);
	__instr.opr3 = (instr->opr[2].val.v64);
	__instr.disp3 = (instr->opr[2].disp32);
	translate2x86_64_shift((&__instr), is_write, ((char*)tpc), nativelimit, 0, 0);
	RETURN_DECODE_STATUS(I0_TRANS_SUCCESS_NO_FURTHER_PROC,0,0);
}

DECODE_STATUS TranslateSCMP(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	instr_t __instr;
	__instr.addrms[0] = (instr->opr[0].addrm);
	__instr.oprs[0] = (instr->opr[0].val.v64);
	__instr.disps[0] = (instr->opr[0].disp32);
	__instr.addrms[1] = (instr->opr[1].addrm);
	__instr.oprs[1] = (instr->opr[1].val.v64);
	__instr.disps[1] = (instr->opr[1].disp32);
	__instr.addrms[2] = (instr->opr[2].addrm);
	__instr.oprs[2] = (instr->opr[2].val.v64);
	__instr.disps[2] = (instr->opr[2].disp32);
	__instr.addrms[3] = (instr->opr[3].addrm);
	__instr.oprs[3] = (instr->opr[3].val.v64);
	__instr.disps[3] = (instr->opr[3].disp32);
	__instr.addrms[4] = (instr->opr[4].addrm);
	__instr.oprs[4] = (instr->opr[4].val.v64);
	__instr.disps[4] = (instr->opr[4].disp32);
	translate2x86_64_strcmp((&__instr), is_write, ((char*)tpc), nativelimit, 0, 0);
	RETURN_DECODE_STATUS(I0_TRANS_SUCCESS_NO_FURTHER_PROC,0,0);
}


/*void TranslateAluOp(I0INSTR* instr, uint8_t** tpc, uint8_t op) {
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

*/



uint64_t run_i0_code2(uint64_t __tmp__)
{
	static uint8_t native_code_cache[1024];
	(void)__tmp__;
	uint8_t* tran_out = GetDisasmOutAddr();
	uint64_t tran_out_offset = 0;
	uint8_t* spc  = ((uint8_t*)(I0_CODE_BEGIN));
	uint8_t* i0_limit = ((uint8_t*)(I0_CODE_BEGIN + (_pi0codemeta->i0_code_file_len)));
	while(((uint64_t)spc) < ((uint64_t)i0_limit))
	{
		uint64_t nativelimit = 0;
		DECODE_STATUS decode_stat;
		decode_stat = TranslateI0ToNative(&spc, native_code_cache, &nativelimit, i0_limit, 1);
		switch(decode_stat.status)
		{
		case I0_DECODE_BRANCH:
			memcpy(tran_out+tran_out_offset, native_code_cache, nativelimit);
			FlushTransOutput();
			tran_out_offset += nativelimit;
			switch(decode_stat.detail2)
			{
			case I0_DECODE_INT:
				ModLog("branch int @ %lx\n", tran_out_offset);
				break;
			case I0_DECODE_EXIT:
				ModLog("branch exit @ %lx\n", tran_out_offset);
				break;
			case I0_DECODE_JMP:
				ModLog("branch jmp @ %lx\n", tran_out_offset);
				break;
			case I0_DECODE_JCC:
				ModLog("branch jcc @ %lx\n", tran_out_offset);
				break;
			case I0_DECODE_JMP_INDIR:
				ModLog("branch jindir @ %lx\n", tran_out_offset);
				break;
			}
			break;
		}
	}
	while(1)
	{}
}
