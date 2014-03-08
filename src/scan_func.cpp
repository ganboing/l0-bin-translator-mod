#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "scan_func.hpp"
#include "i0_mem_layout.h"

#define Op1 (opnds[0])
#define Op2 (opnds[1])
#define Op3 (opnds[2])
#define Op4 (opnds[3])
#define Op5 (opnds[4])

uint64_t i0_prog_size;

unsigned const i0_Ins_Opnd_Cnt[] = { 2, //conv
		3, //add
		3, //sub
		3, //mul
		3, //div
		3, //and
		3, //or
		3, //xor
		4, //spawn
		5, //spawnx
		0, //exit
		3, //bcc
		2, //bcz
		1, //bj
		1, //bij
		0, //nop
		1, //int
		3, //shl
		3, //shr
		4, //scmp
		2, //mov
		};
static_assert((qnumber(i0_Ins_Opnd_Cnt) == I0_ins_last_ins), "i0 ins opt number illegal");

//static_assert((sizeof(op_t) == (2+2+4+8)), "check op_t size");

static void i0_check_bound(i0_ea_type_t ea, size_t range)
{
	if(ea>= I0_MEMSPACE_PROGLOAD_BASE)
	{
		ea-=I0_MEMSPACE_PROGLOAD_BASE;
		if((ea + range)<=i0_prog_size)
		{
			return;
		}
	}
	throw(int(I0_DECODE_STATUS_TEXT_SEGMENT));
}

static uint8_t i0_get_byte(i0_ea_type_t ea) {
	i0_check_bound(ea, sizeof(uint8_t));
	return *((uint8_t*)ea);
}

static uint16_t i0_get_word(i0_ea_type_t ea) {
	i0_check_bound(ea, sizeof(uint16_t));
	return *((uint16_t*)ea);
}

static uint32_t i0_get_dword(i0_ea_type_t ea) {
	i0_check_bound(ea, sizeof(uint32_t));
	return *((uint32_t*)ea);
}

static uint64_t i0_get_qword(i0_ea_type_t ea) {
	i0_check_bound(ea, sizeof(uint64_t));
	return *((uint64_t*)ea);
}

i0_opcode4_t::i0_opcode4_t(insn_t& _cmd) :
		insn_ref(_cmd), shift_cnt(0) {
	load_op_bytes();
}

void i0_opcode4_t::load_op_bytes() {
	load_bytes(2);
}

void i0_opcode4_t::load_extra_bytes(uint8_t cnt) {
	load_bytes(cnt - 2);
}

void i0_opcode4_t::load_bytes(uint8_t cnt) {
	uint8_t load_cnt = insn_ref.ins_size + cnt;
	while (cnt) {
		opcode4.b[3 - load_cnt + cnt] = insn_ref.ins_fetch_byte();
		--cnt;
	}
}
uint32_t i0_opcode4_t::get_bits(unsigned start, unsigned len) {
	return ((opcode4.l << start) >> (32 - len));
}
uint32_t i0_opcode4_t::fetch_bits(unsigned len) {
	uint32_t ret = get_bits(shift_cnt, len);
	shift_cnt += len;
	return ret;
}

op_t::op_t() :
		addrm(-1), ins_offset(-1) {
}

void insn_t::check_oper(op_t& op) {
	check_oper(op, ins_attr);
}

static uint8_t i0_imm_bytelen[] = { 1, 8, 16, 4, 1, 8, 16, 4, 4, 8, -1, -1, -1,
		-1, -1, -1, };
static_assert((qnumber(i0_imm_bytelen) == (1<<I0_INS_BIT_LEN_ATTR)), "");

void insn_t::check_oper_C(op_t& op) {
	br_opnd = (&op - opnds);
	op.addrm = I0_ADDRM_IMMEDIATE;
	check_oper(op, I0_ATTR_UE);
}

