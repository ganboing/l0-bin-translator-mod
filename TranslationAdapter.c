#include <execinfo.h>
#include "sys_config.h"
#include "codec.h"
#include "bin_translator.h"
#include "zlog.h"

static void error(char *msg)
{

    // start stack trace
    const int MAX_TRACE_DEPTH = 20;
    size_t bt_size;
    void *bt[MAX_TRACE_DEPTH];
    char **bt_strs;
    int i;


    zlogf("ERROR: %s\n", msg);
    zlog_flush_buffer();

    bt_size = backtrace(bt, MAX_TRACE_DEPTH);
    bt_strs = backtrace_symbols(bt, bt_size);

    for(i = 0; i < bt_size; ++i)
        zlogf("%d: %s\n", i, bt_strs[i]);

    zlog_flush_buffer();

    vpc_error(msg);

    exit(1);
}

int within_32bit(uint64_t n)
{
    // long t = 0;
    long u = 0;
    // int t2 = 0;
    // t2 = *(int*)(&n);
    // t = (long) t2;
    u = *(long*)(&n);

    if (u > ((0x1 << 31) - 1)) return 0;

    if (u < (0 - (0x1 << 31))) return 0;

    return 1;

    // return (t == u);
}

inline int is_in_reg_file(uint64_t opr)
{
    if (opr >= REG_FILE_BEGIN && opr < REG_FILE_END) {
        return 1;
    } else {
        return 0;
    }
}

inline int opr_reg(uint64_t opr)
{
    return (opr - REG_FILE_BEGIN)/8;
}

inline int check_is_stdout(uint64_t opr, unsigned int addrm)
{
    if (addrm == ADDRM_ABSOLUTE) {
        if (opr == IO_STDOUT_Q) {
            return 1;
        }
    }

    return 0;
}

inline int check_is_stdin(uint64_t opr, unsigned int addrm)
{
    if (addrm == ADDRM_ABSOLUTE) {
        if (opr == IO_STDIN_Q) {
            return 1;
        }
    }

    return 0;
}

inline uint32_t x86_64_reg_encoding(uint64_t reg)
{
    uint32_t encoding = 0;

    switch (reg) {
        case I0_REG_FILE0:
            encoding = 3;
            break;
        case I0_REG_FILE1:
            encoding = 1;
            break;
        case I0_REG_FILE2:
            encoding = 15;
            break;
        case I0_REG_FILE3:
            encoding = 14;
            break;
        case I0_REG_FILE4:
            encoding = 13;
            break;
        case I0_REG_FILE5:
            encoding = 10;
            break;
        case I0_REG_FILE6:
            encoding = 9;
            break;
        case I0_REG_FILE7:
            encoding = 8;
            break;
        case X86_64_R_RDI:
            encoding = 7;
            break;
        case X86_64_R_RSI:
            encoding = 6;
            break;
        case X86_64_R_RAX:
            encoding = 0;
            break;
        case X86_64_R_RDX:
            encoding = 2;
            break;
        default:
            zlogf("reg_encoding: Not supported x86-64 register: %#lx\n", reg);
            error("reg_encoding: Not supported x86-64 register\n");
    }
    return encoding;
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_read_indirect_reg_file
(uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    if (mattr == MATTR_SB || mattr == MATTR_UB) {
        // error("reg_file with SB/UB is not implemented yet.\n");
        return translate2x86_64_read_reg_file_base_disp(opr, 0, reg, n, nl, is_write_n, mattr);
    }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        // movq oprreg, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq (%%oprreg %d), %%reg %d\n", n+(*nl), oprreg, reg);
#endif
        if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
            // %rbx and %rcx are special
            n[(*nl)++] = 0x48;
        } else {
            n[(*nl)++] = 0x49;
        }
        n[(*nl)++] = 0x8b;

        // reg combination
        regcomb = (oprreg << 16) + reg;
        switch (regcomb) {
            case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3b; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x33; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x13; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x03; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3f; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x37; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x17; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x07; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3e; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x36; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x16; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x06; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x7d; n[(*nl)++] = 0x00; break; // FILE4 is different
            case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x75; n[(*nl)++] = 0x00; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x55; n[(*nl)++] = 0x00; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x45; n[(*nl)++] = 0x00; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3a; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x32; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x12; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x02; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x38; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x30; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x10; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x00; break;
            default:
                error("translate2x86_64_read_indirect_reg_file: unsupported regcomb.\n");
        }
    } else {
        *nl += 3;
        if (oprreg == I0_REG_FILE4) {
            *nl += 1;
        }
    }
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_read_reg_file_base_disp
(uint64_t opr, uint32_t disp, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("reg_file with SB/UB is not implemented yet. working.\n");
    // }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        if (mattr == MATTR_SB || mattr == MATTR_UB) {
            // movb disp(oprreg), al
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movb %d(%%oprreg %d), %%al\n", n+(*nl), disp, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                n[(*nl)++] = 0x41;
            }
            n[(*nl)++] = 0x8a;

            // reg combination
            switch (oprreg) {
                case (I0_REG_FILE0): n[(*nl)++] = 0x83; break;
                case (I0_REG_FILE1): n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE2): n[(*nl)++] = 0x87; break;
                case (I0_REG_FILE3): n[(*nl)++] = 0x86; break;
                case (I0_REG_FILE4): n[(*nl)++] = 0x85; break;
                case (I0_REG_FILE5): n[(*nl)++] = 0x82; break;
                case (I0_REG_FILE6): n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE7): n[(*nl)++] = 0x80; break;
                default:
                     error("translate2x86_64_read_reg_file_base_disp: SB/UB: unsupported oprreg.\n");
            }
            // disp
            *((int32_t*)(n + (*nl))) = (int32_t)disp;
            *nl += SIZE_F;
 
            // error("reg_file with SB/UB is not implemented yet. working.\n");
        } else {
            // Q
            // movq disp(oprreg), reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %d(%%oprreg %d), %%reg %d\n", n+(*nl), disp, oprreg, reg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                n[(*nl)++] = 0x48;
            } else {
                n[(*nl)++] = 0x49;
            }
            n[(*nl)++] = 0x8b;

            // reg combination
            regcomb = (oprreg << 16) + reg;
            switch (regcomb) {
                case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbb; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb3; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x93; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x83; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbf; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb7; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x97; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x87; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbe; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb6; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x96; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x86; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbd; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb5; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x95; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x85; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xba; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb2; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x92; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x82; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb8; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb0; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x90; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x80; break;
                default:
                                                            error("translate2x86_64_read_reg_file_base_disp: unsupported regcomb.\n");
            }
            // disp
            *((int32_t*)(n + (*nl))) = (int32_t)disp;
            *nl += SIZE_F;
        }
    } else {
        if (mattr == MATTR_SB || mattr == MATTR_UB) {
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                *nl += 0;
            }
            else {
                *nl += 1;
            }
            *nl += 1 + 1 + SIZE_F;
        }
        else {
            *nl += 3 + SIZE_F;
        }
    }
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_read_reg_file
(uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    if (mattr == MATTR_SB || mattr == MATTR_UB) {
        error("reg_file with SB/UB is not implemented yet.\n");
    }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        // movq oprreg, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq %%oprreg %d, %%reg %d\n", n+(*nl), oprreg, reg);
#endif
        if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
            // %rbx and %rcx are special
            n[(*nl)++] = 0x48;
        } else {
            n[(*nl)++] = 0x4c;
        }
        n[(*nl)++] = 0x89;

        // reg combination
        regcomb = (oprreg << 16) + reg;
        switch (regcomb) {
            case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xdf; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xde; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xda; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xd8; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xcf; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xce; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xca; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc8; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xff; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xfe; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xfa; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xf8; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf7; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf6; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xf2; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xf0; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xef; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xee; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xea; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xe8; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xd7; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xd6; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd2; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xd0; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xcf; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xce; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xca; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc8; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xc7; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xc6; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xc2; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc0; break;
            default:
                error("translate2x86_64_read_reg_file: unsupported regcomb.\n");
        }
    } else {
        *nl += 3;
    }
}

