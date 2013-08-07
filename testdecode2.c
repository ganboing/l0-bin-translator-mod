#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "I0Symbol.h"
#include "I0Types.h"
#include "sys_config.h"
#include "asm_func.h"
#include "ASM_MACROS.h"
#include "DecodeI0.h"
#include "zlog_mod.h"
#include "PrintDisasm.h"
#include "x64Encode.h"
#include "x64Encode.c"

#define NEXT_INS_FROM_STK(x64_ins_stk) \
	(((x64_ins_stk).x64instrs) + ((x64_ins_stk).sp++))

#define NEW_INS_STK()\
	{.sp = 0}

static uint8_t SwapOprTTTN[16]=
{
	0xff,			//x64_TTTN_O currently do not care
	0xff,			//x64_TTTN_NO currently do not care
	x64_TTTN_NBE,	//x64_TTTN_B
	x64_TTTN_BE,	//x64_TTTN_NB
	x64_TTTN_Z,		//x64_TTTN_Z
	x64_TTTN_NZ,	//x64_TTTN_NZ
	x64_TTTN_NB,	//x64_TTTN_BE
	x64_TTTN_B,		//x64_TTTN_NBE
	0xff,			//x64_TTTN_S currently do not care
	0xff,			//x64_TTTN_NS currently do not care
	0xff,			//x64_TTTN_P currently do not care
	0xff,			//x64_TTTN_NP currently do not care
	x64_TTTN_NLE,	//x64_TTTN_L
	x64_TTTN_LE,	//x64_TTTN_NL
	x64_TTTN_NL,	//x64_TTTN_LE
	x64_TTTN_L		//x64_TTTN_NLE
};

//I0OprISize[attr]
static uint8_t I0OprISize[0x10]=
{
	TYPE_LEN_BYTE,
	TYPE_LEN_QWORD,
	0x00,
	TYPE_LEN_DWORD,
	TYPE_LEN_BYTE,
	TYPE_LEN_QWORD,
	0x00,
	TYPE_LEN_DWORD,
	TYPE_LEN_DWORD,
	TYPE_LEN_QWORD,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00
};

//I0OprMSize[addrm]
static uint8_t I0OprMSize[0x08]=
{
	0x00, 
	TYPE_LEN_QWORD, 
	TYPE_LEN_QWORD, 
	(TYPE_LEN_QWORD + TYPE_LEN_DWORD),
	0x00, 
	0x00, 
	0x00, 
	0x00
};

//I0OprDSize[attr][addrm]
static uint8_t I0OprDSize[0x10][0x08]=
{
	{TYPE_LEN_BYTE, TYPE_LEN_QWORD, TYPE_LEN_QWORD, (TYPE_LEN_QWORD + TYPE_LEN_DWORD), 0x00, 0x00, 0x00, 0x00},
	{TYPE_LEN_QWORD, TYPE_LEN_QWORD, TYPE_LEN_QWORD, (TYPE_LEN_QWORD + TYPE_LEN_DWORD), 0x00, 0x00, 0x00, 0x00},

	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

	{TYPE_LEN_DWORD, TYPE_LEN_QWORD, TYPE_LEN_QWORD, (TYPE_LEN_QWORD + TYPE_LEN_DWORD), 0x00, 0x00, 0x00, 0x00},
	{TYPE_LEN_BYTE, TYPE_LEN_QWORD, TYPE_LEN_QWORD, (TYPE_LEN_QWORD + TYPE_LEN_DWORD), 0x00, 0x00, 0x00, 0x00},
	{TYPE_LEN_QWORD, TYPE_LEN_QWORD, TYPE_LEN_QWORD, (TYPE_LEN_QWORD + TYPE_LEN_DWORD), 0x00, 0x00, 0x00, 0x00},

	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

	{TYPE_LEN_DWORD, TYPE_LEN_QWORD, TYPE_LEN_QWORD, (TYPE_LEN_QWORD + TYPE_LEN_DWORD), 0x00, 0x00, 0x00, 0x00},
	{TYPE_LEN_DWORD, TYPE_LEN_QWORD, TYPE_LEN_QWORD, (TYPE_LEN_QWORD + TYPE_LEN_DWORD), 0x00, 0x00, 0x00, 0x00},
	{TYPE_LEN_QWORD, TYPE_LEN_QWORD, TYPE_LEN_QWORD, (TYPE_LEN_QWORD + TYPE_LEN_DWORD), 0x00, 0x00, 0x00, 0x00},

	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};


