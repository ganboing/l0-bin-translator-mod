#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "PrintDisasm.h"
#include "zlog_mod.h"
#include "DecodeStatus.h"
#include "I0Types.h"
#include "I0Symbol.h"
#include "sys_config.h"
#include "x64Encode.c"

#define ENCODE_OPR(i0opr,x64opr,reg_encoded) \
	do{\
		printf("opr: addrm== %x val== %lx disp32==%x", (i0opr).addrm, (i0opr).val.v64, (i0opr).disp32);\
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
				printf("opr is in reg file %x\n",(unsigned)(x64opr).reg);\
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
				printf("opr is in reg file %x\n",(unsigned)(x64opr).reg);\
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
				printf("opr is in reg file %x\n",(unsigned)(x64opr).reg);\
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

DECODE_STATUS TranslateBIJ(I0INSTR* i0instr, uint8_t* nativeblock, uint64_t* nativelimit, int is_write)
{
	//ModLog("bij: opr val.v64 == %lx addrm == %x disp = %x\n",i0instr->opr[0].val.v64, i0instr->opr[0].addrm, i0instr->opr[0].disp32);
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
	x64oprs_tmp[1].type = x64_OPR_TYPE_REG;
	x64oprs_tmp[1].reg = x64_SI;
	uint32_t opr_encoded_reg[1] = {0};
	ENCODE_OPR((*i0_opr), x64oprs[0], opr_encoded_reg[0]);
	do{
		if(!(opr_encoded_reg[0]))
		{
			x64EncodeMovMI64ToAX(x64instrs+(instr_cnt++), (i0_opr->val.v64), TYPE_LEN_QWORD);
			x64oprs[0].reg = x64_AX;
			if((i0_opr->addrm) == ADDRM_ABSOLUTE)
			{
				x64oprs[0].type = x64_OPR_TYPE_REG;
				break;
			}
		}
		x64EncodeMovGE(x64instrs+(instr_cnt++), x64oprs_tmp[0], x64oprs[0], TYPE_LEN_QWORD);
	}while(0);
	x64EncodeMovEG(x64instrs+(instr_cnt++), x64oprs_tmp[1], x64oprs_tmp[0], TYPE_LEN_QWORD);
	Writex64Instrs(x64instrs, instr_cnt, nativeblock, nativelimit, is_write);
	if(is_write)
	{
		memcpy(nativeblock+(*nativelimit), and_eax_opcode, 1);
		(*nativeblock) += 1;
		(*((uint32_t*)(nativeblock+(*nativelimit)))) = ( 0x40000 - 1);
		(*nativeblock) += 4;
		memcpy(nativeblock+(*nativelimit),shl_eax_3_lea_3rax_op, 7);
		(*nativeblock) += 7;
		(*((uint32_t*)(nativeblock+(*nativelimit)))) = ((uint32_t) ((uint64_t)0x12345678abcdabcd) );
		(*nativeblock) += 4;
		memcpy(nativeblock+(*nativelimit), rest1, 11);
		(*nativeblock) += 11;
		(*((uint32_t*)(nativeblock+(*nativelimit)))) = ((uint32_t) ((uint64_t)0x56781234dcbadcba) );
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

int main()
{
	struct stat file_detail;
	static const uint64_t suggest_map_addr = 0x400000000;
	static void* real_map_addr;

	uint8_t* output_addr = GetDisasmOutAddr();

	int i0_prog_file = open("/tmp/i0_prog.out", O_RDONLY);
	if(i0_prog_file == -1)
	{
		exit(-100);
	}
	unsigned long filesize;
	if(fstat(i0_prog_file, &file_detail)!=0)
	{
		exit(-200);
	}
	filesize = ((unsigned long)file_detail.st_size);
	real_map_addr = mmap((void*)suggest_map_addr, filesize, PROT_READ, MAP_SHARED, i0_prog_file, 0);
	if(real_map_addr != ((void*)-1))
	{
		printf("map success @ %lx\n", (uint64_t)real_map_addr);
	}
	else
	{
		exit(-300);
	}
	uint64_t nativelimit = 0;
	x64INSTR x64instrs[10];
	uint32_t instr_cnt = 0;
	
	/*ZEROOUT_x64_INSTR();
	x64EncodeJmpCcRel32(x64instrs+(instr_cnt++), x64_TTTN_BE,(uint32_t) 0x1234abcdef);
	ZEROOUT_x64_INSTR();
	x64EncodeJmpCcRel8(x64instrs+(instr_cnt++), x64_TTTN_Z, 127);
	Writex64Instrs(x64instrs, instr_cnt, output_addr, &nativelimit, 1);
	x64EncodeCallAbsImm64(output_addr, &nativelimit, 0x1234abcdef1234, 1);
	x64_OPR x64_opr1;
	x64_opr1.type = x64_OPR_TYPE_REG;
	x64_opr1.reg = x64_SI;
	x64_OPR x64_opr2;
	x64_opr2.type = x64_OPR_TYPE_REG;
	x64_opr2.reg = x64_AX;
	instr_cnt = 0;
	ZEROOUT_x64_INSTR();
	x64EncodeMovGE(x64instrs+(instr_cnt++), x64_opr2, x64_opr1, TYPE_LEN_QWORD);
	*/
	//Writex64Instrs(x64instrs, instr_cnt, output_addr, &nativelimit, 1);
	
	I0INSTR i0_instr;
	i0_instr.opr[0].addrm = ADDRM_ABSOLUTE;
	i0_instr.opr[0].val.v64 = 0x200000018ULL;
	i0_instr.opr[0].disp32 = 0;
	TranslateBIJ(&i0_instr, output_addr, &nativelimit, 1);
	FlushTransOutput();
	//run_i0_code(0);
	return 0;
}