/* floating point only: implement D only currently
 * when mattr is set
 * reg is always eax/ax
 */
    inline void translate2x86_64_fp_read
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n)
{
    // TODO implement it
    int in_reg_file = 0;
    unsigned int mattr = 0;

#ifdef _DEBUG_MORE_
    zlogf("translate2x86_64_fp_read: addrm: %d\n", addrm);
#endif
    
    if (mattr == MATTR_SB || mattr == MATTR_UB) {
        error("_translate2x86_64_fp_read: B is not implemented yet.\n");
    }

    switch (addrm) {
        case ADDRM_IMMEDIATE:
            if (is_write_n) {
                if (mattr == MATTR_SF || mattr == MATTR_UF) {
                    // movl $opr, %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movl $%#lx, %%rax\n", n+(*nl), opr);
#endif
                    n[(*nl)++] = 0xb8;
                    // $opr
                    *((uint32_t*)(n + (*nl))) = (uint32_t)opr;
                    *nl += SIZE_F;
                }
                else {
                    // movq $opr, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq $%#lx, %%reg %d\n", n+(*nl), opr, reg);
#endif
                    n[(*nl)++] = 0x48;
                    // reg
                    switch (reg) {
                        case X86_64_R_RDI:
                            n[(*nl)++] = 0xbf;
                            break;
                        case X86_64_R_RSI:
                            n[(*nl)++] = 0xbe;
                            break;
                        case X86_64_R_RAX:
                            n[(*nl)++] = 0xb8;
                            break;
                        default:
                            error("translate2x86_64_fp_read: unsupported reg #1.\n");
                    }
                    // $opr
                    *((uint64_t*)(n + (*nl))) = opr;
                    *nl += SIZE_E;
                }
            } else {
                if (mattr == MATTR_SF || mattr == MATTR_UF) {
                    *nl += 1 + SIZE_F;
                }
                else {
                    *nl += 2 + SIZE_E;
                }
            }
            break;
        case ADDRM_ABSOLUTE:
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                translate2x86_64_read_reg_file(opr, reg, n, nl, is_write_n, mattr);
            } else {
                if (is_write_n) {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl opr, %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movl %#lx, %%eax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                    else {
                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // movq %rax, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movq %%rax, %%reg %d\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xc7;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xc6;
                                break;
                            case X86_64_R_RDX:
                                n[(*nl)++] = 0xc2;
                                break;
                                /*
                                   case X86_64_R_RCX:
                                   n[(*nl)++] = 0xc1;
                                   break;
                                   */
                            case X86_64_R_RAX:
                                n[(*nl)++] = 0xc0;
                                break;
                            default:
                                error("translate2x86_64_fp_read: unsupported reg #2.\n");
                        }
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 1 + SIZE_E;
                    }
                    else {
                        *nl += 2 + SIZE_E + 3;
                    }
                }
            }
            break;
        case ADDRM_INDIRECT:

            if (is_write_n) {
                // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0xa1;
                *((uint64_t*)(n + (*nl))) = opr;
                *nl += SIZE_E;
                if (mattr == MATTR_SF || mattr == MATTR_UF) {
                    // movl (%rax), %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movl (%%rax), %%eax %d\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x8b;
                    n[(*nl)++] = 0x00;
                }
                else {
                    // mov (%rax), reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx mov (%%rax), %%reg %d\n", n+(*nl), reg);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x8b;
                    // reg
                    switch (reg) {
                        case X86_64_R_RDI:
                            n[(*nl)++] = 0x38;
                            break;
                        case X86_64_R_RSI:
                            n[(*nl)++] = 0x30;
                            break;
                        default:
                            error("translate_read: unsupported reg #3.\n");
                    }
                }
            } else {
                if (mattr == MATTR_SF || mattr == MATTR_UF) {
                    *nl += 2 + SIZE_E + 2;
                }
                else {
                    *nl += 2 + SIZE_E + 3;
                }
            }
            break;
        case ADDRM_DISPLACEMENT:
            // check reg file
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                // TODO: implement float
                error("float: read base+disp mode use reg is not implemented yet. \n");
                // translate2x86_64_read_reg_file(opr, disp, reg, n, nl, is_write_n);
            } else {

                if (is_write_n) {
                    // movq $opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xa1;
                    *((uint64_t*)(n + (*nl))) = opr;
                    *nl += SIZE_E;
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // mov disp(%rax), %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %d(%%rax), %%eax %d\n", n+(*nl), disp);
#endif
                        n[(*nl)++] = 0x8b;
                        n[(*nl)++] = 0x80;

                    }
                    else {
                        // mov disp(%rax), reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %d(%%rax), %%reg %d\n", n+(*nl), disp, reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x8b;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xb8;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xb0;
                                break;
                            case X86_64_R_RAX:
                                n[(*nl)++] = 0x80;
                                break;
                            default:
                                error("translate_read: unsupported reg #1.\n");
                        }
                    }
                    // disp
                    *((uint32_t*)(n + (*nl))) = disp;
                    *nl += SIZE_F;
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + SIZE_E + 2 + SIZE_F;
                    }
                    else {
                        *nl += 2 + SIZE_E + 3 + SIZE_F;
                    }
                }
            }
            break;
        default:
            error("translate_read: undefined/unsupported ADDRM.\n");
    }
}


/* integer only
 * suppor t mattr
 * when mattr is set
 * reg is always eax/ax
 */
    inline void _translate2x86_64_read
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int in_reg_file = 0;
    int is_stdin = 0;

#ifdef _DEBUG_MORE_
    zlogf("translate2x86_64_read: addrm: %d\n", addrm);
#endif

    // safety check first
    if (mattr != MATTR_UNDEFINED && reg != X86_64_R_RAX) {
        error("_translate2x86_64_read: reg must be rax/ax when mattr is set.\n");
    }

    switch (addrm) {
        case ADDRM_IMMEDIATE:
            if (is_write_n) {
                if (mattr == MATTR_SF || mattr == MATTR_UF) {
                    // movl $opr, %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movl $%#lx, %%rax\n", n+(*nl), opr);
#endif
                    n[(*nl)++] = 0xb8;
                    // $opr
                    *((uint32_t*)(n + (*nl))) = (uint32_t)opr;
                    *nl += SIZE_F;
                } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                    // movb $opr, %al
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movb $%#lx, %%al\n", n+(*nl), (uint8_t)opr);
#endif
                    n[(*nl)++] = 0xb0;
                    // $opr
                    *((uint8_t*)(n + (*nl))) = (uint8_t)opr;
                    *nl += SIZE_B;
                }
                else {
                    // movq $opr, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq $%#lx, %%reg %d\n", n+(*nl), opr, reg);
#endif
                    n[(*nl)++] = 0x48;
                    // reg
                    switch (reg) {
                        case X86_64_R_RDI:
                            n[(*nl)++] = 0xbf;
                            break;
                        case X86_64_R_RSI:
                            n[(*nl)++] = 0xbe;
                            break;
                        case X86_64_R_RAX:
                            n[(*nl)++] = 0xb8;
                            break;
                        default:
                            error("_translate_read: unsupported reg #1.\n");
                    }
                    // $opr
                    *((uint64_t*)(n + (*nl))) = opr;
                    *nl += SIZE_E;
                }
            } else {
                if (mattr == MATTR_SF || mattr == MATTR_UF) {
                    *nl += 1 + SIZE_F;
                } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                    *nl += 1 + SIZE_B;
                }
                else {
                    *nl += 2 + SIZE_E;
                }
            }
            break;
        case ADDRM_ABSOLUTE:
            in_reg_file = 0;
            is_stdin = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            // check whether read from stdin
            is_stdin = check_is_stdin(opr, addrm);
            // reg file
            if (in_reg_file) {
                translate2x86_64_read_reg_file(opr, reg, n, nl, is_write_n, mattr);
            }
            // read from stdin
            else if (is_stdin) {
                if (is_write_n) {
                    // leaq 0(%rip), %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx leaq 0(%%rip), %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x8d;
                    n[(*nl)++] = 0x05;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;

                    // pushq %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx push %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x50;

                    // movq sys_stdin_q, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq sys_stdin_q, %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xa1;
                    *((uint64_t*)(n + (*nl))) = (uint64_t)(SYS_STDIN_Q);
                    *nl += SIZE_E;

                    // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0xff;
                    n[(*nl)++] = 0xe0;

                    // Note: %rax is poped by sys_new_runner
                    // and return value is in %rdi
 
                    // movq %rdi, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq %%rdi, %%reg %d\n", n+(*nl), reg);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x89;
                    // reg
                    switch (reg) {
                        case X86_64_R_RDI:
                            n[(*nl)++] = 0xff;
                            break;
                        case X86_64_R_RSI:
                            n[(*nl)++] = 0xfe;
                            break;
                        case X86_64_R_RDX:
                            n[(*nl)++] = 0xfa;
                            break;
                        case X86_64_R_RAX:
                            n[(*nl)++] = 0xf8;
                            break;
                        default:
                            error("_translate_read: unsupported reg #2.\n");
                    }
                } else {
                    *nl += 7 + 1 + 2 + SIZE_E + 2 + 3;
                }

            }
            // normal memory operation
            else {
                if (is_write_n) {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl opr, %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movl %#lx, %%eax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                         // movl opr, %al
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movb %#lx, %%al\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0xa0;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    } else {
                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // movq %rax, reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movq %%rax, %%reg %d\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xc7;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xc6;
                                break;
                            case X86_64_R_RDX:
                                n[(*nl)++] = 0xc2;
                                break;
                                /*
                                   case X86_64_R_RCX:
                                   n[(*nl)++] = 0xc1;
                                   break;
                                   */
                            case X86_64_R_RAX:
                                n[(*nl)++] = 0xc0;
                                break;
                            default:
                                error("_translate_read: unsupported reg #2.\n");
                        }
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 1 + SIZE_E;
                    } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 1 + SIZE_E;
                    }
                    else {
                        *nl += 2 + SIZE_E + 3;
                    }
                }
            }
            break;
        case ADDRM_INDIRECT:
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                translate2x86_64_read_indirect_reg_file(opr, reg, n, nl, is_write_n, mattr);
            } else {
                if (is_write_n) {
                    // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xa1;
                    *((uint64_t*)(n + (*nl))) = opr;
                    *nl += SIZE_E;
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl (%rax), %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movl (%%rax), %%eax %d\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x8b;
                        n[(*nl)++] = 0x00;
                    } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        // movl (%rax), %al
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movl (%%rax), %%al %d\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x8a;
                        n[(*nl)++] = 0x00;
                    }
                    else {
                        // mov (%rax), reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov (%%rax), %%reg %d\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x8b;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0x38;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0x30;
                                break;
                            default:
                                error("_translate_read: unsupported reg #3.\n");
                        }
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + SIZE_E + 2;
                    } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 2 + SIZE_E + 2;
                    }
                    else {
                        *nl += 2 + SIZE_E + 3;
                    }
                }
            }
            break;
        case ADDRM_DISPLACEMENT:
            // check reg file
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                // error("read base+disp mode use reg is not implemented yet. \n");
                translate2x86_64_read_reg_file_base_disp(opr, disp, reg, n, nl, is_write_n, mattr);
            } else {

                if (is_write_n) {
                    // movq $opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xa1;
                    *((uint64_t*)(n + (*nl))) = opr;
                    *nl += SIZE_E;
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // mov disp(%rax), %eax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %d(%%rax), %%eax %d\n", n+(*nl), disp);
#endif
                        n[(*nl)++] = 0x8b;
                        n[(*nl)++] = 0x80;

                    } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        // mov disp(%rax), %al
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %d(%%rax), %%al %d\n", n+(*nl), disp);
#endif
                        n[(*nl)++] = 0x8a;
                        n[(*nl)++] = 0x80;

                    }
                    else {
                        // mov disp(%rax), reg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %d(%%rax), %%reg %d\n", n+(*nl), disp, reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x8b;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xb8;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xb0;
                                break;
                            case X86_64_R_RAX:
                                n[(*nl)++] = 0x80;
                                break;
                            default:
                                error("_translate_read: unsupported reg #1.\n");
                        }
                    }
                    // disp
                    *((uint32_t*)(n + (*nl))) = disp;
                    *nl += SIZE_F;
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + SIZE_E + 2 + SIZE_F;
                    } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 2 + SIZE_E + 2 + SIZE_F;
                    }
                    else {
                        *nl += 2 + SIZE_E + 3 + SIZE_F;
                    }
                }
            }
            break;
        default:
            error("_translate_read: undefined/unsupported ADDRM.\n");
    }
}


