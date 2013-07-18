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

#define x64_AX 0
#define x64_CX 1
#define x64_DX 2
#define x64_BX 3
#define x64_SP 4
#define x64_BP 5
#define x64_SI 6
#define x64_DI 7
#define x64_R8 8
#define x64_R9 9
#define x64_R10 10
#define x64_R11 11
#define x64_R12 12
#define x64_R13 13
#define x64_R14 14
#define x64_R15 15

#define REX_B_BIT 0
#define REX_X_BIT 1
#define REX_R_BIT 2
#define REX_W_BIT 3

#define TYPE_ID_BYTE 0
#define TYPE_ID_DWORD 1
#define TYPE_ID_QWORD 2
#define TYPE_ID_OWORD 3

#define ZEROOUT_x64_INSTR() \
	do{memset((char*)(x64instrs + instr_cnt),0,sizeof(x64INSTR));}while(0)

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

#endif /* X64ENCODE_H_ */
