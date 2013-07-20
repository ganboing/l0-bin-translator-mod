#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "PrintDisasm.h"
#include "zlog_mod.h"
#include "x64Encode.c"

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
	ZEROOUT_x64_INSTR();
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
	x64EncodeMovGE(x64instrs+(instr_cnt++), x64_opr2, x64_opr1, TYPE_LEN_QWORD);
	Writex64Instrs(x64instrs, instr_cnt, output_addr, &nativelimit, 1);
	FlushTransOutput();
	//run_i0_code(0);
	return 0;
}