// return register whether the destination is in
    inline int translate2x86_64_read
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int suggested_reg, char *n, uint64_t *nl, int is_write_n)
{

    _translate2x86_64_read(opr, disp, addrm, suggested_reg, n, nl, is_write_n, MATTR_UNDEFINED);
    return suggested_reg;
}

// old ones

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_write_reg_file_base_disp_0
(uint64_t opr, uint32_t disp, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("reg_file with SB/UB is not implemented yet.\n");
    // }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        if (mattr == MATTR_SF || mattr == MATTR_UF) {
            error("reg_file with SL/UL is not implemented\n");
        } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
            // movb al, disp(oprreg)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movb %d(%%oprreg %d), %%al %d\n", n+(*nl), disp, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                n[(*nl)++] = 0x41;
            }
            n[(*nl)++] = 0x88;

            // reg combination
            switch (oprreg) {
                case (I0_REG_FILE0): n[(*nl)++] = 0x83; break;
                case (I0_REG_FILE1): n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE2): n[(*nl)++] = 0x87; break;
                case (I0_REG_FILE3): n[(*nl)++] = 0x86; break;
                case (I0_REG_FILE4): n[(*nl)++] = 0x85; break;
                case (I0_REG_FILE5): n[(*nl)++] = 0x82; break;
                case (I0_REG_FILE6): n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE7): n[(*nl)++] = 0x80; break;
                default:
                                       error("translate2x86_64_read_reg_file_base_disp: SB/UB: unsupported oprreg.\n");
            }
            // disp
            *((int32_t*)(n + (*nl))) = (int32_t)disp;
            *nl += SIZE_F;

            // error("reg_file with SB/UB is not implemented\n");
        }
        else {
            // Q
            // movq reg, disp(oprreg)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%reg %d, %d(%%oprreg %d)\n", n+(*nl), reg, disp, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                n[(*nl)++] = 0x48;
            } else {
                n[(*nl)++] = 0x49;
            }
            n[(*nl)++] = 0x89;

            // reg combination
            regcomb = (oprreg << 16) + reg;
            switch (regcomb) {
                case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbb; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb3; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x93; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x83; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbf; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb7; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x97; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x87; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbe; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb6; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x96; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x86; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbd; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb5; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x95; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x85; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xba; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb2; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x92; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x82; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb8; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb0; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x90; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x80; break;
                default:
                                                            error("translate2x86_64_write_reg_file_base_disp: unsupported regcomb.\n");
            }

            // zlogf("Debug 1: %d\n", disp);
            // disp
            *((int32_t*)(n + (*nl))) = (int32_t)disp;
            *nl += SIZE_F;
        }
    } else {
        if (mattr == MATTR_SF || mattr == MATTR_UF) {
            error("reg_file with SL/UL is not implemented\n");
        } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                *nl += 0;
            }
            else {
                *nl += 1;
            }
            *nl += 1 + 1 + SIZE_F;

            // error("reg_file with SB/UB is not implemented\n");
        }
        else {
            // Q
            *nl += 3 + SIZE_F;
        }
    }
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_write_reg_file_0
(uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    if (mattr == MATTR_SB || mattr == MATTR_UB) {
        error("reg_file with SB/UB is not supported.\n");
    }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not supported.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        // movq reg, oprreg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq %%reg %d, %%oprreg %d\n", n+(*nl), reg, oprreg);
#endif
        if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
            // %rbx and %rcx are special
            n[(*nl)++] = 0x48;
        } else {
            n[(*nl)++] = 0x49;
        }
        n[(*nl)++] = 0x89;

        // reg combination
        regcomb = (oprreg << 16) + reg;
        switch (regcomb) {
            case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfb; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf3; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd3; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc3; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf9; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf1; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd1; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc1; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xff; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf7; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd7; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc7; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfe; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf6; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd6; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc6; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfd; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf5; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd5; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc5; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfa; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf2; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd2; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc2; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf9; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf1; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd1; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc1; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf8; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf0; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd0; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc0; break;
            default:
                error("translate2x86_64_write_reg_file: unsupported regcomb.\n");
        }
    } else {
        *nl += 3;
    }
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_write_indirect_reg_file_0
(uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("reg_file with SB/UB is not implemented yet.\n");
    // }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {

        if (mattr == MATTR_SB || mattr == MATTR_UB) {
            // movb %al, (oprreg)

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movb %%al %d, (%%oprreg %d)\n", n+(*nl), oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                n[(*nl)++] = 0x41;
            }
            n[(*nl)++] = 0x88;

            // reg combination
            switch (oprreg) {
                case (I0_REG_FILE0): n[(*nl)++] = 0x03; break;
                case (I0_REG_FILE1): n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE2): n[(*nl)++] = 0x07; break;
                case (I0_REG_FILE3): n[(*nl)++] = 0x06; break;
                case (I0_REG_FILE4): n[(*nl)++] = 0x45; n[(*nl)++] = 0x00; break; // %r13 is special
                case (I0_REG_FILE5): n[(*nl)++] = 0x02; break;
                case (I0_REG_FILE6): n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE7): n[(*nl)++] = 0x00; break;
                default:
                     error("translate2x86_64_write_indirect_reg_file: SB/UB: unsupported oprreg.\n");
            }

        }
        else {
            // UQ / SQ
            // movq reg, (oprreg)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%reg %d, (%%oprreg %d)\n", n+(*nl), reg, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                n[(*nl)++] = 0x48;
            } else {
                n[(*nl)++] = 0x49;
            }
            n[(*nl)++] = 0x89;

            // reg combination
            regcomb = (oprreg << 16) + reg;
            switch (regcomb) {
                case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3b; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x33; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x13; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x03; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3f; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x37; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x17; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x07; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3e; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x36; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x16; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x06; break;
                                                            // %r13 FILE4 is
                                                            // special here
                case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x7d; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x75; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x55; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x45; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3a; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x32; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x12; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x02; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x38; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x30; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x10; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x00; break;
                default:
                                                            error("translate2x86_64_write_indirect_reg_file: unsupported regcomb.\n");
            }
        }
    } else {
        if (mattr == MATTR_SB || mattr == MATTR_UB) {
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                (*nl)++;
            }
            (*nl) += 2;

            if (oprreg == I0_REG_FILE4) {
                // %r13 is special
                (*nl)++;
            }
        }
        else {
            *nl += 3;
            if (oprreg == I0_REG_FILE4) {
                *nl += 1;
            }
        }
    }
}



