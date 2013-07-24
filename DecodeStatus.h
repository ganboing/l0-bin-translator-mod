#ifndef DECODESTATUS_H_
#define DECODESTATUS_H_

#define I0_TRANS_SUCCESS_NO_FURTHER_PROC	0x00
#define I0_DECODE_INVALID_INSTRUCTION		0x01
	#define I0_DECODE_OPR_I_MATTR_UNDEFINED 	0x01
	#define I0_DECODE_OPR_I_MATTR_NOTIMPLEMENT	0x02
	#define I0_DECODE_OPR_M_ADDRM_UNDEFINED		0x03
	#define I0_DECODE_OPR_M_ADDRM_EXCEPTION		0x04
	#define I0_DECODE_OPCODE_UNDEFINED			0x05
	#define I0_DECODE_OPCODE_B_UNDEFINED		0x06
	#define I0_DECODE_OPERAND_UNDEFINED			0x07
#define I0_DECODE_SEGMENT_LIMIT				0x03
#define I0_DECODE_BRANCH					0x04
	#define I0_DECODE_JCC						0x01
	#define I0_DECODE_JMP						0x02
	#define I0_DECODE_JMP_INDIR					0x03
	#define I0_DECODE_EXIT						0x04
	#define I0_DECODE_INT						0x05

typedef struct _DECODE_STATUS{
	unsigned long status;
	unsigned long detail;
	unsigned long detail2;
}DECODE_STATUS;

#define RETURN_DECODE_STATUS(status, detail, detail2) \
	do{ DECODE_STATUS result= {(status), (detail), (detail2)}; return result;}while(0)


#endif /* DECODESTATUS_H_ */
