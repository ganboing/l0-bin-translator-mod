#ifndef SCAN_FUNC_H
#define SCAN_FUNC_H

typedef uint64_t i0_ea_type_t;
typedef uint64_t i0_val_type_t;

#define qnumber(a)     (sizeof(a)/sizeof((a)[0]))

#define I0_DECODE_STATUS_B_OPT 				(1U)
#define I0_DECODE_STATUS_SHRL_OPT 			(2U)
#define I0_DECODE_STATUS_OPCODE				(3U)
#define I0_DECODE_STATUS_ADDRM				(4U)
#define I0_DECODE_STATUS_ATTR				(5U)
#define I0_DECODE_STATUS_TEXT_SEGMENT		(6U)
#define I0_MAX_OPND_NUMBER	5

#pragma pack(push, 1)

enum I0_ins_options {
	i0_ins_opt_pref_b_le,
	i0_ins_opt_pref_b_l,
	i0_ins_opt_pref_b_e,
	i0_ins_opt_pref_b_ne,
	i0_ins_opt_pref_b_sl,
	i0_ins_opt_pref_b_z,
	i0_ins_opt_pref_b_nz,
	i0_ins_opt_pref_exit_c,
	i0_ins_opt_pref_exit_a,
	i0_ins_opt_pref_exit_cd,
	i0_ins_opt_pref_exit_ad,
	i0_ins_opt_pref_last
};

static_assert((sizeof(I0_ins_options)==1), "");

enum i0_ins_names {
	I0_ins_conv,
	I0_ins_add,
	I0_ins_sub,
	I0_ins_mul,
	I0_ins_div,
	I0_ins_and,
	I0_ins_or,
	I0_ins_xor,
	I0_ins_spawn,
	I0_ins_spawnx,
	I0_ins_exit,
	I0_ins_bcc,
	I0_ins_bcz,
	I0_ins_bj,
	I0_ins_bij,
	I0_ins_nop,
	I0_ins_int,
	I0_ins_shl,
	I0_ins_shr,
	I0_ins_scmp,
	I0_ins_mov,
	I0_ins_last_ins
};

static_assert((sizeof(i0_ins_names)==1), "");

enum i0_instr_addrm {
	i0_addrm_Imm = 0,
	i0_addrm_Abs = 1,
	i0_addrm_Indir = 2,
	i0_addrm_Disp = 3,
	i0_addrm_last
};

enum i0_oper_attr {
	i0_attr_sb = 0,
	i0_attr_se = 1,
	i0_attr_ss = 2,
	i0_attr_sf = 3,
	i0_attr_ub = 4,
	i0_attr_ue = 5,
	i0_attr_us = 6,
	i0_attr_uf = 7,
	i0_attr_fs = 8,
	i0_attr_fd = 9,
	i0_attr_last
};

enum I0_regs {
	i0_reg_BP,
	i0_reg_SP,
	i0_reg_R2,
	i0_reg_R3,
	i0_reg_R4,
	i0_reg_R5,
	i0_reg_R6,
	i0_reg_R7,
	i0_reg_rVcs,
	i0_reg_rVds,
	i0_reg_l0_stdin,
	i0_reg_l0_stdout,
	i0_reg_TaskWpr_sp,
	i0_reg_Syscall_sp,
	i0_reg_rrStkBase,
	i0_reg_rrStkLen,
	i0_reg_rr_fi,
	i0_reg_rr_ID,
	i0_reg_I0_regs_last
};

#pragma pack(pop)

struct op_mem_desp {
	int32_t displ;
	i0_ea_type_t addr;
};

struct op_t {
	op_t();
	//uint8_t attr;
	uint8_t addrm;
	uint8_t ins_offset;
	/*bool code_ref;
	 int32_t displ;
	 union {
	 i0_ea_type_t addr;
	 uint8_t reg;
	 uint64_t val_ue;
	 int64_t val_se;
	 uint32_t val_uf;
	 int32_t val_sf;
	 uint8_t val_ub;
	 int8_t val_sb;
	 float val_fs;
	 double val_fd;
	 };*/
};

static_assert((sizeof(op_t) == (1+1)), "check op_t size");

class insn_t;
class i0_opcode4_t {
private:
	i0_opcode4_t();
	uint32_t get_bits(unsigned start, unsigned len);
	insn_t& insn_ref;
	union {
		uint8_t b[4];
		uint32_t l;
	} opcode4;
	unsigned shift_cnt;
	void load_bytes(uint8_t cnt);
	void load_op_bytes();
public:
	i0_opcode4_t(insn_t& _cmd);
	void load_extra_bytes(uint8_t cnt);
	uint32_t fetch_bits(unsigned len);
};

class insn_t {
public:
	insn_t(i0_ea_type_t _ip);
	~insn_t();
private:
	insn_t();
	void check_oper(op_t&);
	void check_oper(op_t& op, uint8_t attr);
	void check_oper_M(op_t&);
	void check_oper_M(op_t& op, uint8_t attr);
	void check_oper_C(op_t& op);
	void check_oper_C_indir(op_t& op);
	uint8_t ins_fetch_byte();
	uint16_t ins_fetch_word();
	uint32_t ins_fetch_dword();
	uint64_t ins_fetch_qword();
	void ins_check_byte();
	void ins_check_word();
	void ins_check_dword();
	void ins_check_qword();
	void* i0_op_raw_ptr(op_t& op);

