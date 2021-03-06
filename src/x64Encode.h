#ifndef X64ENCODE_H_
#define X64ENCODE_H_

#define x64_ALU_OP_ID_ADD		0
#define x64_ALU_OP_ID_OR		1
#define x64_ALU_OP_ID_ADC		2
#define x64_ALU_OP_ID_SBB		3
#define x64_ALU_OP_ID_AND		4
#define x64_ALU_OP_ID_SUB		5
#define x64_ALU_OP_ID_XOR		6
#define x64_ALU_OP_ID_CMP		7

#define x64_TTTN_O				0
#define x64_TTTN_NO				1
#define x64_TTTN_B				2
#define x64_TTTN_NB				3
#define x64_TTTN_Z				4
#define x64_TTTN_NZ				5
#define x64_TTTN_BE				6
#define x64_TTTN_NBE			7
#define x64_TTTN_S				8
#define x64_TTTN_NS				9
#define x64_TTTN_P				10
#define x64_TTTN_NP				11
#define x64_TTTN_L				12
#define x64_TTTN_NL				13
#define x64_TTTN_LE				14
#define x64_TTTN_NLE			15

#define x64_REG_AX 0
#define x64_REG_CX 1
#define x64_REG_DX 2
#define x64_REG_BX 3
#define x64_REG_SP 4
#define x64_REG_BP 5
#define x64_REG_SI 6
#define x64_REG_DI 7
#define x64_REG_R8 8
#define x64_REG_R9 9
#define x64_REG_R10 10
#define x64_REG_R11 11
#define x64_REG_R12 12
#define x64_REG_R13 13
#define x64_REG_R14 14
#define x64_REG_R15 15

#define REX_B_BIT 0
#define REX_X_BIT 1
#define REX_R_BIT 2
#define REX_W_BIT 3

#define TYPE_LEN_BYTE 1
#define TYPE_LEN_DWORD 4
#define TYPE_LEN_QWORD 8
#define TYPE_LEN_OWORD 16

#define ZEROOUT_x64_INSTR(instr) \
	do{memset((char*)(instr),0,sizeof(x64INSTR));}while(0)

#define WITHIN64_8BIT(val) \
	(((int64_t)val) == ((int64_t)((int8_t)((uint8_t)((uint64_t)(val))))))

#define WITHIN64_32BIT(val) \
	(((int64_t)val) == ((int64_t)((int32_t)((uint32_t)((uint64_t)(val))))))

#define WITHIN32_8BIT(val) \
	(((int32_t)val) == ((int32_t)((int8_t)((uint8_t)((uint32_t)(val))))))

typedef union _IMM_TYPE {
	uint8_t v8;
	uint16_t v16;
	uint32_t v32;
	uint64_t v64;
}IMM_TYPE;

typedef union _OFFSET_TYPE {
	uint8_t v8;
	uint32_t v32;
}OFFSET_TYPE;

typedef struct _x64_OPR{
	uint8_t type;
	uint8_t full_encoded;
#define x64_OPR_TYPE_REG		1
#define x64_OPR_TYPE_M			2
#define x64_OPR_I				0
	uint8_t reg;
	uint32_t off32;
	IMM_TYPE imm;
}x64_OPR;

typedef struct _x64INSTR{
	uint8_t LegacyPrefix_Len;
	uint8_t opcode_len;
	uint8_t ModRM_SIB_len;
	uint8_t disp_len;
	uint8_t imm_len;
	uint8_t LegacyPrefix[1];
	uint8_t REX;
	uint8_t opcode[2];
	uint8_t ModRM_SIB[2];
	OFFSET_TYPE disp;
	IMM_TYPE imm;

}x64INSTR;

typedef struct {
	unsigned sp;
	x64INSTR x64instrs[10];
}x64INSTR_ST;

#endif /* X64ENCODE_H_ */