/*
 * suppor t mattr
 * when mattr is set
 * reg is always eax/al
 */
    inline void _translate2x86_64_write_0
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int in_reg_file = 0;
    int is_stdout = 0;

#ifdef _DEBUG_MORE_
    zlogf("translate2x86_64_write: addrm: %d\n", addrm);
#endif

    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("_translate2x86_64_write: B is not implemented yet.\n");
    // }

    if (mattr != MATTR_UNDEFINED && reg != X86_64_R_RAX) {
        error("_translate2x86_64_write: when mattr is used, reg should always be ax/rax\n");
    }

    switch (addrm) {
        case ADDRM_ABSOLUTE:
            in_reg_file = 0;
            is_stdout = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            // check whether write to stdout
            is_stdout = check_is_stdout(opr, addrm);
            if (in_reg_file) {
                translate2x86_64_write_reg_file_0(opr, reg, n, nl, is_write_n, mattr);
            } 
            // write to stdout
            else if (is_stdout) {
                if (is_write_n) {
                    // movq reg, %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq %%reg, %%rdi\n", n+(*nl), reg);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x89;
                    // reg
                    switch (reg) {
                        case X86_64_R_RDI:
                            n[(*nl)++] = 0xff;
                            break;
                        case X86_64_R_RSI:
                            n[(*nl)++] = 0xf7;
                            break;
                        case X86_64_R_RDX:
                            n[(*nl)++] = 0xd7;
                            break;
                        case X86_64_R_RAX:
                            n[(*nl)++] = 0xc7;
                            break;
                        default:
                            error("translate_write: unsupported reg for stdout.\n");
                    }

                    // leaq 0(%rip), %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx leaq 0(%%rip), %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x8d;
                    n[(*nl)++] = 0x05;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;

                    // pushq %rax
                    // Note: %rax is poped by caller
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx push %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x50;

                    // movq sys_stdout_q, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq sys_stdout_q, %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xa1;
                    *((uint64_t*)(n + (*nl))) = (uint64_t)(SYS_STDOUT_Q);
                    *nl += SIZE_E;

                    // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0xff;
                    n[(*nl)++] = 0xe0;
                } else {
                    *nl += 3 + 7 + 1 + 2 + SIZE_E + 2;
                }
            }
            // normal write op
            else {
                if (is_write_n) {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl %eax, opr
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movl %%eax, %#lx\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0xa3;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        // movl %al, opr
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movb %%al, %#lx\n", opr, n+(*nl));
#endif
                        n[(*nl)++] = 0xa2;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                    else {
                        // movabsq reg, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %%reg %d, %%rax\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xf8;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xf0;
                                break;
                            default:
                                error("translate_write: unsupported reg #2.\n");
                        }
                        // movq %rax, opr
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movq %%rax, %#lx\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa3;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 1 + SIZE_E;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 1 + SIZE_E;
                    }
                    else {
                        *nl += 3 + 2 + SIZE_E;
                    }
                }
            }
            break;
        case ADDRM_INDIRECT:
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                translate2x86_64_write_indirect_reg_file_0(opr, reg, n, nl, is_write_n, mattr);
            } else {
                if (is_write_n) {

                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl %eax, %edx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%eax %d, %%edx)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0xc2;
                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;

                        // mov %edx, (%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%edx %d, (%%rax)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0x10;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        // movl %al, %dl
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movb %%al %d, %%dl)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x88;
                        n[(*nl)++] = 0xc2;
                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;

                        // mov %dl, (%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%dl %d, (%%rax)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x88;
                        n[(*nl)++] = 0x10;
                    }
                    else {
                        // MATTR_SE or MATTR_UE


                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // mov reg, (%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%reg %d, (%%rax)\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0x38;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0x30;
                                break;
                            default:
                                error("translate_write: unsupported reg #3.\n");
                        }
                    }
                }
                else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + 2 + SIZE_E + 2;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 2 + 2 + SIZE_E + 2;
                    } 
                    else {
                        *nl += 2 + SIZE_E + 3;
                    }
                }
            }

            break;
        case ADDRM_DISPLACEMENT:
            // check reg file
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                // error("write base+disp mode use reg is not implemented yet. \n");
                translate2x86_64_write_reg_file_base_disp_0(opr, disp, reg, n, nl, is_write_n, mattr);
            } else {

                if (is_write_n) {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl %eax, %edx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%eax %d, %%edx)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0xc2;
                        // movq $opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // mov %edx, disp(%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%edx, %d(%%rax)\n", n+(*nl), disp, reg);
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0x90;
                        // disp
                        *((uint32_t*)(n + (*nl))) = disp;
                        *nl += SIZE_F;
                    }
                    else if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        error("base+disp without reg_file as base with SB/UB is not implemented yet.\n");
                        // TODO later: working here
                    }
                    else {
                        // movq $opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // mov reg, disp(%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%reg %d, %d(%%rax)\n", n+(*nl), disp, reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xb8;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xb0;
                                break;
                            case X86_64_R_RAX:
                                n[(*nl)++] = 0x80;
                                break;
                            default:
                                error("translate_write: unsupported reg #1.\n");
                        }
                        // disp
                        *((uint32_t*)(n + (*nl))) = disp;
                        *nl += SIZE_F;
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + 2 + SIZE_E + 2 + SIZE_F;
                    }
                    else if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        error("base+disp without reg_file as base with SB/UB is not implemented yet.\n");
                        // TODO later: working here
                    }
                    else {
                        *nl += 2 + SIZE_E + 3 + SIZE_F;
                    }
                }
            }
            break;
        case ADDRM_IMMEDIATE:
            error("translate_write: should never write to immediate addmr.\n");
            break;
        default:
            error("translate_write: undefined/unsupported ADDRM.\n");
    }
}

    inline void translate2x86_64_write_0
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n)
{
    _translate2x86_64_write_0(opr, disp, addrm, reg, n, nl, is_write_n, MATTR_UNDEFINED);
}



// new one

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_write_reg_file_base_disp
(uint64_t opr, uint32_t disp, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("reg_file with SB/UB is not implemented yet.\n");
    // }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        if (mattr == MATTR_SF || mattr == MATTR_UF) {
            error("reg_file with SL/UL is not implemented\n");
        } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
            // movb al, disp(oprreg)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movb %%al, %d(%%oprreg %d) %d\n", n+(*nl), disp, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                n[(*nl)++] = 0x41;
            }
            n[(*nl)++] = 0x88;

            // reg combination
            switch (oprreg) {
                case (I0_REG_FILE0): n[(*nl)++] = 0x83; break;
                case (I0_REG_FILE1): n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE2): n[(*nl)++] = 0x87; break;
                case (I0_REG_FILE3): n[(*nl)++] = 0x86; break;
                case (I0_REG_FILE4): n[(*nl)++] = 0x85; break;
                case (I0_REG_FILE5): n[(*nl)++] = 0x82; break;
                case (I0_REG_FILE6): n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE7): n[(*nl)++] = 0x80; break;
                default:
                                       error("translate2x86_64_read_reg_file_base_disp: SB/UB: unsupported oprreg.\n");
            }
            // disp
            *((int32_t*)(n + (*nl))) = (int32_t)disp;
            *nl += SIZE_F;

            // error("reg_file with SB/UB is not implemented\n");
        }
        else {
            // Q
            // movq reg, disp(oprreg)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%reg %d, %d(%%oprreg %d)\n", n+(*nl), reg, disp, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                n[(*nl)++] = 0x48;
            } else {
                n[(*nl)++] = 0x49;
            }
            n[(*nl)++] = 0x89;

            // reg combination
            regcomb = (oprreg << 16) + reg;
            switch (regcomb) {
                case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbb; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb3; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x93; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x83; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbf; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb7; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x97; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x87; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbe; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb6; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x96; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x86; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xbd; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb5; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x95; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x85; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xba; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb2; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x92; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x82; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb9; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb1; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x91; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x81; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xb8; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xb0; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x90; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x80; break;
                default:
                                                            error("translate2x86_64_write_reg_file_base_disp: unsupported regcomb.\n");
            }

            // zlogf("Debug 1: %d\n", disp);
            // disp
            *((int32_t*)(n + (*nl))) = (int32_t)disp;
            *nl += SIZE_F;
        }
    } else {
        if (mattr == MATTR_SF || mattr == MATTR_UF) {
            error("reg_file with SL/UL is not implemented\n");
        } else if (mattr == MATTR_SB || mattr == MATTR_UB) {
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                *nl += 0;
            }
            else {
                *nl += 1;
            }
            *nl += 1 + 1 + SIZE_F;

            // error("reg_file with SB/UB is not implemented\n");
        }
        else {
            // Q
            *nl += 3 + SIZE_F;
        }
    }
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_write_reg_file
(uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    if (mattr == MATTR_SB || mattr == MATTR_UB) {

        error("reg_file with SB/UB is not supported.\n");
        // TODO: A temporary implementation. Should be more efficient.
    }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not supported.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {
        // movq reg, oprreg
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq %%reg %d, %%oprreg %d\n", n+(*nl), reg, oprreg);
#endif
        if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
            // %rbx and %rcx are special
            n[(*nl)++] = 0x48;
        } else {
            n[(*nl)++] = 0x49;
        }
        n[(*nl)++] = 0x89;

        // reg combination
        regcomb = (oprreg << 16) + reg;
        switch (regcomb) {
            case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfb; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf3; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd3; break;
            case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc3; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf9; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf1; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd1; break;
            case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc1; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xff; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf7; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd7; break;
            case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc7; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfe; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf6; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd6; break;
            case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc6; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfd; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf5; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd5; break;
            case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc5; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xfa; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf2; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd2; break;
            case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc2; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf9; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf1; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd1; break;
            case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc1; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0xf8; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0xf0; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0xd0; break;
            case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0xc0; break;
            default:
                error("translate2x86_64_write_reg_file: unsupported regcomb.\n");
        }
    } else {
        *nl += 3;
    }
}