	i0_ea_type_t ip;
	i0_ins_names op_name;
	I0_ins_options opt;
	op_t opnds[I0_MAX_OPND_NUMBER];
	union {
		uint8_t ins_attr;
		uint8_t src_attr;
		uint8_t cmp_attr;
	};
	union {
		uint8_t __ins_extra_attr__;
		uint8_t dest_attr;
		uint8_t br_opnd;
	};
	uint8_t ins_size;
	uint8_t br_type;
	friend class i0_opcode4_t;
};

#define I0_OPCODE_NOP		(0x00U)
#define I0_OPCODE_ADD		(0x03U)
#define I0_OPCODE_SUB		(0x06U)
#define I0_OPCODE_MUL		(0x09U)
#define I0_OPCODE_DIV		(0x0cU)
#define I0_OPCODE_B			(0x15U)
#define I0_OPCODE_AND		(0x18U)
#define I0_OPCODE_OR		(0x1bU)
#define I0_OPCODE_XOR		(0x1eU)
#define I0_OPCODE_CONV		(0x21U)
#define I0_OPCODE_INT		(0x24U)
#define I0_OPCODE_SPAWN		(0x25U)
#define I0_OPCODE_SHIFT		(0x27U)
#define I0_OPCODE_SCMP		(0x2aU)
#define I0_OPCODE_EXIT		(0x86U)

#define I0_OPT_JUMP_R		(0x01U)
#define I0_OPT_JUMP_A		(0x00U)
#define I0_OPT_EXIT_C		(0x00U)
#define I0_OPT_EXIT_A		(0x01U)
#define I0_OPT_EXIT_CD		(0x02U)
#define I0_OPT_EXIT_AD		(0x03U)
#define I0_OPT_B_J			(0x00U)
#define I0_OPT_B_L			(0x02U)
#define I0_OPT_B_LE			(0x03U)
#define I0_OPT_B_E			(0x04U)
#define I0_OPT_B_NE			(0x05U)
#define I0_OPT_B_Z			(0x06U)
#define I0_OPT_B_NZ			(0x07U)
#define I0_OPT_B_SL			(0x08U)
#define I0_OPT_B_IJ			(0x0cU)
#define I0_OPT_SHIFT_L		(0x00U)
#define I0_OPT_SHIFT_R		(0x01U)

#define I0_INS_LEN_CONV		(0x04U)
#define I0_INS_LEN_ALU		(0x03U)
#define I0_INS_LEN_SPAWN	(0x03U)
#define I0_INS_LEN_EXIT		(0x02U)
#define I0_INS_LEN_BIJ		(0x03U)
#define I0_INS_LEN_BJ		(0x02U)
#define I0_INS_LEN_BCMP		(0x04U)
#define I0_INS_LEN_BZNZ		(0x03U)
#define I0_INS_LEN_NOP		(0x02U)
#define I0_INS_LEN_INT		(0x02U)
#define I0_INS_LEN_SHIFT	(0x04U)
#define I0_INS_LEN_SCMP		(0x04U)
#define I0_INS_LEN_OPCODE	(0x02U)

#define I0_INS_BIT_LEN_OPCODE			(0x0bU)
#define I0_INS_BIT_LEN_OPT_EXIT			(0x02U)
#define I0_INS_BIT_LEN_OPT_B			(0x04U)
#define I0_INS_BIT_LEN_RA				(0x01U)
#define I0_INS_BIT_LEN_MATTR			(0x04U)
#define I0_INS_BIT_LEN_ATTR				(0x04U)
#define I0_INS_BIT_LEN_ADDRM			(0x03U)
#define I0_INS_BIT_LEN_OPT_SHIFT		(0x02U)

#define I0_INS_LEN_ADDRM_LEN		(3U)
#define I0_INS_LEN_ATTR_LEN			(4U)

#define I0_ADDRM_IMMEDIATE			(0x00U)
#define I0_ADDRM_ABSOLUTE			(0x01U)
#define I0_ADDRM_INDIRECT			(0x02U)
#define I0_ADDRM_DISPLACEMENT		(0x03U)

#define I0_ATTR_SB					(0x00U)
#define I0_ATTR_SE					(0x01U)
#define I0_ATTR_SS					(0x02U)
#define I0_ATTR_SF					(0x03U)
#define I0_ATTR_UB					(0x04U)
#define I0_ATTR_UE					(0x05U)
#define I0_ATTR_US					(0x06U)
#define I0_ATTR_UF					(0x07U)
#define I0_ATTR_FS					(0x08U)
#define I0_ATTR_FD					(0x09U)

#define I0_INS_BR_TYPE_MASK				(0x03U)
	#define I0_INS_BR_TYPE_FLOW_NXT			(0x00U)
	#define I0_INS_BR_TYPE_STOP				(0x01U)
	#define I0_INS_BR_TYPE_JCC				(0x02U)
	#define I0_INS_BR_TYPE_JMP				(0x03U)
#define I0_INS_BR_TARGET_TYPE_MASK		(0x04U)
	#define I0_INS_BR_TARGET_DETERMINE		(0x00U)
	#define I0_INS_BR_TARGET_UNDETERMINE	(0x04U)
#define I0_INS_BR_RA_MASK				(0x10U)
	#define I0_INS_BR_R						(0x10U)
	#define I0_INS_BR_A						(0x00U)
#define I0_INS_BR_RET_MASK				(0x20U)
	#define I0_INS_BR_RET					(0x20U)
	#define I0_INS_BR_M						(0x00U)

#endif
