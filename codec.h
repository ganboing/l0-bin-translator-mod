#ifndef _CODEC_H_
#define _CODEC_H_

enum {
    EBM = 0,
    SBM = 1,
} enum_address_size_mode;

enum {
    OP_UNDEFINED = -1,
    OP_NOP = 0,
    OP_CONV = 0x21,
    OP_ADD = 0x3,
    OP_SUB = 0x6,
    OP_MUL = 0x9,
    OP_DIV = 0xc,
    OP_SPAWN = 0x25,
    OP_EXIT = 0x86,
    OP_B = 0x15,
    OP_AND = 0x18,
    OP_OR = 0x1b,
    OP_XOR = 0x1e,
    OP_INT = 0x24,
    OP_SHIFT = 0x27,
    OP_SCMP = 0x2a,
} enum_opcode;

// enum {
//     TYPE_UNDEFINED = -1,
//     TYPE_INSTR,
//     TYPE_LABEL,
//     TYPE_COMMENT,
//     TYPE_ETC,
// };

enum {
    OPT_UNDEFINED = -1,
    // OPT_CONV_S = 1,
    // OPT_CONV_Z = 0,
    OPT_EXIT_C = 0,
    OPT_EXIT_A = 1,
    OPT_EXIT_CD = 2,
    OPT_EXIT_AD = 3,
    OPT_B_J = 0,
    OPT_B_L = 2,
    OPT_B_LE = 3,
    OPT_B_E = 4,
    OPT_B_NE = 5,
    OPT_B_Z = 6,
    OPT_B_NZ = 7,
    OPT_B_SL = 8,
    OPT_B_IJ = 12,
    OPT_SHIFT_L = 0,
    OPT_SHIFT_R = 1,
} enum_option;

enum {
    ADDRM_UNDEFINED = -1,
    ADDRM_IMMEDIATE = 0,
    ADDRM_ABSOLUTE = 1,
    ADDRM_INDIRECT = 2,
    ADDRM_DISPLACEMENT = 3,
} enum_addressing_mode;

/* obsolete */
// enum {
//     LEN_UNDEFINED = -1,
//     LEN_Q = 0,
//     LEN_L = 1,
//     LEN_B = 2,
// };

enum {
    MATTR_UNDEFINED = -1,
    ATTR_UNDEFINED = -1,
    // byte
    MATTR_SB = 0,
    ATTR_SB = 0,
    MATTR_UB = 4,
    ATTR_UB = 4,
    // fourbyte
    // not implemented yet
    MATTR_SF = 3,
    ATTR_SF = 3,
    MATTR_UF = 7,
    ATTR_UF = 7,
    // eightbyte
    MATTR_SE = 1,
    ATTR_SE = 1,
    MATTR_UE = 5,
    ATTR_UE = 5,
    // sixteenbyte
    MATTR_SS = 2,
    ATTR_SS = 2,
    MATTR_US = 6,
    ATTR_US = 6,
    // double float
    MATTR_FD = 9,
    ATTR_FD = 9,
    // single float
    MATTR_FS = 8,
    ATTR_FS = 8,
} enum_attribute;

enum {
    SIZE_UNDEFINED = -1,
    SIZE_S = 16,
    SIZE_E = 8,
    SIZE_F = 4,
    SIZE_B = 1,
    SIZE_FD = 8,
    SIZE_FS = 4,
} enum_size;

enum {
    JUMP_UNDEFINED = -1,
    JUMP_R = 1,
    JUMP_A = 0,
} enum_jump_type;

enum {
    BIT_LEN_UNDEFINED = -1,
    BIT_LEN_ADDR_SIZE_MODE = 1,
    BIT_LEN_OPCODE = 10,
    // BIT_LEN_OPT_CONV = 1,
    BIT_LEN_OPT_EXIT = 2,
    BIT_LEN_OPT_B = 4,
    BIT_LEN_RA = 1, /* relative / absolute */
    // BIT_LEN_LEN = 2, /* obsolete */
    BIT_LEN_MATTR = 4,
    BIT_LEN_ATTR = 4,
    BIT_LEN_ADDRM = 3,
    BIT_LEN_OPT_SHIFT = 1,
};

// No change to the order here (number value).
// reg to physical reg mapping should not be changed.
enum {
    I0_REG_FILE0 = 0, // %rbx
    I0_REG_FILE1 = 1, // %rcx
    I0_REG_FILE2 = 2, // %r15
    I0_REG_FILE3 = 3, // %r14
    I0_REG_FILE4 = 4, // %r13
    I0_REG_FILE5 = 5, // %r10
    I0_REG_FILE6 = 6, // %r9
    I0_REG_FILE7 = 7  // %r8
};

// Note: should no overlap with REG_FILEx. Can rename REG_FILEx.
enum {
    X86_64_R_RDI = 101,
    X86_64_R_RSI = 102,
    X86_64_R_RAX = 103,
    X86_64_R_RDX = 104,

    X86_64_R_RCX = I0_REG_FILE1
};

#endif // _CODEC_H_