// when mattr is not Q: reg is always eax/al/rax
    inline void translate2x86_64_write_indirect_reg_file
(uint64_t opr, uint32_t reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int oprreg = -1;
    int regcomb = 0;
  
    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("reg_file with SB/UB is not implemented yet.\n");
    // }

    if (mattr == MATTR_SF || mattr == MATTR_UF) {
        error("reg_file with SL/UL is not implemented yet.\n");
    }

    // determine oprreg for the opr
    // tricky here: just map the REG_FILEX
    oprreg = (opr - REG_FILE_BEGIN)/8;

    if (is_write_n) {

        if (mattr == MATTR_SB || mattr == MATTR_UB) {
            // movb %al, (oprreg)

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movb %%al %d, (%%oprreg %d)\n", n+(*nl), oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                n[(*nl)++] = 0x41;
            }
            n[(*nl)++] = 0x88;

            // reg combination
            switch (oprreg) {
                case (I0_REG_FILE0): n[(*nl)++] = 0x03; break;
                case (I0_REG_FILE1): n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE2): n[(*nl)++] = 0x07; break;
                case (I0_REG_FILE3): n[(*nl)++] = 0x06; break;
                case (I0_REG_FILE4): n[(*nl)++] = 0x45; n[(*nl)++] = 0x00; break; // %r13 is special
                case (I0_REG_FILE5): n[(*nl)++] = 0x02; break;
                case (I0_REG_FILE6): n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE7): n[(*nl)++] = 0x00; break;
                default:
                     error("translate2x86_64_write_indirect_reg_file: SB/UB: unsupported oprreg.\n");
            }

        }
        else {
            // UQ / SQ
            // movq reg, (oprreg)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%reg %d, (%%oprreg %d)\n", n+(*nl), reg, oprreg);
#endif
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                n[(*nl)++] = 0x48;
            } else {
                n[(*nl)++] = 0x49;
            }
            n[(*nl)++] = 0x89;

            // reg combination
            regcomb = (oprreg << 16) + reg;
            switch (regcomb) {
                case (I0_REG_FILE0 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3b; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x33; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x13; break;
                case (I0_REG_FILE0 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x03; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
                case (I0_REG_FILE1 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3f; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x37; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x17; break;
                case (I0_REG_FILE2 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x07; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3e; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x36; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x16; break;
                case (I0_REG_FILE3 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x06; break;
                                                            // %r13 FILE4 is
                                                            // special here
                case (I0_REG_FILE4 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x7d; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x75; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x55; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE4 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x45; n[(*nl)++] = 0x00; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x3a; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x32; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x12; break;
                case (I0_REG_FILE5 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x02; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x39; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x31; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x11; break;
                case (I0_REG_FILE6 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x01; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDI: n[(*nl)++] = 0x38; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RSI: n[(*nl)++] = 0x30; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RDX: n[(*nl)++] = 0x10; break;
                case (I0_REG_FILE7 << 16) + X86_64_R_RAX: n[(*nl)++] = 0x00; break;
                default:
                                                            error("translate2x86_64_write_indirect_reg_file: unsupported regcomb.\n");
            }
        }
    } else {
        if (mattr == MATTR_SB || mattr == MATTR_UB) {
            if (oprreg == I0_REG_FILE0 || oprreg == I0_REG_FILE1) {
                // %rbx and %rcx are special
                ; 
            } else {
                (*nl)++;
            }
            (*nl) += 2;

            if (oprreg == I0_REG_FILE4) {
                // %r13 is special
                (*nl)++;
            }
        }
        else {
            *nl += 3;
            if (oprreg == I0_REG_FILE4) {
                *nl += 1;
            }
        }
    }
}



/*
 * suppor t mattr
 * when mattr is set
 * reg is always eax/al
 */
    inline void _translate2x86_64_write
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n, unsigned int mattr)
{
    int in_reg_file = 0;
    int is_stdout = 0;

#ifdef _DEBUG_MORE_
    zlogf("translate2x86_64_write: addrm: %d\n", addrm);
#endif

    // if (mattr == MATTR_SB || mattr == MATTR_UB) {
    //     error("_translate2x86_64_write: B is not implemented yet.\n");
    // }

    if (mattr != MATTR_UNDEFINED && reg != X86_64_R_RAX) {
        error("_translate2x86_64_write: when mattr is used, reg should always be ax/rax\n");
    }

    switch (addrm) {
        case ADDRM_ABSOLUTE:
            in_reg_file = 0;
            is_stdout = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            // check whether write to stdout
            is_stdout = check_is_stdout(opr, addrm);
            if (in_reg_file) {
                translate2x86_64_write_reg_file(opr, reg, n, nl, is_write_n, mattr);
            } 
            // write to stdout
            else if (is_stdout) {
                if (is_write_n) {
                    // movq reg, %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq %%reg, %%rdi\n", n+(*nl), reg);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x89;
                    // reg
                    switch (reg) {
                        case X86_64_R_RDI:
                            n[(*nl)++] = 0xff;
                            break;
                        case X86_64_R_RSI:
                            n[(*nl)++] = 0xf7;
                            break;
                        case X86_64_R_RDX:
                            n[(*nl)++] = 0xd7;
                            break;
                        case X86_64_R_RAX:
                            n[(*nl)++] = 0xc7;
                            break;
                        default:
                            error("translate_write: unsupported reg for stdout.\n");
                    }

                    // leaq 0(%rip), %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx leaq 0(%%rip), %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0x8d;
                    n[(*nl)++] = 0x05;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;
                    n[(*nl)++] = 0x00;

                    // pushq %rax
                    // Note: %rax is poped by caller
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx push %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x50;

                    // movq sys_stdout_q, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx movq sys_stdout_q, %%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xa1;
                    *((uint64_t*)(n + (*nl))) = (uint64_t)(SYS_STDOUT_Q);
                    *nl += SIZE_E;

                    // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
                    n[(*nl)++] = 0xff;
                    n[(*nl)++] = 0xe0;
                } else {
                    *nl += 3 + 7 + 1 + 2 + SIZE_E + 2;
                }
            }
            // normal write op
            else {
                if (is_write_n) {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl %eax, opr
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movl %%eax, %#lx\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0xa3;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        // movl %al, opr
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movb %%al, %#lx\n", opr, n+(*nl));
#endif
                        n[(*nl)++] = 0xa2;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                    else {
                        // movabsq reg, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %%reg %d, %%rax\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xf8;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xf0;
                                break;
                            default:
                                error("translate_write: unsupported reg #2.\n");
                        }
                        // movq %rax, opr
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movq %%rax, %#lx\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa3;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 1 + SIZE_E;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 1 + SIZE_E;
                    }
                    else {
                        *nl += 3 + 2 + SIZE_E;
                    }
                }
            }
            break;
        case ADDRM_INDIRECT:
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                translate2x86_64_write_indirect_reg_file(opr, reg, n, nl, is_write_n, mattr);
            } else {
                if (is_write_n) {

                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl %eax, %edx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%eax %d, %%edx)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0xc2;
                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;

                        // mov %edx, (%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%edx %d, (%%rax)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0x10;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        // movl %al, %dl
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movb %%al %d, %%dl)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x88;
                        n[(*nl)++] = 0xc2;
                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;

                        // mov %dl, (%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%dl %d, (%%rax)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x88;
                        n[(*nl)++] = 0x10;
                    }
                    else {
                        // MATTR_SE or MATTR_UE


                        // movabsq opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq %#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // mov reg, (%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%reg %d, (%%rax)\n", n+(*nl), reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0x38;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0x30;
                                break;
                            default:
                                error("translate_write: unsupported reg #3.\n");
                        }
                    }
                }
                else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + 2 + SIZE_E + 2;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        *nl += 2 + 2 + SIZE_E + 2;
                    } 
                    else {
                        *nl += 2 + SIZE_E + 3;
                    }
                }
            }

            break;
        case ADDRM_DISPLACEMENT:
            // check reg file
            in_reg_file = 0;
            // check whether using reg file
            in_reg_file = is_in_reg_file(opr);
            if (in_reg_file) {
                // error("write base+disp mode use reg is not implemented yet. \n");
                translate2x86_64_write_reg_file_base_disp(opr, disp, reg, n, nl, is_write_n, mattr);
            } else {

                if (is_write_n) {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        // movl %eax, %edx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%eax %d, %%edx)\n", n+(*nl));
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0xc2;
                        // movq $opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // mov %edx, disp(%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%edx, %d(%%rax)\n", n+(*nl), disp, reg);
#endif
                        n[(*nl)++] = 0x89;
                        n[(*nl)++] = 0x90;
                        // disp
                        *((uint32_t*)(n + (*nl))) = disp;
                        *nl += SIZE_F;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        error("base+disp without reg_file as base with SB/UB is not implemented yet.\n");
                        // TODO later: working here
                    }
                    else {
                        // movq $opr, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx movabsq $%#lx, %%rax\n", n+(*nl), opr);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0xa1;
                        *((uint64_t*)(n + (*nl))) = opr;
                        *nl += SIZE_E;
                        // mov reg, disp(%rax)
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                        zlogf("native instr: @%#lx mov %%reg %d, %d(%%rax)\n", n+(*nl), disp, reg);
#endif
                        n[(*nl)++] = 0x48;
                        n[(*nl)++] = 0x89;
                        // reg
                        switch (reg) {
                            case X86_64_R_RDI:
                                n[(*nl)++] = 0xb8;
                                break;
                            case X86_64_R_RSI:
                                n[(*nl)++] = 0xb0;
                                break;
                            case X86_64_R_RAX:
                                n[(*nl)++] = 0x80;
                                break;
                            default:
                                error("translate_write: unsupported reg #1.\n");
                        }
                        // disp
                        *((uint32_t*)(n + (*nl))) = disp;
                        *nl += SIZE_F;
                    }
                } else {
                    if (mattr == MATTR_SF || mattr == MATTR_UF) {
                        *nl += 2 + 2 + SIZE_E + 2 + SIZE_F;
                    }
                    else if (mattr == MATTR_SB || mattr == MATTR_UB) {
                        error("base+disp without reg_file as base with SB/UB is not implemented yet.\n");
                        // TODO later: working here
                    }
                    else {
                        *nl += 2 + SIZE_E + 3 + SIZE_F;
                    }
                }
            }
            break;
        case ADDRM_IMMEDIATE:
            error("translate_write: should never write to immediate addmr.\n");
            break;
        default:
            error("translate_write: undefined/unsupported ADDRM.\n");
    }
}

    inline void translate2x86_64_write