void insn_t::check_oper_C_indir(op_t& op) {
	br_opnd = (&op - opnds);
	br_type |= I0_INS_BR_TARGET_UNDETERMINE;
	check_oper_M(op, I0_ATTR_UE);
	if (op.addrm == I0_ADDRM_INDIRECT) {
		if((*(uint64_t*)i0_op_raw_ptr(op)) == I0_MEMSPACE_STACK_POINTER)
		{
			br_type |= I0_INS_BR_RET;
		}
	}
}

void insn_t::check_oper(op_t& op, uint8_t attr) {
	op.ins_offset = ins_size;
	switch (op.addrm) {
	case I0_ADDRM_IMMEDIATE:
		switch (i0_imm_bytelen[attr]) {
		case 1:
			ins_check_byte();
			break;
		case 4:
			ins_check_dword();
			break;
		case 8:
			ins_check_qword();
			break;
		default:
			throw(int(I0_DECODE_STATUS_ATTR));
		}
		break;
	case I0_ADDRM_DISPLACEMENT:
		ins_check_dword();
	case I0_ADDRM_ABSOLUTE:
	case I0_ADDRM_INDIRECT:
		ins_check_qword();
		break;
	default:
		throw(int(I0_DECODE_STATUS_ADDRM));
	}
}

void insn_t::check_oper_M(op_t& op) {
	check_oper_M(op, ins_attr);
}

void insn_t::check_oper_M(op_t& op, uint8_t attr) {
	if (op.addrm == I0_ADDRM_IMMEDIATE) {
		throw(int(I0_DECODE_STATUS_ADDRM));
	}
	check_oper(op, attr);
}

void* insn_t::i0_op_raw_ptr(op_t& op) {
	uint8_t* p = (uint8_t*) ip;
	p += op.ins_offset;
	return (void*) p;
}

