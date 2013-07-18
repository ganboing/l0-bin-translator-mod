#ifndef I0TYPES_H_
#define I0TYPES_H_

typedef union _OPRVAL {
	uint8_t v8;
	uint16_t v16;
	uint32_t v32;
	uint64_t v64;
}OPRVAL;

typedef struct _I0OPR {
	OPRVAL val;
	uint32_t addrm;
	uint32_t disp32;
}I0OPR;

typedef struct _I0INSTR {
	uint64_t addr;
	uint32_t addr_size_mode;
	uint32_t opcode;
	uint32_t attr;
	uint32_t attr2;
	uint32_t option;
	uint32_t ra;
	I0OPR opr[5];
}I0INSTR;

#define MAX_OPR_NUM 5

typedef struct _instr_t {
    // unsigned int l1; // obsolete
    // unsigned int l2; // obsolete
    uint64_t addr;
    uint32_t addr_size_mode;
    uint32_t opcode;
    uint32_t option;
    uint32_t mattr1;
    uint32_t mattr2;
    uint32_t addrm1;
    uint32_t addrm2;
    uint32_t addrm3;
    uint32_t addrm4;
    uint32_t attr;
    uint32_t ra;
    uint64_t opr1;
    uint32_t disp1;
    uint64_t opr2;
    uint32_t disp2;
    uint64_t opr3;
    uint32_t disp3;
    uint64_t opr4;
    uint32_t disp4;
    uint32_t addrms[MAX_OPR_NUM];
    uint64_t oprs[MAX_OPR_NUM];
    uint32_t disps[MAX_OPR_NUM];
} instr_t;

#undef MAX_OPR_NUM


#endif /* I0TYPES_H_ */