unsigned long IndirJmpHashTab[1];

void error(char*)
#ifndef MSVC
	__attribute__((noreturn))
#endif
	;

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
	x64_REG_BX,
	x64_REG_CX,
	x64_REG_R15,
	x64_REG_R14,
	x64_REG_R13,
	x64_REG_R10,
	x64_REG_R9,
	x64_REG_R8
};

static const x64_OPR x64_reg_ax = {.type=x64_OPR_TYPE_REG, .reg=x64_REG_AX };
static const x64_OPR x64_reg_dx = {.type=x64_OPR_TYPE_REG, .reg=x64_REG_DX };
static const x64_OPR x64_reg_si = {.type=x64_OPR_TYPE_REG, .reg=x64_REG_SI };
static const x64_OPR x64_reg_di = {.type=x64_OPR_TYPE_REG, .reg=x64_REG_DI };

__inline static int IS_I0_REGISTER_FILE(uint64_t i0addr)
{
	if(i0addr>=(REG_FILE_BEGIN))
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


#define ENCODE_OPR(i0opr,x64opr) \
	do{\
		switch((i0opr).addrm)\
		{\
		case ADDRM_IMMEDIATE:\
			(x64opr).type = x64_OPR_I;\
			(x64opr).imm.v64 = (i0opr).val.v64;\
			(x64opr).full_encoded = 1;\
			break;\
		case ADDRM_ABSOLUTE:\
			if(IS_I0_REGISTER_FILE((i0opr).val.v64))\
			{\
				(x64opr).reg=MAP_TO_NATIVE_REGISTER((i0opr).val.v64);\
				(x64opr).type = x64_OPR_TYPE_REG;\
				(x64opr).full_encoded = 1;\
			}\
			else\
			{\
				(x64opr).type = x64_OPR_TYPE_M;\
				(x64opr).off32 = 0;\
				(x64opr).full_encoded = 0;\
			}\
			break;\
		case ADDRM_DISPLACEMENT:\
			(x64opr).type = x64_OPR_TYPE_M;\
			if(IS_I0_REGISTER_FILE((i0opr).val.v64))\
			{\
				(x64opr).reg=MAP_TO_NATIVE_REGISTER((i0opr).val.v64);\
				(x64opr).full_encoded = 1;\
			}\
			else\
			{\
				(x64opr).full_encoded = 0;\
			}\
			(x64opr).off32 = (i0opr).disp32;\
			break;\
		case ADDRM_INDIRECT:\
			(x64opr).type = x64_OPR_TYPE_M;\
			if(IS_I0_REGISTER_FILE((i0opr).val.v64))\
			{\
				(x64opr).reg=MAP_TO_NATIVE_REGISTER((i0opr).val.v64);\
				(x64opr).full_encoded = 1;\
			}\
			else\
			{\
				(x64opr).full_encoded = 0;\
			}\
			(x64opr).off32 = 0;\
			break;\
		}\
	}while(0)

inline static int IS_I0_OPR_EQUAL(I0OPR* opr1, I0OPR* opr2)
{
	if(opr1->addrm == opr2->addrm)
	{
		switch(opr1->addrm)
		{
		case ADDRM_DISPLACEMENT:
			if( (opr1->disp32) != (opr2->disp32))
			{
				return 0;
			}
			//fall through
		case ADDRM_IMMEDIATE:
		case ADDRM_ABSOLUTE:
		case ADDRM_INDIRECT:
			return ((opr1->val.v64) == (opr2->val.v64));
			break;
		}
	}
	return 0;
}

DECODE_STATUS TranslateINT(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write)
{
	x64INSTR x64instrs[5];
	uint32_t instr_cnt = 0;
	x64_OPR idt_base;
	x64_OPR selector;
	idt_base.type = x64_OPR_I;
	idt_base.imm.v64 = IDT_BASE;
	selector.type = x64_OPR_I;
	selector.imm.v64 = (i0instr->opr[0].val.v64);
	x64EncodeMovGI(x64instrs+(instr_cnt++), x64_reg_dx,idt_base,TYPE_LEN_QWORD);
	x64EncodeMovGI(x64instrs+(instr_cnt++), x64_reg_ax,selector,TYPE_LEN_QWORD);
	x64instrs[instr_cnt].opcode_len = 1;
	x64instrs[instr_cnt].opcode[0] = 0xff;
	x64instrs[instr_cnt].ModRM_SIB_len = 2;
	x64instrs[instr_cnt].ModRM_SIB[0] = 0x14;
	x64instrs[instr_cnt].ModRM_SIB[1] = 0xc2;
	//callq *(%rdx, %rax, 8)
	instr_cnt++;
	Writex64Instrs(x64instrs, instr_cnt, nativeblock, nativelimit, is_write);
	RETURN_DECODE_STATUS(I0_TRANS_SUCCESS_NO_FURTHER_PROC, 0, 0);
}

//typedef struct DECODE_STATUS ;
DECODE_STATUS TranslateBJ(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write)
{
	x64EncodeCallAbsImm64(nativeblock, nativelimit, (i0instr->opr[0].val.v64), is_write);
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH_UNCOND, I0_DECODE_JMP, (*nativelimit) -12);
}