(uint64_t opr, uint32_t disp, unsigned int addrm, unsigned int reg, char *n, uint64_t *nl, int is_write_n)
{
    _translate2x86_64_write(opr, disp, addrm, reg, n, nl, is_write_n, MATTR_UNDEFINED);
}


void instr2x86_64_opr(unsigned int attr, unsigned int addrm, uint64_t opr, uint32_t disp, x86_64_opr_t *to) 
{
    to->class0 = X86_64_CLASS0_NOT_SUPPORTED;
    // only for SQ/UQ currently
    switch (attr) {
        case ATTR_SE:
        case ATTR_UE:
            break;
        default:
            error("instr2x86_64_opr: not implemented.\n");
            return;
    }

    switch (addrm) {
        case ADDRM_IMMEDIATE:
            if (within_32bit(opr)) {
                to->class0 = X86_64_CLASS0_NO_MEM;
                to->class1 = X86_64_CLASS1_IMM;
                to->opr = opr;
                to->addrm = X86_64_ADDRM_IMMEDIATE_SL;
            } else {
                // zlogf("not within_32bit.\n");
                return;
            }
            break;
        case ADDRM_ABSOLUTE:
            if (is_in_reg_file(opr)) {
                to->class0 = X86_64_CLASS0_NO_MEM;
                to->class1 = X86_64_CLASS1_REG;
                to->opr = opr_reg(opr);
                to->addrm = X86_64_ADDRM_REG;
            } else {
                return;
            }
            break;
        case ADDRM_INDIRECT:
            if (is_in_reg_file(opr)) {
                to->class0 = X86_64_CLASS0_MEM;
                to->class1 = X86_64_CLASS1_MEM_REG;
                to->opr = opr_reg(opr);
                to->addrm = X86_64_ADDRM_INDIRECT;
            } else {
                return;
            }
            break;
        case ADDRM_DISPLACEMENT:
            if (is_in_reg_file(opr)) {
                to->class0 = X86_64_CLASS0_MEM;
                to->class1 = X86_64_CLASS1_MEM_REG;
                to->opr = opr_reg(opr);
                to->disp = disp;
                to->addrm = X86_64_ADDRM_DISPLACEMENT;
            } else {
                return;
            }
            break;
        default:
            error("instr2x86_64_opr: not implemented.\n");
            return;
    }

    return;
}

int x86_64_encode_movq(int is_write_n, char *n, uint64_t *nl, x86_64_opr_t *opr1, x86_64_opr_t *opr2)
{

    int reg1_en = 0;
    int reg2_en = 0;
    x86_64_opr_t *tmp;
    char c;
    char opcode = 0;

#ifdef _DEBUG_MOVQ_
    zlogf("opr1: \n");
    print_x86_64_opr(opr1);
    zlogf("opr2: \n");
    print_x86_64_opr(opr2);
#endif

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("native instr: @%#lx movq A, B\n", n+(*nl));
    }
#endif

    if (opr1->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("with $imm -- ");
#endif
        if (is_write_n) {
            reg2_en = 0;
            reg1_en = x86_64_reg_encoding(opr2->opr);
            // encoding
            // REX.W
            c = 0x40;
            c += 0x01 << 3; // W
            // R 0
            // X 0
            c += (reg1_en & 0x8) >> 3; // B
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // Opcode
            n[(*nl)++] = 0xc7;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // ModRM
            switch (opr2->addrm) {
                case X86_64_ADDRM_REG:
                    c = 0xc0; // mod
                    break;
                case X86_64_ADDRM_INDIRECT:
                    c = 0x00; // mod
                    break;
                case X86_64_ADDRM_DISPLACEMENT:
                    c = 0x80; // mod
                    break;
                default:
                    error("Should not reach here.\n");
            } 
            // rrr 000
            c += (reg1_en & 0x7); // bbb
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
            // disp
            if (opr2->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *((uint32_t*)(n + (*nl))) = opr2->disp;
                *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
                zlogf("%x \n", n[(*nl)-4]);
#endif
            }
            // imm
            *((uint32_t*)(n + (*nl))) = *(uint32_t*)(&(opr1->opr));
            *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%x \n", n[(*nl)-4]);
#endif
        } else {
            *nl += 3 + 4;
            if (opr2->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *nl += 4;
            }
        }

    } else {
        if (opr1->addrm == X86_64_ADDRM_REG) {
            opcode = 0x89;
            // now exchange reg1/reg2
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("exchange it -- ");
#endif
            tmp = opr1;
            opr1 = opr2;
            opr2 = tmp;
        } else if (opr2->addrm == X86_64_ADDRM_REG) {
            opcode = 0x8b;
        } else {
            error("Should not reach here 5629.\n");
        }

        if (is_write_n) {
            reg1_en = x86_64_reg_encoding(opr1->opr); // B
            reg2_en = x86_64_reg_encoding(opr2->opr); // R
            // encoding
            // REX.W
            c = 0x40;
            c += 0x01 << 3; // W
            c += (reg2_en & 0x8) >> 1; // R
            // X 0
            c += (reg1_en & 0x8) >> 3; // B
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // Opcode
            n[(*nl)++] = opcode;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // ModRM
            switch (opr1->addrm) {
                case X86_64_ADDRM_REG:
                    c = 0xc0; // mod
                    break;
                case X86_64_ADDRM_INDIRECT:
                    c = 0x00; // mod
                    break;
                case X86_64_ADDRM_DISPLACEMENT:
                    c = 0x80; // mod
                    break;
                default:
                    error("Should not reach here.\n");
            } 

            c += (reg2_en & 0x7) << 3; // rrr
            c += (reg1_en & 0x7); // r/m
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif 
            // disp
            if (opr1->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *((uint32_t*)(n + (*nl))) = opr1->disp;
                *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
                zlogf("%x \n", n[(*nl)-4]);
#endif
            }

        } else {
            *nl += 3;
            if (opr1->addrm == X86_64_ADDRM_DISPLACEMENT) {
                *nl += 4;
            }
        }
    }

    return 0;
}