insn_t::insn_t(i0_ea_type_t _ip)
try :
		ip(_ip), op_name(I0_ins_last_ins), opt(i0_ins_opt_pref_last), ins_attr(
				i0_attr_last), __ins_extra_attr__(i0_attr_last), ins_size(0), br_type(
		I0_INS_BR_TYPE_FLOW_NXT) {
	i0_opcode4_t opcode4(*this);
	uint32_t opcode = opcode4.fetch_bits(I0_INS_BIT_LEN_OPCODE);
	switch (opcode) {
	case I0_OPCODE_CONV:
		op_name = I0_ins_conv;
		opcode4.load_extra_bytes(I0_INS_LEN_CONV);
		src_attr = opcode4.fetch_bits(I0_INS_BIT_LEN_ATTR);
		dest_attr = opcode4.fetch_bits(I0_INS_BIT_LEN_ATTR);
		if (src_attr == dest_attr) {
			op_name = I0_ins_mov;
			ins_attr = src_attr;
		}
		Op1.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op2.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		check_oper(Op1, src_attr);
		check_oper_M(Op2, dest_attr);
		return;
	case I0_OPCODE_ADD:
		op_name = I0_ins_add;
		break;
	case I0_OPCODE_SUB:
		op_name = I0_ins_sub;
		break;
	case I0_OPCODE_MUL:
		op_name = I0_ins_mul;
		break;
	case I0_OPCODE_DIV:
		op_name = I0_ins_div;
		break;
	case I0_OPCODE_AND:
		op_name = I0_ins_and;
		break;
	case I0_OPCODE_OR:
		op_name = I0_ins_or;
		break;
	case I0_OPCODE_XOR:
		op_name = I0_ins_xor;
		break;
	case I0_OPCODE_SPAWN:
		op_name = I0_ins_spawn;
		opcode4.load_extra_bytes(I0_INS_LEN_SPAWN);
		Op1.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op2.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op3.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op4.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		check_oper_M(Op1);
		check_oper_M(Op2);
		check_oper_M(Op3);
		check_oper_M(Op4);
		return;
	case I0_OPCODE_EXIT:
		op_name = I0_ins_exit;
		br_type = I0_INS_BR_TYPE_STOP;
		opcode4.load_extra_bytes(I0_INS_LEN_EXIT);
		{
			uint32_t exit_opt = opcode4.fetch_bits(I0_INS_BIT_LEN_OPT_EXIT);
			switch (exit_opt) {
			case I0_OPT_EXIT_A:
				opt = i0_ins_opt_pref_exit_a;
				break;
			case I0_OPT_EXIT_AD:
				opt = i0_ins_opt_pref_exit_ad;
				break;
			case I0_OPT_EXIT_C:
				opt = i0_ins_opt_pref_exit_c;
				break;
			case I0_OPT_EXIT_CD:
				opt = i0_ins_opt_pref_exit_cd;
				break;
			}
		}
		return;
	case I0_OPCODE_B: {
		uint32_t b_opt = opcode4.fetch_bits(I0_INS_BIT_LEN_OPT_B);
		uint32_t b_ra;
		op_t* br_target_op = NULL;
		br_type = I0_INS_BR_TYPE_JMP;
		switch (b_opt) {
		case I0_OPT_B_IJ:
			op_name = I0_ins_bij;
			br_opnd = 0;
			opcode4.load_extra_bytes(I0_INS_LEN_BIJ);
			Op1.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
			check_oper_C_indir(Op1);
			return;
		case I0_OPT_B_J:
			op_name = I0_ins_bj;
			br_opnd = 0;
			opcode4.load_extra_bytes(I0_INS_LEN_BJ);
			br_target_op = &Op1;
			break;
		case I0_OPT_B_LE:
			opt = i0_ins_opt_pref_b_le;
			break;
		case I0_OPT_B_E:
			opt = i0_ins_opt_pref_b_e;
			break;
		case I0_OPT_B_L:
			opt = i0_ins_opt_pref_b_l;
			break;
		case I0_OPT_B_NE:
			opt = i0_ins_opt_pref_b_ne;
			break;
		case I0_OPT_B_SL:
			opt = i0_ins_opt_pref_b_sl;
			break;
		case I0_OPT_B_Z:
			opt = i0_ins_opt_pref_b_z;
			break;
		case I0_OPT_B_NZ:
			opt = i0_ins_opt_pref_b_nz;
			break;
		default:
			throw(int(I0_DECODE_STATUS_B_OPT));
		}
		switch (b_opt) {
		case I0_OPT_B_LE:
		case I0_OPT_B_E:
		case I0_OPT_B_L:
		case I0_OPT_B_NE:
		case I0_OPT_B_SL:
			op_name = I0_ins_bcc;
			br_opnd = 2;
			br_type = I0_INS_BR_TYPE_JCC;
			opcode4.load_extra_bytes(I0_INS_LEN_BCMP);
			cmp_attr = opcode4.fetch_bits(I0_INS_BIT_LEN_ATTR);
			Op1.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
			Op2.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
			check_oper(Op1);
			check_oper(Op2);
			br_target_op = &Op3;
			break;
		case I0_OPT_B_Z:
		case I0_OPT_B_NZ:
			op_name = I0_ins_bcz;
			br_opnd = 1;
			br_type = I0_INS_BR_TYPE_JCC;
			opcode4.load_extra_bytes(I0_INS_LEN_BZNZ);
			cmp_attr = opcode4.fetch_bits(I0_INS_BIT_LEN_ATTR);
			Op1.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
			check_oper(Op1);
			br_target_op = &Op2;
			return;
		default:
			throw(int(0));
		}
		b_ra = opcode4.fetch_bits(I0_INS_BIT_LEN_RA);
		if (b_ra == I0_OPT_JUMP_R) {
			br_type |= I0_INS_BR_R;
		}
		check_oper_C(*br_target_op);
	}
		return;
	case I0_OPCODE_NOP:
		op_name = I0_ins_nop;
		opcode4.load_extra_bytes(I0_INS_LEN_NOP);
		return;
	case I0_OPCODE_INT:
		op_name = I0_ins_int;
		opcode4.load_extra_bytes(I0_OPCODE_INT);
		Op1.addrm = I0_ADDRM_IMMEDIATE;
		check_oper(Op1, I0_ATTR_UB);
		return;
	case I0_OPCODE_SHIFT:
		opcode4.load_extra_bytes(I0_INS_LEN_SHIFT);
		{
			uint32_t shrl_op = opcode4.fetch_bits(I0_INS_BIT_LEN_OPT_SHIFT);
			// currently other 2 options not implemented
			switch (shrl_op) {
			case I0_OPT_SHIFT_L:
				op_name = I0_ins_shl;
				break;
			case I0_OPT_SHIFT_R:
				op_name = I0_ins_shr;
				break;
			default:
				throw(int(I0_DECODE_STATUS_SHRL_OPT));
			}
		}
		ins_attr = opcode4.fetch_bits(I0_INS_BIT_LEN_ATTR);
		Op1.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op2.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op3.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		check_oper(Op1);
		check_oper(Op2);
		check_oper_M(Op3);
		return;
	case I0_OPCODE_SCMP:
		op_name = I0_ins_scmp;
		opcode4.load_extra_bytes(I0_INS_LEN_SCMP);
		Op1.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op2.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op3.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op4.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op5.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		ins_attr = I0_ATTR_UE;
		check_oper_M(Op1);
		check_oper(Op2);
		check_oper_M(Op3);
		check_oper(Op4);
		check_oper_M(Op5);
		return;
	default:
		throw(int(I0_DECODE_STATUS_OPCODE));
	}
	//only alu instructions fall here!
	switch (opcode) {
	case I0_OPCODE_ADD:
	case I0_OPCODE_SUB:
	case I0_OPCODE_MUL:
	case I0_OPCODE_DIV:
	case I0_OPCODE_AND:
	case I0_OPCODE_OR:
	case I0_OPCODE_XOR:
		opcode4.load_extra_bytes(I0_INS_LEN_ALU);
		ins_attr = opcode4.fetch_bits(I0_INS_BIT_LEN_ATTR);
		Op1.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op2.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op3.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		check_oper(Op1);
		check_oper(Op2);
		check_oper_M(Op3);
		return;
	default:
		throw(int(0));
	}
}
catch (...) {
	delete[] opnds;
	throw;
}