DECODE_STATUS TranslateBIJ(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write)
{
	ModLog("bij: opr val.v64 == %lx addrm == %x disp = %x\n",i0instr->opr[0].val.v64, i0instr->opr[0].addrm, i0instr->opr[0].disp32);
	//25 ?? ?? ?? ??		andl $0x(cnt_of_ijtab-1), %eax 
	//c1 e0 03				shll $0x03, %eax
	//48 8d bc 40 ??x4		leaq ijtab_addr(%rax, %rax, 2), %rdi
	//48 3b 37				cmpq (%rdi), %rsi
	//75 05					jne .+3
	//ff 67 08				jmpq *0x08(%rdi)
	//b8 ??x4				movl hash_tab_miss_handler, %eax
	//ff e0					jmpq *%rax
	static const uint8_t and_eax_opcode[1] = {0x25};
	static const uint8_t shl_eax_3_lea_3rax_op[7] = {0xc1, 0xe0, 0x03, 0x48, 0x8d, 0xbc, 0x40};
	static const uint8_t rest1[9] = {0x48, 0x3b, 0x37, 0x75, 0x03,
		0xff, 0x67, 0x08, 0xb8};
	static const uint8_t jmpq_rax[2] = {0xff, 0xe0};
	//read dest addr to %rax
	I0OPR* i0_opr = (&(i0instr->opr[0]));
	x64INSTR_ST x64ins_st = NEW_INS_STK();
	x64_OPR x64opr0;
	ENCODE_OPR((*i0_opr), x64opr0);
	do{
		if(!(x64opr0.full_encoded))
		{
			x64EncodeMovMI64ToAX(NEXT_INS_FROM_STK(x64ins_st), (i0_opr->val.v64), TYPE_LEN_QWORD);
			x64opr0.reg = x64_REG_AX;
			if((i0_opr->addrm) == ADDRM_ABSOLUTE)
			{
				x64opr0.type = x64_OPR_TYPE_REG;
				break;
			}
		}
		x64EncodeMovGE(NEXT_INS_FROM_STK(x64ins_st), x64_reg_ax, x64opr0, TYPE_LEN_QWORD);
	}while(0);
	x64EncodeMovGE(NEXT_INS_FROM_STK(x64ins_st), x64_reg_si, x64_reg_ax, TYPE_LEN_QWORD);
	Writex64InsStk((&x64ins_st), nativeblock, nativelimit, is_write);
	if(is_write)
	{
		memcpy(nativeblock+(*nativelimit), and_eax_opcode, 1);
		(*nativelimit) += 1;
		(*((uint32_t*)(nativeblock+(*nativelimit)))) = ( 0x40000 - 1);
		(*nativelimit) += 4;
		memcpy(nativeblock+(*nativelimit),shl_eax_3_lea_3rax_op, 7);
		(*nativelimit) += 7;
		(*((uint32_t*)(nativeblock+(*nativelimit)))) = ((uint32_t) ((uint64_t)0x12345678abcdabcd) );
		(*nativelimit) += 4;
		memcpy(nativeblock+(*nativelimit), rest1, 9);
		(*nativelimit) += 9;
		(*((uint32_t*)(nativeblock+(*nativelimit)))) = ((uint32_t) ((uint64_t)0x56781234dcbadcba) );
		(*nativelimit) += 4;
		memcpy(nativeblock+(*nativelimit), jmpq_rax, 2);
		(*nativelimit) += 2;
	}
	else
	{
		(*nativelimit) += (5+3+4+4+3+5+7);
	}
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH_UNCOND, I0_DECODE_JMP_INDIR, (*nativelimit));
}