int opt_translate2x86_64_mov(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    // TODO: mov b to long and long to b
    uint32_t class0_comb = 0;
    x86_64_opr_t oprs[2];

    // optimize SQ and UQ only
    if ( ! (i->mattr1 == ATTR_SE || i->mattr1 == ATTR_UE) ) {
        return 0;
    }
    if ( ! (i->mattr2 == ATTR_SE || i->mattr2 == ATTR_UE) ) {
        return 0;
    }

    // print_instr(i);

    instr2x86_64_opr(i->mattr1, i->addrm1, i->opr1, i->disp1, &oprs[0]);
    instr2x86_64_opr(i->mattr2, i->addrm2, i->opr2, i->disp2, oprs+1);

    // print_x86_64_opr(&oprs[0]);
    // print_x86_64_opr(oprs+1);

    if (
            oprs[0].class0 == X86_64_CLASS0_NOT_SUPPORTED ||
            oprs[1].class0 == X86_64_CLASS0_NOT_SUPPORTED
       ) {
        return 0;
    }

    class0_comb = (oprs[0].class0 << 8) + oprs[1].class0;
    switch (class0_comb) {
        case (X86_64_CLASS0_NO_MEM << 8) + X86_64_CLASS0_MEM:
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//             if (is_write_n) {
//                 zlogf("@%#lx mov non_mem, mem\n", n+(*nl));
//             }
// #endif
            x86_64_encode_movq(is_write_n, n, nl, oprs, oprs+1);
            return 1;
            break;
        case (X86_64_CLASS0_NO_MEM << 8) + X86_64_CLASS0_NO_MEM:
            // return 0;
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//             if (is_write_n) {
//                 zlogf("@%#lx mov non_mem, non_mem\n", n+(*nl));
//             }
// #endif
            // zlogf("!!! Check here.\n");
            x86_64_encode_movq(is_write_n, n, nl, oprs, oprs+1);
            return 1;
            break;
        case (X86_64_CLASS0_MEM << 8) + X86_64_CLASS0_NO_MEM:
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//             if (is_write_n) {
//                 zlogf("@%#lx mov mem, non_mem\n", n+(*nl));
//             }
// #endif
            x86_64_encode_movq(is_write_n, n, nl, oprs, oprs+1);
            return 1;
            break;
        case (X86_64_CLASS0_MEM << 8) + X86_64_CLASS0_MEM:
            return 0; // since the default one can handle it well already
            break;
        default:
            break;
    }

//     // TODO: not implemnted yet
//     int oprreg1 = -1;
//     int oprreg2 = -1;
//     int oprregcomb = 0;
// 
//     // if there is optimization opportunity
//     // immediate -> oprreg
//     if ( 0 &&
//             (i->addrm1 == ADDRM_IMMEDIATE ) &&
//             (i->addrm2 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr2) ) && 
//             (i->mattr1 == MATTR_SE || i->mattr1 == MATTR_UE) && 
//             (i->mattr2 == MATTR_SE || i->mattr2 == MATTR_UE) 
//             ) {
//         // determine oprreg for the opr
//         // tricky here: just map the REG_FILEX
//         oprreg2 = (i->opr2 - REG_FILE_BEGIN)/8;
//         
//         if (is_write_n) {
// 
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//             zlogf("native instr: @%#lx movq $%#lx, %%oprreg %d\n", n+(*nl), i->opr1, oprreg2 );
// #endif
//             if (oprreg2 == I0_REG_FILE0 || oprreg2 == I0_REG_FILE1 ) {
//                 // %rbx and %rcx are different from others
//                 n[(*nl)++] = 0x48;
//             }
//             else {
//                 n[(*nl)++] = 0x49;
//             }
// 
//             switch (oprreg2) {
//                 case I0_REG_FILE0:
//                     n[(*nl)++] = 0xbb;
//                     break;
//                 case I0_REG_FILE1:
//                     n[(*nl)++] = 0xb9;
//                     break;
//                 case I0_REG_FILE2:
//                     n[(*nl)++] = 0xbf;
//                     break;
//                 case I0_REG_FILE3:
//                     n[(*nl)++] = 0xbe;
//                     break;
//                 case I0_REG_FILE4:
//                     n[(*nl)++] = 0xbd;
//                     break;
//                 case I0_REG_FILE5:
//                     n[(*nl)++] = 0xba;
//                     break;
//                 case I0_REG_FILE6:
//                     n[(*nl)++] = 0xb9;
//                     break;
//                 case I0_REG_FILE7:
//                     n[(*nl)++] = 0xb8;
//                     break;
//                 default: 
//                     error("translate2x86_64_mov imm -> reg: not implemented mov oprreg.\n");
//             }
// 
//             *((uint64_t*)(n + (*nl))) = i->opr1;
//             *nl += SIZE_E;
// 
//         }
//         else {
//             *nl += 2 + SIZE_E;
//         }
// 
//         // return
//         return 1;
//     }
// 
//     // reg to reg
//     if ( 0 &&  (i->mattr1 == MATTR_SE || i->mattr2 == MATTR_SE) && 
//             (i->addrm1 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr1) ) &&
//             (i->addrm2 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr2) )  ) {
//         // Two register operation
//         // determine oprreg for the opr
//         // tricky here: just map the REG_FILEX
//         oprreg1 = (i->opr1 - REG_FILE_BEGIN)/8;
//         oprreg2 = (i->opr2 - REG_FILE_BEGIN)/8;
// 
//         // reg combination
//         oprregcomb = (oprreg1 << 16) + oprreg2;
// 
//         if (is_write_n) {
//             switch (oprregcomb) {
//                 case (I0_REG_FILE0 << 16) + I0_REG_FILE1:
//                     /* add %rbx to %rcx */
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//                     zlogf("native instr: @%#lx add %%rbx, %%rcx\n", n+(*nl));
// #endif
//                     n[(*nl)++] = 0x48; 
//                     n[(*nl)++] = 0x01; 
//                     n[(*nl)++] = 0xd9; 
//                     break;
//                 default: error("translate2x86_64_mov: not implemented add oprregcomb.\n");
//             }
// 
//         }
//         else {
//             *nl += 3;
//         }
// 
//         // return
//         return 1;
//     }
// 
// 
//     /*
//     // immediate -> (oprreg)
//     if ( 0 &&  (i->mattr1 == MATTR_SE || i->mattr2 == MATTR_SE) && 
//             (i->addrm1 == ADDRM_IMMEDIATE ) &&
//             (i->addrm2 == ADDRM_INDIRECT && is_in_reg_file(i->opr2) )  ) {
//         // determine oprreg for the opr
//         // tricky here: just map the REG_FILEX
//         oprreg2 = (i->opr2 - REG_FILE_BEGIN)/8;
// 
//         // TODO other regs
//         
//         if (is_write_n) {
// 
// #ifdef _DEBUG_PRINT_NATIVE_INSTR_
//             zlogf("native instr: @%#lx movq $%#lx, (%%rcx)\n", n+(*nl), i->opr1 );
// #endif
//             if (oprreg2 == I0_REG_FILE1 || oprreg2 == I0_REG_FILE2 ) {
//                 // %rbx and %rcx are different from others
//                 n[(*nl)++] = 0x48;
//             }
//             else {
//                 n[(*nl)++] = 0x49;
//             }
// 
//             switch (oprreg2) {
//                 case I0_REG_FILE1:
//                     n[(*nl)++] = 0x48; 
//                     n[(*nl)++] = 0xd9;
//                     break;
//                 default: 
//                     error("translate2x86_64_mov imm -> reg: not implemented mov oprreg.\n");
//             }
// 
//             *((uint64_t*)(n + (*nl))) = i->opr1;
//             *nl += SIZE_E;
// 
//         }
//         else {
//             *nl += 2 + SIZE_E;
//         }
// 
//         // return
//         return 0;
// 
//     }
//     */
// 

    return 0;
}

