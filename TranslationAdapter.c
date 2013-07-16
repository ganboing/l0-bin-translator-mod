#include <execinfo.h>
#include "sys_config.h"
#include "codec.h"
#include "bin_translator.h"
#include "zlog.h"
#include "DecodeStatus.h"
#include "oldtrans.c"
//#include "I0Symbol.h"


int translate2x86_64_shift(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl);


#define RETURN_DECODE_STATUS(status, detail, detail2) \
	do{ DECODE_STATUS result= {(status), (detail), (detail2)}; return result;}while(0)

//int translate2x86_64_nop(instr_t *i, int is_write_n,char *n, uint64_t *nl, char *t, uint64_t *tl);
//int translate2x86_64_mov(instr_t *i, int is_write_n,char *n, uint64_t *nl, char *t, uint64_t *tl);
//int translate2x86_64_alu_op(instr_t *i, int is_write_n,char *n, uint64_t *nl, char *t, uint64_t *tl);
//int translate2x86_64_int(instr_t *i, int is_write_n,char *n, uint64_t *nl, char *t, uint64_t *tl);
//int translate2x86_64_exit(instr_t *i, int is_write_n,char *n, uint64_t *nl, char *t, uint64_t *tl);

//extern uint8_t TempNativeCodeStore[1024];

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

DECODE_STATUS TranslateINT(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	instr_t __instr;
	__instr.opr1 =((uint64_t)(instr->opr[0].val.v8));
	translate2x86_64_int((&__instr), is_write, ((char*)tpc), nativelimit, 0, 0);
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH,I0_DECODE_INT,__instr.opr1);
}

DECODE_STATUS TranslateEXIT(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	instr_t __instr;
	__instr.option = (instr->option);
	translate2x86_64_exit((&__instr), is_write, ((char*)tpc), nativelimit, 0, 0);
	RETURN_DECODE_STATUS(I0_DECODE_BRANCH,I0_DECODE_EXIT,__instr.option);
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

DECODE_STATUS TranslateBCMP(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{

}

DECODE_STATUS TranslateBZNZ(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	if(((instr->attr) == MATTR_FD) || ((instr->attr) == MATTR_FS))
	{
		error("jznz attr FD FS not implemented!");
	}
