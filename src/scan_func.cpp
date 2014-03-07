#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "scan_func.hpp"
#include <boost/shared_ptr.hpp>

#define Op1 (opnds[0])
#define Op2 (opnds[1])
#define Op3 (opnds[2])
#define Op4 (opnds[3])
#define Op5 (opnds[4])

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

static uint8_t i0_get_byte(i0_ea_type_t ea) {
	return 0;
}

static uint16_t i0_get_word(i0_ea_type_t ea) {
	return 0;
}

static uint32_t i0_get_dword(i0_ea_type_t ea) {
	return 0;
}

static uint64_t i0_get_qword(i0_ea_type_t ea) {
	return 0;
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
	uint8_t load_cnt = insn_ref.size + cnt;
	while (cnt) {
		opcode4.b[3 - load_cnt + cnt] = insn_ref.i0_fetch_byte();
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

void insn_t::fill_oper(op_t& op) {

}

void insn_t::fill_oper(op_t& op, uint8_t attr) {

}

insn_t::insn_t(i0_ea_type_t _ip)
try :
		opnds(NULL), ip(_ip), op_name(I0_ins_last_ins), opt(
				i0_ins_opt_pref_last), ins_attr(i0_attr_last), size(0) {
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
		fill_oper(Op1, src_attr);
		fill_oper_M(Op2, dest_attr);
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
		fill_oper_M(Op1);
		fill_oper_M(Op2);
		fill_oper_M(Op3);
		fill_oper_M(Op4);
		return;
	case I0_OPCODE_EXIT:
		op_name = I0_ins_exit;
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
		switch (b_opt) {
		case I0_OPT_B_IJ:
			op_name = I0_ins_bij;
			opcode4.load_extra_bytes(I0_INS_LEN_BIJ);
			Op1.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
			fill_oper_C_indir(Op1);
			return;
		case I0_OPT_B_J:
			op_name = I0_ins_bj;
			opcode4.load_extra_bytes(I0_INS_LEN_BJ);
			opt = opcode4.fetch_bits(I0_INS_BIT_LEN_RA);
			fill_oper_C(Op1);
			return;
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
			opcode4.load_extra_bytes(I0_INS_LEN_BCMP);
			cmp_attr = opcode4.fetch_bits(I0_INS_BIT_LEN_ATTR);
			Op1.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
			Op2.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
			opt = opcode4.fetch_bits(I0_INS_BIT_LEN_RA);
			fill_oper(Op1);
			fill_oper(Op2);
			fill_oper_C(Op3);
			return;
		case I0_OPT_B_Z:
		case I0_OPT_B_NZ:
			op_name = I0_ins_bcz;
			opcode4.load_extra_bytes(I0_INS_LEN_BZNZ);
			cmp_attr = opcode4.fetch_bits(I0_INS_BIT_LEN_ATTR);
			Op1.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
			opt = opcode4.fetch_bits(I0_INS_BIT_LEN_RA);
			fill_oper(Op1);
			fill_oper_C(Op2);
			return;
		default:
			throw(int(0));
		}
	}
		return;
	case I0_OPCODE_NOP:
		op_name = I0_ins_nop;
		opcode4.load_extra_bytes(I0_INS_LEN_NOP);
		return;
	case I0_OPCODE_INT:
		op_name = I0_ins_int;
		opcode4.load_extra_bytes(I0_OPCODE_INT);
		Op1.addrm=I0_ADDRM_IMMEDIATE;
		fill_oper(Op1, I0_ATTR_UB);
		return;
	case I0_OPCODE_SHIFT:
		opcode4.load_extra_bytes(I0_INS_LEN_SHIFT);
		opt = opcode4.fetch_bits(I0_INS_BIT_LEN_OPT_SHIFT);
		// currently other 2 options not implemented
		switch(opt)
		{
		case I0_OPT_SHIFT_L:
			op_name = I0_ins_shl;
			break;
		case I0_OPT_SHIFT_R:
			op_name = I0_ins_shr;
			break;
		default:
			throw(int(I0_DECODE_STATUS_SHRL_OPT));
		}
		ins_attr = opcode4.fetch_bits(I0_INS_BIT_LEN_ATTR);
		Op1.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op2.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		Op3.addrm = opcode4.fetch_bits(I0_INS_BIT_LEN_ADDRM);
		fill_oper(Op1);
		fill_oper(Op2);
		fill_oper_M(Op3);
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
		fill_oper_M(Op1);
		fill_oper(Op2);
		fill_oper_M(Op3);
		fill_oper(Op4);
		fill_oper_M(Op5);
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
		fill_oper(Op1);
		fill_oper(Op2);
		fill_oper_M(Op3);
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
uint8_t insn_t::i0_fetch_byte() {
	uint8_t ret = i0_get_byte(ip + size);
	size += sizeof(uint8_t);
	return ret;
}
uint16_t insn_t::i0_fetch_word() {
	uint16_t ret = i0_get_word(ip + size);
	size += sizeof(uint16_t);
	return ret;
}
uint32_t insn_t::i0_fetch_dword() {
	uint32_t ret = i0_get_dword(ip + size);
	size += sizeof(uint32_t);
	return ret;
}
uint64_t insn_t::i0_fetch_qword() {
	uint64_t ret = i0_get_qword(ip + size);
	size += sizeof(uint64_t);
	return ret;
}

int main(int argc, char** argv) {
	int i0_prog_fd = open(argv[1], O_RDONLY);
	if (i0_prog_fd < 0) {
		exit(-1);
	}
	struct stat file_info;
	fstat(i0_prog_fd, &file_info);
	__off64_t i0_prog = file_info.st_size;
	void* mapped_text_seg = mmap(NULL, i0_prog, PROT_READ, MAP_PRIVATE,
			i0_prog_fd, 0);

	return 0;
}