DECODE_STATUS TranslateBZNZ(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write)
{
	x64INSTR_ST x64ins_st = NEW_INS_STK();
	x64_OPR x64_opr0;
	x64_OPR x64_opr_z;
	ENCODE_OPR((i0instr->opr[0]), x64_opr0);
	x64_opr_z.type = x64_OPR_I;
	x64_opr_z.imm.v64 = 0;
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
	switch(x64_opr0.type)
	{
	case x64_OPR_I:
		switch(I0OprISize[i0instr->attr])
		{
		case TYPE_LEN_BYTE:
			cmp_imm_z = ((x64_opr0.imm.v8) == 0);
			break;
		case TYPE_LEN_DWORD:
			cmp_imm_z = ((x64_opr0.imm.v32) == 0);
			break;
		case TYPE_LEN_QWORD:
			cmp_imm_z = ((x64_opr0.imm.v64) == 0);
			break;
		}
		if((cmp_imm_z && ((i0instr->option) == OPT_B_Z)) || 
			((!cmp_imm_z) && ((i0instr->option) == OPT_B_NZ)))
		{
			x64EncodeCallAbsImm64(nativeblock, nativelimit,(i0instr->opr[2].val.v64),is_write);
			RETURN_DECODE_STATUS(I0_DECODE_BRANCH_UNCOND,I0_DECODE_JMP,(*nativelimit)-12);
		}
		else
		{
			RETURN_DECODE_STATUS(I0_TRANS_SUCCESS_NO_FURTHER_PROC,0,0);
		}
		break;
	case x64_OPR_TYPE_M:
		if(!(x64_opr0.full_encoded))
		{
			x64EncodeMovMI64ToAX(NEXT_INS_FROM_STK(x64ins_st),(i0instr->opr[0].val.v64), I0OprISize[i0instr->attr]);	
			x64_opr0.reg = x64_REG_AX;
			if((i0instr->opr[0].addrm) == ADDRM_ABSOLUTE)
			{
				x64_opr0.type = x64_OPR_TYPE_REG;
			}
		}
		break;
	}
	x64EncodeAluEI(NEXT_INS_FROM_STK(x64ins_st), x64_ALU_OP_ID_CMP, x64_opr0, x64_opr_z, I0OprISize[i0instr->attr]);
	x64EncodeJmpCcRel8(NEXT_INS_FROM_STK(x64ins_st), tttn, 12);
	Writex64InsStk((&x64ins_st), nativeblock, nativelimit, is_write);
	x64EncodeCallAbsImm64(nativeblock, nativelimit, (i0instr->opr[2].val.v64), is_write);
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH_COND, I0_DECODE_JCC, (*nativelimit) - 14);
}

