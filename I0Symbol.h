#ifndef I0SYMBOL_H_
#define I0SYMBOL_H_

#define OP_UNDEFINED	-1
#define OP_NOP			0x0
#define OP_ADD			0x03
#define OP_SUB			0x06
#define OP_MUL			0x09
#define OP_DIV			0x0c
#define OP_B			0x15
#define OP_AND			0x18
#define OP_OR			0x1b
#define OP_XOR			0x1e
#define OP_CONV			0x21
#define OP_INT			0x24
#define OP_SPAWN		0x25
#define OP_SHIFT		0x27
#define OP_SCMP			0x2a
#define OP_EXIT			0x86

#define OPT_UNDEFINED	-1
#define OPT_EXIT_C		0x00
#define OPT_EXIT_A		0x01
#define OPT_EXIT_CD		0x02
#define OPT_EXIT_AD		0x03
#define OPT_B_J			0x00
#define OPT_B_L			0x02
#define OPT_B_LE		0x03
#define OPT_B_E			0x04
#define OPT_B_NE		0x05
#define OPT_B_Z			0x06
#define OPT_B_NZ		0x07
#define OPT_B_SL		0x08
#define OPT_B_IJ		0x0c
#define OPT_SHIFT_L		0x00
#define OPT_SHIFT_R		0x01


#define ADDRM_UNDEFINED		-1
#define ADDRM_IMMEDIATE		0x00
#define ADDRM_ABSOLUTE		0x01
#define ADDRM_INDIRECT		0x02
#define ADDRM_DISPLACEMENT	0x03


#define OPR_I_MATTR_UNDEFINED 		0x01
#define OPR_I_MATTR_NOTIMPLEMENT	0x02
#define OPR_M_ADDRM_UNDEFINED		0x03
#define OPR_M_ADDRM_EXCEPTION		0x04
#define OPCODE_UNDEFINED			0x05
#define OPCODE_B_UNDEFINED			0x06

#define MATTR_UNDEFINED		-1
#define ATTR_UNDEFINED		-1
#define MATTR_SB			0x00
#define ATTR_SB				0x00
#define MATTR_SE			0x01
#define ATTR_SE				0x01
#define MATTR_SS			0x02
#define ATTR_SS				0x02
#define MATTR_SF			0x03
#define ATTR_SF				0x03
#define MATTR_UB			0x04
#define ATTR_UB				0x04
#define MATTR_UE			0x05
#define ATTR_UE				0x05
#define MATTR_US			0x06
#define ATTR_US				0x06
#define MATTR_UF			0x07
#define ATTR_UF				0x07
#define MATTR_FS			0x08
#define ATTR_FS				0x08
#define MATTR_FD			0x09
#define ATTR_FD				0x09

#define SIZE_UNDEFINED		-1
#define SIZE_B				0x01
#define SIZE_FS				0x04
#define SIZE_F				0x04
#define SIZE_FD				0x08
#define SIZE_E				0x08
#define SIZE_S				0x10

#define JUMP_UNDEFINED		-1
#define JUMP_R				1
#define JUMP_A				0


#define BIT_LEN_UNDEFINED		-1
#define BIT_LEN_ADDR_SIZE_MODE	0x01
#define BIT_LEN_OPCODE			0x0a
#define BIT_LEN_OPT_EXIT		0x02
#define BIT_LEN_OPT_B			0x04
#define BIT_LEN_RA				0x01
#define BIT_LEN_MATTR			0x04
#define BIT_LEN_ATTR			0x04
#define BIT_LEN_ADDRM			0x03
#define BIT_LEN_OPT_SHIFT		0x02

#define BYTE_OP_CONV			0x04
#define BYTE_OP_ALU				0x03
#define BYTE_OP_SPAWN			0x03
#define BYTE_OP_EXIT			0x02
#define BYTE_OP_BIJ				0x03
#define BYTE_OP_BJ				0x02
#define BYTE_OP_BCMP			0x04
#define BYTE_OP_BZNZ			0x03
#define BYTE_OP_NOP				0x02
#define BYTE_OP_INT				0x02
#define BYTE_OP_SHIFT			0x04
#define BYTE_OP_SCMP			0x04


//#define DECODE_I0_DEBUG

#endif /* I0SYMBOL_H_ */
