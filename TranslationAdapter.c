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


DECODE_STATUS TranslateBCMP(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{

}

DECODE_STATUS TranslateBZNZ(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit, int is_write)
{
	if(((instr->attr) == MATTR_FD) || ((instr->attr) == MATTR_FS))
	{
		error("jznz attr FD FS not implemented!");
	}