insn_t::~insn_t() {
	delete[] opnds;
}
uint8_t insn_t::ins_fetch_byte() {
	uint8_t ret = i0_get_byte(ip + ins_size);
	ins_size += sizeof(uint8_t);
	return ret;
}
void insn_t::ins_check_byte(){
	i0_check_bound(ip, sizeof(uint8_t));
}
uint16_t insn_t::ins_fetch_word() {
	uint16_t ret = i0_get_word(ip + ins_size);
	ins_size += sizeof(uint16_t);
	return ret;
}
void insn_t::ins_check_word(){
	i0_check_bound(ip, sizeof(uint16_t));
}
uint32_t insn_t::ins_fetch_dword() {
	uint32_t ret = i0_get_dword(ip + ins_size);
	ins_size += sizeof(uint32_t);
	return ret;
}
void insn_t::ins_check_dword(){
	i0_check_bound(ip, sizeof(uint32_t));
}
uint64_t insn_t::ins_fetch_qword() {
	uint64_t ret = i0_get_qword(ip + ins_size);
	ins_size += sizeof(uint64_t);
	return ret;
}
void insn_t::ins_check_qword(){
	i0_check_bound(ip, sizeof(uint64_t));
}

int main(int argc, char** argv) {
	int i0_prog_fd = open(argv[1], O_RDONLY);
	if (i0_prog_fd < 0) {
		exit(-1);
	}
	struct stat file_info;
	fstat(i0_prog_fd, &file_info);
	i0_prog_size = file_info.st_size;
	void* mapped_text_seg = mmap(NULL, i0_prog_size, PROT_READ, MAP_PRIVATE,
			i0_prog_fd, 0);

	return 0;
}