DECODE_STATUS TranslateBCMP(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write) {
	x64_OPR x64oprs[2];
	x64_OPR x64oprs_tmp[3];
	x64INSTR_ST x64ins_st = NEW_INS_STK();
	I0OPR* i0_opr0 = (&(i0instr->opr[0]));
	I0OPR* i0_opr1 = (&(i0instr->opr[1]));

	uint8_t tttn = 0;

	x64_OPR* x64_opr0 = (&(x64oprs[0]));
	x64_OPR* x64_opr1 = (&(x64oprs[1]));
	
	x64oprs_tmp[0].type = x64_OPR_TYPE_REG;
	x64oprs_tmp[0].reg = x64_REG_AX;

	int is_less = 0;
	int is_equal = 0;

	int is_jmp = 0;

	ENCODE_OPR((*i0_opr0), (*x64_opr0));
	ENCODE_OPR((*i0_opr1), (*x64_opr1));

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
	default:
		error("opt_b_sl not implemented!");
		break;
	}

	if ((x64_opr0->type) == x64_OPR_I) {
		x64_opr0 = (&(x64oprs[1]));
		x64_opr1 = (&(x64oprs[0]));
		i0_opr0 = (&(i0instr->opr[1]));
		i0_opr1 = (&(i0instr->opr[0]));
		tttn = SwapOprTTTN[tttn];
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
			RETURN_DECODE_STATUS(I0_DECODE_BRANCH_UNCOND, I0_DECODE_JMP, (*nativelimit) - 12);
		} else {
			RETURN_DECODE_STATUS(I0_TRANS_SUCCESS_NO_FURTHER_PROC, 0, 0);
		}
	}
	int ax_used = 0;
	if(!(x64_opr0->full_encoded))
	{
		switch(i0_opr0->addrm)
		{
		case ADDRM_ABSOLUTE:
			(x64_opr1->type) = x64_OPR_TYPE_REG;
			//fall through!
		default:
			x64EncodeMovMI64ToAX(NEXT_INS_FROM_STK(x64ins_st), (i0_opr0->val.v64), TYPE_LEN_QWORD);
			break;
		}
		(x64_opr0->reg) = x64_REG_AX;
		ax_used = 1;
	}
	switch(x64_opr1->type)
	{
	case x64_OPR_I:
		if ((I0OprISize[i0instr->attr] == TYPE_LEN_QWORD) && (!(WITHIN64_32BIT(x64_opr1->imm.v64))))
		{
			x64EncodeMovGI(NEXT_INS_FROM_STK(x64ins_st), x64_reg_dx, (*x64_opr1), I0OprISize[i0instr->attr]);
			(*x64_opr1) = x64_reg_dx;
		}
		break;
	case x64_OPR_TYPE_M:
		if((x64_opr0->type) == x64_OPR_TYPE_M)
		{
			ax_used = 0;
			x64EncodeMovGE(NEXT_INS_FROM_STK(x64ins_st), x64_reg_dx, (*x64_opr0), I0OprISize[i0instr->attr]);
		}
		(*x64_opr0) = x64_reg_dx;
		if(!(x64_opr1->full_encoded))
		{
			if((i0_opr1->addrm) == ADDRM_ABSOLUTE)
			{
				x64_OPR tmp_i;
				tmp_i.type = x64_OPR_I;
				tmp_i.imm.v64 = (i0_opr1->val.v64);
				x64EncodeMovGI(NEXT_INS_FROM_STK(x64ins_st), x64_reg_di, tmp_i, TYPE_LEN_QWORD);
				(x64_opr1->reg) = x64_REG_DI;
			}
			else
			{
				if(ax_used)
				{
					x64EncodeMovEG(NEXT_INS_FROM_STK(x64ins_st), x64_reg_dx, x64_reg_ax, I0OprISize[i0instr->attr]);
					(*x64_opr0) = x64_reg_dx;
				}
				x64EncodeMovMI64ToAX(NEXT_INS_FROM_STK(x64ins_st), (i0_opr1->val.v64), TYPE_LEN_QWORD);
			}
			(x64_opr1->reg) = x64_REG_AX;
		}
		break;
	}
	x64EncodeAlu(NEXT_INS_FROM_STK(x64ins_st), x64_ALU_OP_ID_CMP, (*x64_opr0), (*x64_opr1), I0OprISize[i0instr->attr]);
	x64EncodeJmpCcRel8(NEXT_INS_FROM_STK(x64ins_st), tttn, 12);
	Writex64InsStk((&x64ins_st), nativeblock, nativelimit, is_write);
	x64EncodeCallAbsImm64(nativeblock, nativelimit, i0instr->opr[2].val.v64, is_write);
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH_COND, I0_DECODE_JCC, (*nativelimit) -14);
}