int translate2x86_64_mov(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    int modes = 0;

    // optimization opportunities
    if (opt_translate2x86_64_mov(i, is_write_n, n, nl, t, tl)) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("mov: optimized.\n");
#endif
        return 0;
    }

    // // default one: fall back

    modes = ((i->mattr1) << 16) + (i->mattr2);

    switch (modes) {
        case (MATTR_SE << 16) + MATTR_SE:
        case (MATTR_SE << 16) + MATTR_UE:
        case (MATTR_UE << 16) + MATTR_SE:
        case (MATTR_UE << 16) + MATTR_UE:
        case (MATTR_FD << 16) + MATTR_FD:
            // read in opr1 to %rdi
            // READ(D1, %rdi)
            translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RDI, n, nl, is_write_n);
            // write %rdi to opr2
            // WRITE(%rdi, M)
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RDI, n, nl, is_write_n);
            break;
        case (MATTR_SE << 16) + MATTR_FD:
            // read in opr1 to %rax
            // READ(D1, %rax)
            translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n);

            // convert
            if (is_write_n) {
                // cvtsi2sdq   %rax, %xmm0
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx cvtsi2sdq %%rax, %%xmm0\n", n+(*nl));
#endif
                n[(*nl)++] = 0xf2;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x2a;
                n[(*nl)++] = 0xc0;

                // movq %xmm0, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq %%xmm0, %%rax\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x7e;
                n[(*nl)++] = 0xc0;
            } else {
                *nl += 10;
            }

            // write %rax to opr2
            // WRITE(%rax, M)
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);

            break;
        case (MATTR_FD << 16) + MATTR_SE:
            // read in opr1 to %rax
            // READ(D1, %rax)
            translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n);

            // convert
            if (is_write_n) {
                // movq %rax, %xmm0
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq %%rax, %%xmm0\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x6e;
                n[(*nl)++] = 0xc0;
                // cvttsd2siq  %xmm0, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx cvtsd2siq %%xmm0, %%rax\n", n+(*nl));
#endif
                n[(*nl)++] = 0xf2;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x2c;
                n[(*nl)++] = 0xc0;
            } else {
                *nl += 10;
            }

            // write %rax to opr2
            // WRITE(%rax, M)
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);

            break;

        case (MATTR_FS << 16) + MATTR_SE:
        case (MATTR_SE << 16) + MATTR_FS:
            error("translate2x86_64_mov: undefined/unsupported/unimplemented mattr combination. F<->SQ\n");
            break;

        case (MATTR_SF << 16) + MATTR_SE:
        case (MATTR_SF << 16) + MATTR_UE:
            // signed extension
            // read opr1 to %eax
            _translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n, i->mattr1);

            // signed extend %eax to %rax
            if (is_write_n) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx cltq\n", n+(*nl));
#endif
                // cltq
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x98;
            }
            else {
                *nl += 2;
            }

            // write %rax to opr2
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);

            break;
        case (MATTR_UF << 16) + MATTR_SE:
        case (MATTR_UF << 16) + MATTR_UE:
            // zero extension
            // read opr1 to %eax
            _translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n, i->mattr1);

            // write %rax to opr2
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);
           
            break;
        case (MATTR_SE << 16) + MATTR_SF:
        case (MATTR_SE << 16) + MATTR_UF:
        case (MATTR_UE << 16) + MATTR_SF:
        case (MATTR_UE << 16) + MATTR_UF:
            // read opr1 to %rax
            translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n);

            // write %eax to opr2
            _translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n, i->mattr2);
 
            break;
            /*
        case (MATTR_UB << 16) + MATTR_SE:
        case (MATTR_UB << 16) + MATTR_UE:
            // zero extension
            //
            // read opr1 to %eax
            // _translate2x86_64_read(i->opr1, i->disp1, i->addrm1, 0, n, nl, is_write_n, i->mattr1);

            // write %rax to opr2
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);
            */
        case (MATTR_SE << 16) + MATTR_SB:
        case (MATTR_SE << 16) + MATTR_UB:
        case (MATTR_UE << 16) + MATTR_SB:
        case (MATTR_UE << 16) + MATTR_UB:
            // read opr1 to %al
            translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n);

            // write %al to opr2
            _translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n, i->mattr2);
 
            break;
        case (MATTR_SB << 16) + MATTR_SE:
        // case (MATTR_SB << 16) + MATTR_UE:
            // signed extension
            //
            // read opr1 to %al
            _translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n, i->mattr1);

            // signed extend %al to %rax
            if (is_write_n) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movsbq %%al, %%rax\n", n+(*nl));
#endif
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0xbe;
                n[(*nl)++] = 0xc0;
            }
            else {
                *nl += 4;
            }

            // write %rax to opr2
            translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);
            break;
        case (MATTR_UB << 16) + MATTR_SB:
        case (MATTR_UB << 16) + MATTR_UB:
        case (MATTR_SB << 16) + MATTR_SB:
        case (MATTR_SB << 16) + MATTR_UB:
            // read opr1 to %al
            _translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RAX, n, nl, is_write_n, i->mattr1);

            // write %al to opr2
            // translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n);
            _translate2x86_64_write(i->opr2, i->disp2, i->addrm2, X86_64_R_RAX, n, nl, is_write_n, i->mattr2);
            break;
        default:
            error("translate2x86_64_mov: default: undefined/unsupported/unimplemented mattr combination.\n");
    }
    
    // TODO support more

    return 0;
}

#define RETURN_DECODE_STATUS(status, detail) \
	do{ DECODE_STATUS result= {status, detail}; return result;}while(0);

int translate2x86_64_nop(instr_t *i, int is_write_n,uint8_t *n, uint64_t *nl, char *t, uint64_t *tl);
int translate2x86_64_mov(instr_t *i, int is_write_n,char *n, uint64_t *nl, char *t, uint64_t *tl);
int translate2x86_64_alu_op(instr_t *i, int is_write_n,uint8_t *n, uint64_t *nl, char *t, uint64_t *tl);
int translate2x86_64_int(instr_t *i, int is_write_n,uint8_t *n, uint64_t *nl, char *t, uint64_t *tl);
int translate2x86_64_exit(instr_t *i, int is_write_n,uint8_t *n, uint64_t *nl, char *t, uint64_t *tl);

//extern uint8_t TempNativeCodeStore[1024];

/*DECODE_STATUS TranslateNOP_WR(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit)
{
	instr_t __instr;
	translate2x86_64_nop((&__instr), 1, tpc, nativelimit, 0, 0);
	RETURN_DECODE_STATUS(0,0);
}*/

DECODE_STATUS TranslateCONV_WR(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit)
{
	instr_t __instr;
	__instr.addr = (instr->addr);
	//__instr.mattr1 = (instr->attr);
	__instr.mattr2 = (instr->attr2);
	__instr.addrm1 = (instr->opr[0].addrm);
	__instr.opr1 = (instr->opr[0].val.v64);
	__instr.disp1 = (instr->opr[0].disp32);
	__instr.addrm2 = (instr->opr[1].addrm);
	__instr.opr2 = (instr->opr[1].val.v64);
	__instr.disp2 = (instr->opr[1].disp32);
	translate2x86_64_mov((&__instr), 1, ((char*)tpc), nativelimit, 0, 0);
	RETURN_DECODE_STATUS(0,0);
}

/*DECODE_STATUS TranslateALU_WR(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit)
{
	instr_t __instr;
	__instr.addr = (instr->addr);
	__instr.opcode = (instr->opcode);
	__instr.attr = (instr->attr);
	__instr.addrm1 = (instr->opr[0].addrm);
	__instr.opr1 = (instr->opr[0].val.v64);
	__instr.disp1 = (instr->opr[0].disp32);
	__instr.addrm2 = (instr->opr[1].addrm);
	__instr.opr2 = (instr->opr[1].val.v64);
	__instr.disp2 = (instr->opr[1].disp32);
	__instr.addrm3 = (instr->opr[2].addrm);
	__instr.opr3 = (instr->opr[2].val.v64);
	__instr.disp3 = (instr->opr[2].disp32);
	translate2x86_64_alu_op((&__instr), 1, tpc, nativelimit, 0, 0);
	RETURN_DECODE_STATUS(0,0);
}

DECODE_STATUS TranslateINT_WR(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit)
{
	instr_t __instr;
	__instr.opr1 =((uint64_t)(instr->opr[0].val.v8));
	translate2x86_64_int((&__instr), 1, tpc, nativelimit, 0, 0);
	RETURN_DECODE_STATUS(0,0);
}

DECODE_STATUS TranslateEXIT_WR(I0INSTR* instr, uint8_t* tpc, uint64_t* nativelimit)
{
	instr_t __instr;
	__instr.option = (instr->option);
	translate2x86_64_exit((&__instr), 1, tpc, nativelimit, 0, 0);
	RETURN_DECODE_STATUS(0,0);
}

*/