DECODE_STATUS TranslateNOP(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	(void)instr;
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

/*DECODE_STATUS TranslateALU(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
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
}*/

DECODE_STATUS TranslateEXIT(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	instr_t __instr;
	__instr.option = (instr->option);
	translate2x86_64_exit((&__instr), is_write, ((char*)tpc), nativelimit, 0, 0);
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH_UNCOND,I0_DECODE_EXIT,(*nativelimit));
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

DECODE_STATUS TranslateAluOp(I0INSTR* instr, uint8_t op , uint8_t* nativeblock, uint64_t* nativelimit, int is_write)
{
	x64_OPR x64oprs[3];
	unsigned int opr_swapped = 0;

	I0OPR* i0_opr0 = (&(instr->opr[0]));
	I0OPR* i0_opr1 = (&(instr->opr[1]));
	I0OPR* i0_opr2 = (&(instr->opr[2]));

	x64_OPR* x64_opr0 = (&(x64oprs[0]));
	x64_OPR* x64_opr1 = (&(x64oprs[1]));
	x64_OPR* x64_opr2 = (&(x64oprs[2]));

	x64INSTR_ST x64ins_st0 = NEW_INS_STK();
	x64INSTR_ST x64ins_st1 = NEW_INS_STK();
	x64INSTR_ST x64ins_st2 = NEW_INS_STK();
	x64INSTR_ST x64ins_st3 = NEW_INS_STK();

	ENCODE_OPR((*i0_opr2), (*x64_opr2));

	while(1)
	{
		if (IS_I0_OPR_EQUAL(i0_opr0, i0_opr2)) {
			ENCODE_OPR((*i0_opr1), (*x64_opr1));
			int ax_cob = 0;
			if (!(x64_opr2->full_encoded)) {
				switch (i0_opr2->addrm) {
				case ADDRM_ABSOLUTE: {
					x64_OPR opr_tmp;
					opr_tmp.type = x64_OPR_I;
					opr_tmp.imm.v64 = (i0_opr2->val.v64);
					x64EncodeMovGI(NEXT_INS_FROM_STK(x64ins_st2), x64_reg_di, opr_tmp, TYPE_LEN_QWORD);
					(x64_opr2->reg) = x64_REG_DI;
				}
					break;
				default:
					x64EncodeMovMI64ToAX(NEXT_INS_FROM_STK(x64ins_st2), (i0_opr2->val.v64), TYPE_LEN_QWORD);
					(x64_opr2->reg) = x64_REG_AX;
					ax_cob = 1;
					break;
				}
			}
			switch (x64_opr1->type) {
			case x64_OPR_I:
				if ((I0OprISize[instr->attr] == TYPE_LEN_QWORD) && (!(WITHIN64_32BIT(x64_opr1->imm.v64)))) {
					x64EncodeMovGI(x64ins_st1.x64instrs + (x64ins_st1.sp++), x64_reg_dx, (*x64_opr1), I0OprISize[instr->attr]);
					(*x64_opr1) = x64_reg_dx;
				}
				break;
			case x64_OPR_TYPE_M:
				if ((i0_opr1->addrm) == ADDRM_ABSOLUTE) {
					x64EncodeMovMI64ToAX(NEXT_INS_FROM_STK(x64ins_st1), (i0_opr1->val.v64), I0OprISize[instr->attr]);
					if (ax_cob) {
						x64EncodeMovGE(NEXT_INS_FROM_STK(x64ins_st1), x64_reg_dx, x64_reg_ax, I0OprISize[instr->attr]);
						(*x64_opr1) = x64_reg_dx;
					} else {
						(*x64_opr1) = x64_reg_ax;
					}
				} else {
					if (!(x64_opr1->full_encoded)) {
						x64EncodeMovMI64ToAX(NEXT_INS_FROM_STK(x64ins_st1), (i0_opr1->val.v64), TYPE_LEN_QWORD);
					}
					if ((x64_opr2->type) == x64_OPR_TYPE_M) {
						x64EncodeMovGE(NEXT_INS_FROM_STK(x64ins_st1), x64_reg_dx, (*x64_opr1), I0OprISize[instr->attr]);
						(*x64_opr1) = x64_reg_dx;
					}
				}
				break;
			}
			x64EncodeAlu(NEXT_INS_FROM_STK(x64ins_st2), op, (*x64_opr2), (*x64_opr1), I0OprISize[instr->attr]);
			if (opr_swapped && (op == x64_ALU_OP_ID_SUB)) {
				x64EncodeNegE(NEXT_INS_FROM_STK(x64ins_st2), (*x64_opr2), I0OprISize[instr->attr]);
			}
			break;
		} else {
			if (IS_I0_OPR_EQUAL(i0_opr1, i0_opr2)) {
				opr_swapped ^= 1;
				i0_opr0 = (&(instr->opr[1]));
				i0_opr1 = (&(instr->opr[0]));
			} else {
				x64_OPR dest_reg = x64_reg_dx;
				ENCODE_OPR((*i0_opr0), (*x64_opr0));
				ENCODE_OPR((*i0_opr1), (*x64_opr1));
				switch(x64_opr2->type)
				{
				case x64_OPR_TYPE_REG:
					dest_reg = (*x64_opr2);
					break;
				case x64_OPR_TYPE_M:
					if(!(x64_opr2->full_encoded))
					{
						if((i0_opr2->addrm) == ADDRM_ABSOLUTE)
						{
							dest_reg = x64_reg_ax;
							x64EncodeMovAXToMI64(NEXT_INS_FROM_STK(x64ins_st3), (i0_opr2->val.v64), I0OprISize[instr->attr]);
							break;
						}
						else
						{
							x64EncodeMovMI64ToAX(NEXT_INS_FROM_STK(x64ins_st3), (i0_opr2->val.v64), TYPE_LEN_QWORD);
							(x64_opr2->reg) = x64_REG_AX;
						}
					}
					x64EncodeMovEG(NEXT_INS_FROM_STK(x64ins_st3), (*x64_opr2), dest_reg, I0OprISize[instr->attr]);
					break;
				}
				if(!(x64_opr1->full_encoded))
				{
					x64_opr1->reg = x64_REG_AX;
				}
				if(op != x64_ALU_OP_ID_SUB)
				{
					if(((x64_opr0->type) == x64_OPR_I)  || (((x64_opr1->type) == x64_OPR_TYPE_M) && ((x64_opr1->reg) == (dest_reg.reg))) )
					{
						opr_swapped ^= 1;
						i0_opr0 = (&(instr->opr[1]));
						i0_opr1 = (&(instr->opr[0]));
						x64_opr0 = (&(x64oprs[1]));
						x64_opr1 = (&(x64oprs[0]));
					}
				}
				do{
					if(!(x64_opr1->full_encoded))
					{
						if((i0_opr1->addrm) == ADDRM_ABSOLUTE)
						{
							x64_OPR tmp_i;
							tmp_i.type = x64_OPR_I;
							tmp_i.imm.v64 = (i0_opr1->val.v64);
							x64EncodeMovGI(NEXT_INS_FROM_STK(x64ins_st0), x64_reg_di, tmp_i, TYPE_LEN_QWORD);
							(x64_opr1->reg) = x64_REG_DI;
							break;
						}
						else
						{
							x64_opr1->reg = x64_REG_AX;
						}
					}
					x64INSTR_ST* encode_ax_st = (&x64ins_st1);
					if(((x64_opr1->type) == x64_OPR_TYPE_M) && ((x64_opr1->reg) == (dest_reg.reg)))
					{
						x64_OPR reg_to_dup;
						reg_to_dup.type = x64_OPR_TYPE_REG;
						reg_to_dup.reg = (dest_reg.reg);
						x64EncodeMovEG(NEXT_INS_FROM_STK(x64ins_st1), x64_reg_di, reg_to_dup, TYPE_LEN_QWORD);
						(x64_opr1->reg) = x64_REG_DI;
						encode_ax_st = (&x64ins_st0);
					}
					if(!(x64_opr1->full_encoded))
					{
						x64EncodeMovMI64ToAX(NEXT_INS_FROM_STK(*encode_ax_st), (i0_opr1->val.v64), TYPE_LEN_QWORD);
					}
				}while(0);
				do{
					if(!(x64_opr0->full_encoded))
					{
						if((i0_opr0->addrm) == ADDRM_ABSOLUTE)
						{
							if(dest_reg.reg == x64_REG_AX)
							{
								x64EncodeMovMI64ToAX(NEXT_INS_FROM_STK(x64ins_st1), (i0_opr0->val.v64), I0OprISize[instr->attr]);
								break;
							}
							else
							{
								x64_OPR tmp;
								tmp.type = x64_OPR_I;
								tmp.imm.v64 = (i0_opr0->val.v64);
								x64EncodeMovGI(NEXT_INS_FROM_STK(x64ins_st1), x64_reg_si, tmp, TYPE_LEN_QWORD);
								(x64_opr0->reg) = x64_REG_SI;
							}
						}
						else
						{
							x64EncodeMovMI64ToAX(NEXT_INS_FROM_STK(x64ins_st1), (i0_opr0->val.v64), TYPE_LEN_QWORD);
							(x64_opr0->reg) = x64_REG_AX;
						}
					}
					x64EncodeMovGE(NEXT_INS_FROM_STK(x64ins_st1), dest_reg, (*x64_opr0), I0OprISize[instr->attr]);
				}while(0);
				x64EncodeAlu(NEXT_INS_FROM_STK(x64ins_st2), op, dest_reg, (*x64_opr1), I0OprISize[instr->attr]);
				break;
			}
		}
	}
	Writex64InsStk((&x64ins_st0), nativeblock, nativelimit, is_write);
	Writex64InsStk((&x64ins_st1), nativeblock, nativelimit, is_write);
	Writex64InsStk((&x64ins_st2), nativeblock, nativelimit, is_write);
	Writex64InsStk((&x64ins_st3), nativeblock, nativelimit, is_write);
	RETURN_DECODE_STATUS(I0_TRANS_SUCCESS_NO_FURTHER_PROC, 0, 0);
}

#ifdef MSVC
#define typeof(a) uint8_t*
#endif

#define GET_INST_OPR_I64(opr,t) \
	do{ \
		(opr).val.v64 = (*((uint64_t*)(t))); \
		(t) = (typeof(t))(((uint64_t*)(t))+1); \
	}while(0)

#define GET_INST_OPR_I8(opr,t) \
	do{ \
		(opr).val.v64 = (*((uint8_t*)(t))); \
		(t) = (typeof(t))(((uint8_t*)(t))+1); \
	}while(0)


/*#define DECODE_OPR_I(opr, spc, attr) \
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
	}while(0)*/

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
		(opr).disp32 = 0;\
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


DECODE_STATUS TranslateI0ToNative(uint8_t** spc, uint8_t* tpc, uint64_t* nativelimit, uint8_t* i0limit, unsigned int is_write) {
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
		i0instrlen += 1;
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

void AppendTailJump(uint8_t* nativeblock, uint64_t* nativelimit, uint64_t i0_addr, int is_write)
{
	x64EncodeCallAbsImm64(nativeblock, nativelimit, i0_addr, is_write);
}

void AppendUDHandler(uint8_t* nativeblock, uint64_t* nativelimit, uint64_t i0_addr, uint32_t func_ptr, int is_write)
{
	if (is_write) {
		(*((uint16_t*) (nativeblock + (*nativelimit)))) = 0xbf48;
		(*nativelimit) += 2;
		(*((uint64_t*) (nativeblock + (*nativelimit)))) = i0_addr;
		(*nativelimit) += 8;
		(*(nativeblock + (*nativelimit))) = 0xb8;
		(*nativelimit) ++;
		(*((uint32_t*) (nativeblock + (*nativelimit)))) = func_ptr;
		(*nativelimit) += 4;
		(*((uint16_t*) (nativeblock + (*nativelimit)))) = 0xe0ff;
		(*nativelimit) += 2;
	}
	else
	{
		(*nativelimit) += 2;
		(*nativelimit) += 8;
		(*nativelimit) ++;
		(*nativelimit) += 4;
		(*nativelimit) += 2;
	}
}

typedef struct _i0_code_meta_t {
    uint64_t i0_code_file_len;
} i0_code_meta_t;

extern i0_code_meta_t *_pi0codemeta;

/*uint64_t run_i0_code2(uint64_t __tmp__)
{
	static uint8_t native_code_cache[1024];
	(void)__tmp__;
	uint8_t* tran_out = GetDisasmOutAddr();
	uint64_t tran_out_offset = 0;
	uint8_t* spc  = ((uint8_t*)(I0_CODE_BEGIN));
	uint8_t* i0_limit = ((uint8_t*)(I0_CODE_BEGIN + (_pi0codemeta->i0_code_file_len)));
	while(((uint64_t)spc) < ((uint64_t)i0_limit))
	{
		//ModLog("translating i0 @ %lx\n", (uint64_t)spc);
		uint64_t spc_shadow = (uint64_t)spc;
		uint64_t nativelimit = 0;
		DECODE_STATUS decode_stat;
		decode_stat = TranslateI0ToNative(&spc, native_code_cache, &nativelimit, i0_limit, 1);
		switch(decode_stat.status)
		{
		case I0_DECODE_BRANCH:
			memcpy(tran_out+tran_out_offset, native_code_cache, nativelimit);
			FlushTransOutput();
			switch(decode_stat.detail)
			{
			case I0_DECODE_INT:
				ModLog("branch int @ %lx native @ %lx\n",spc_shadow, tran_out_offset);
				break;
			case I0_DECODE_EXIT:
				ModLog("branch exit @ %lx native @ %lx\n",spc_shadow,  tran_out_offset);
				break;
			case I0_DECODE_JMP:
				ModLog("branch jmp @ %lx native @ %lx\n",spc_shadow,  tran_out_offset);
				break;
			case I0_DECODE_JCC:
				ModLog("branch jcc @ %lx native @ %lx\n",spc_shadow,  tran_out_offset);
				break;
			case I0_DECODE_JMP_INDIR:
				ModLog("branch jindir @ %lx native @ %lx\n",spc_shadow,  tran_out_offset);
				break;
			default:
				ModLog("unexpected!\n");
				break;
			
			}
			tran_out_offset += nativelimit;
			break;
		}
	}
	while(1)
	{}
}*/
