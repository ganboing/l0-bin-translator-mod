#include "bin_translator.h"
#include "codec.h"
#include <stdio.h>

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

void instr2x86_64_oprs(instr_t *i, x86_64_opr_t *oprs, int n)
{
    if (n != 3) {
        error("instr2x86_64_oprs: only 3 for n is supported.\n");
    }

    instr2x86_64_opr(i->attr, i->addrm1, i->opr1, i->disp1, oprs);
    instr2x86_64_opr(i->attr, i->addrm2, i->opr2, i->disp2, oprs+1);
    instr2x86_64_opr(i->attr, i->addrm3, i->opr3, i->disp3, oprs+2);

    return;
}

int x86_64_encode_leaq_rr(int is_write_n, char *n, uint64_t *nl, int reg1, int reg2)
{
    int reg1_en = 0;
    int reg2_en = 0;
    char c;

    if (is_write_n) {
        reg1_en = x86_64_reg_encoding(reg1);
        reg2_en = x86_64_reg_encoding(reg2);
        // zlogf("reg1 %d. en: %2x \n", reg1, 0xff & reg1_en);
        // zlogf("reg2 %d. en: %2x \n", reg2, 0xff & reg2_en);
        // encoding
        // REX.W
        c = 0x40;
        c += 0x01 << 3; // W
        c += (reg2_en & 0x8) >> 1; // R
        c += (reg2_en & 0x8) >> 2; // X
        c += (reg1_en & 0x8) >> 3; // B
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
        // Opcode
        n[(*nl)++] = 0x8d;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
        // ModRM
        c = 0x00; // mod
        c += (reg2_en & 0x7) << 3; // rrr
        c += 0x04; // r/m
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
        c = 0x00; // ss 1 times
        c += (reg2_en & 0x7) << 3; // xxx
        c += (reg1_en & 0x7); // bbb
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
    } else {
        *nl += 4;
    }
    return 0;

    return 0;
}

// reg1 OP reg2 -> reg2
int x86_64_encode_addq_subq_rr(int is_add, int is_write_n, char *n, uint64_t *nl, int reg1, int reg2)
{
    int reg1_en = 0;
    int reg2_en = 0;
    char c;

    if (is_write_n) {
        reg1_en = x86_64_reg_encoding(reg1);
        reg2_en = x86_64_reg_encoding(reg2);
        // zlogf("reg1 %d. en: %2x \n", reg1, 0xff & reg1_en);
        // zlogf("reg2 %d. en: %2x \n", reg2, 0xff & reg2_en);
        // encoding
        // REX.W
        c = 0x40;
        c += 0x01 << 3; // W
        c += (reg1_en & 0x8) >> 1; // R
        c += (reg2_en & 0x8) >> 3; // B
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
        // Opcode
        if (is_add) {
            n[(*nl)++] = 0x01;
        } else {
            // sub
            n[(*nl)++] = 0x29;
        }
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
        // ModRM
        c = 0xc0; // mod
        c += (reg1_en & 0x7) << 3; // rrr
        c += (reg2_en & 0x7); // bbb
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
    } else {
        *nl += 3;
    }
    return 0;
}

inline int x86_64_encode_subq_rr(int is_write_n, char *n, uint64_t *nl, int reg1, int reg2)
{
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("opt native instr: @%#lx sub %%oprreg %d, %%oprreg %d\n", n+(*nl), reg1, reg2);
    }
#endif
    return x86_64_encode_addq_subq_rr(0, is_write_n, n, nl, reg1, reg2);
}

inline int x86_64_encode_addq_rr(int is_write_n, char *n, uint64_t *nl, int reg1, int reg2)
{
#ifdef BT_OPT_ADDQ_LEAQ
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("opt native instr: @%#lx leaq(%%oprreg %d, %%oprreg %d), %%oprreg %d\n", n+(*nl), reg1, reg2, reg2);
    }
#endif
    return x86_64_encode_leaq_rr(is_write_n, n, nl, reg1, reg2);
#else
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("opt native instr: @%#lx add %%oprreg %d, %%oprreg %d\n", n+(*nl), reg1, reg2);
    }
#endif
    return x86_64_encode_addq_subq_rr(1, is_write_n, n, nl, reg1, reg2);
#endif
}

// leaq imm(reg1) -> reg1
int x86_64_encode_leaq_ir(int is_write_n, char *n, uint64_t *nl, uint32_t imm, int reg1)
{
    // error("Not implemented yet.\n");
    int reg1_en = 0;
    char c;

    if (is_write_n) {
        reg1_en = x86_64_reg_encoding(reg1);
        // zlogf("reg1 %d. en: %2x \n", reg1, 0xff & reg1_en);
        // zlogf("reg2 %d. en: %2x \n", reg2, 0xff & reg2_en);
        // encoding
        // REX.W
        c = 0x40;
        c += 0x01 << 3; // W
        c += (reg1_en & 0x8) >> 1; // R
        // X = 0
        c += (reg1_en & 0x8) >> 3; // B
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        if (is_write_n) {
            zlogf("%2x ", 0xff & n[(*nl)-1]);
        }
#endif
        // Opcode
        n[(*nl)++] = 0x8d;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        if (is_write_n) {
            zlogf("%2x ", 0xff & n[(*nl)-1]);
        }
#endif
        // ModRM
        c = 0x80; // mod
        c += (reg1_en & 0x7) << 3; // rrr
        c += (reg1_en & 0x7); // bbb
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        if (is_write_n) {
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
        }
#endif
        // imm
        *((int32_t*)(n + (*nl))) = (int32_t)imm;
        *nl += SIZE_F;
    } else {
        *nl += 3 + SIZE_F;
    }
    return 0;
}

inline int x86_64_encode_addq_ir(int is_write_n, char *n, uint64_t *nl, uint32_t imm, int reg1)
{
#ifdef BT_OPT_ADDQ_LEAQ

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("opt native instr: @%#lx leaq(%%oprreg %d, %#x), %%oprreg %d\n", n+(*nl), reg1, imm, reg1);
    }
#endif
    return x86_64_encode_leaq_ir(is_write_n, n, nl, imm, reg1);

#else
    error("BT_OPT_ADDQ_LEAQ should always be defined.\n");
#endif
}

inline int x86_64_encode_subq_ir(int is_write_n, char *n, uint64_t *nl, uint32_t imm, int reg1)
{
    // error("Not implemented yet.\n");

    int reg1_en = 0;
    char c;

    if (is_write_n) {
        reg1_en = x86_64_reg_encoding(reg1);
        // zlogf("reg1 %d. en: %2x \n", reg1, 0xff & reg1_en);
        // zlogf("reg2 %d. en: %2x \n", reg2, 0xff & reg2_en);
        // encoding
        // REX.W
        c = 0x40;
        c += 0x01 << 3; // W
        // R = 0
        // X = 0
        c += (reg1_en & 0x8) >> 3; // B
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        if (is_write_n) {
            zlogf("%2x ", 0xff & n[(*nl)-1]);
        }
#endif
        // opcode
        // sub
        n[(*nl)++] = 0x81;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        if (is_write_n) {
            zlogf("%2x ", 0xff & n[(*nl)-1]);
        }
#endif
        // ModRM
        c = 0xc0; // mod
        c += (0x5) << 3; // rrr
        c += (reg1_en & 0x7); // bbb
        n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        if (is_write_n) {
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
        }
#endif
        // imm
        *((int32_t*)(n + (*nl))) = (int32_t)imm;
        *nl += SIZE_F;
    } else {
        *nl += 3 + SIZE_F;
    }
    return 0;
}

inline int x86_64_opr_equal(x86_64_opr_t *opr1, x86_64_opr_t *opr2)
{
    if (opr1 == opr2) return 1; // true if they are the same
    if (opr1->class0 != opr2->class0) return 0;
    if (opr1->class1 != opr2->class1) return 0;
    if (opr1->addrm != opr2->addrm) return 0;
    if (opr1->opr != opr2->opr) return 0;
    if (opr1->addrm == X86_64_ADDRM_DISPLACEMENT) {
        if (opr1->disp != opr2->disp) {
            return 0;
        }
    }
    return 1;
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

int x86_64_encode_leaq(int is_write_n, char *n, uint64_t *nl, x86_64_opr_t *opr1, x86_64_opr_t *opr2, x86_64_opr_t *opr3)
{
    int reg1_en = 0;
    int reg2_en = 0;
    int reg3_en = 0;
    char c;
    x86_64_opr_t imm;

    x86_64_opr_t *tmp = NULL;
    
    if (opr1->addrm == X86_64_ADDRM_IMMEDIATE_SL && opr2->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
        // error("leaq: imm, imm: should not reach here.\n");
        // calculate it and mov to destination
        imm.opr = opr1->opr + opr2->opr;
        if (within_32bit(imm.opr)) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("add: optimized with constant porpogation. 5801 \n");
#endif
            imm.class0 = opr1->class0;
            imm.class1 = opr1->class1;
            imm.addrm = opr1->addrm;
            // mov
            x86_64_encode_movq(is_write_n, n, nl, &imm, opr3);
            return 0;
        } else {
            error("leaq: imm, imm: should not reach here. 5803\n");
        }
    }

    if (opr3->addrm != X86_64_ADDRM_REG) {
        error("leaq: 3rd oprand should be a register.\n");
    }

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("native instr: @%#lx leaq A, B, C\n", n+(*nl));
    }
#endif
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("add: optimized with leaq \n");
#endif

    if (opr1->addrm == X86_64_ADDRM_IMMEDIATE_SL || opr2->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("with $imm -- ");
#endif
        if (opr1->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
            tmp = opr1;
            opr1 = opr2;
            opr2 = tmp;
        }
        // opr1: reg, opr2: imm, opr3: reg
        if (is_write_n) {
            reg1_en = x86_64_reg_encoding(opr1->opr);
            reg3_en = x86_64_reg_encoding(opr3->opr);
            // encoding
            // REX.W
            c = 0x40;
            c += 0x01 << 3; // W
            c += (reg3_en & 0x8) >> 1; // R
            // X 0
            c += (reg1_en & 0x8) >> 3; // B
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // Opcode
            n[(*nl)++] = 0x8d;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // ModRM
            c = 0x80; // mod
            c += (reg3_en & 0x7) << 3; // rrr
            c += (reg1_en & 0x7); // bbb
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
            *((uint32_t*)(n + (*nl))) = *(uint32_t*)(&(opr2->opr));
            *nl += 4;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%x \n", n[(*nl)-4]);
#endif
        } else {
            *nl += 3 + 4;
        }

    } else {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
        zlogf("(reg, reg) -> reg -- ");
#endif
        // opr1: reg, opr2: reg, opr3: reg
        if (is_write_n) {
            reg1_en = x86_64_reg_encoding(opr1->opr);
            reg2_en = x86_64_reg_encoding(opr2->opr);
            reg3_en = x86_64_reg_encoding(opr3->opr);
            // encoding
            // REX.W
            c = 0x40;
            c += 0x01 << 3; // W
            c += (reg3_en & 0x8) >> 1; // R
            c += (reg2_en & 0x8) >> 2; // X
            c += (reg1_en & 0x8) >> 3; // B
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // Opcode
            n[(*nl)++] = 0x8d;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            // ModRM
            if (opr1->opr == I0_REG_FILE4) {
                // %r13 must have displacement of 0 with mod 0
                c = 0x40; // mod
            } else {
                c = 0x00; // mod
            }
            c += (reg3_en & 0x7) << 3; // rrr
            c += 0x04; // r/m
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x ", 0xff & n[(*nl)-1]);
#endif
            c = 0x00; // ss 1 times
            c += (reg2_en & 0x7) << 3; // xxx
            c += (reg1_en & 0x7); // bbb
            n[(*nl)++] = c;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
            zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
            if (opr1->opr == I0_REG_FILE4) {
                // %r13 must have displacement of 0
                n[(*nl)++] = 0;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_BINARY_
                zlogf("%2x \n", 0xff & n[(*nl)-1]);
#endif
            }
        } else {
            *nl += 4;
            if (opr1->opr == I0_REG_FILE4) {
                // %r13 must have displacement of 0
                *nl += 1;
            }

        }
    }

    return 0;
}

int x86_64_encode_addq(int is_write_n, char *n, uint64_t *nl, x86_64_opr_t *opr1, x86_64_opr_t *opr2)
{
    int reg1_en = 0;
    int reg2_en = 0;
    x86_64_opr_t *tmp;
    char c;
    char opcode = 0;

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("native instr: @%#lx addq A, B\n", n+(*nl));
    }
#endif

    if (opr1->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
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
            n[(*nl)++] = 0x81;
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
            opcode = 0x01;
            // now exchange reg1/reg2
            tmp = opr1;
            opr1 = opr2;
            opr2 = tmp;
        } else if (opr2->addrm == X86_64_ADDRM_REG) {
            opcode = 0x03;
        } else {
            error("Should not reach here 5938.\n");
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

int x86_64_encode_subq(int is_write_n, char *n, uint64_t *nl, x86_64_opr_t *opr1, x86_64_opr_t *opr2)
{
    int reg1_en = 0;
    int reg2_en = 0;
    x86_64_opr_t *tmp;
    char c;
    char opcode = 0;

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    if (is_write_n) {
        zlogf("native instr: @%#lx subq A, B\n", n+(*nl));
    }
#endif

    if (opr1->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
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
            n[(*nl)++] = 0x81;
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
            c += (5 & 0x7) << 3; // rrr 5
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
            opcode = 0x29;
            // now exchange reg1/reg2
            tmp = opr1;
            opr1 = opr2;
            opr2 = tmp;
        } else if (opr2->addrm == X86_64_ADDRM_REG) {
            opcode = 0x2b;
        } else {
            error("Should not reach here 5938.\n");
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

// optimized: return 1
// otherwise return 0
int opt_translate2x86_64_sub(
        x86_64_opr_t *oprs, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    x86_64_opr_t *r = NULL;
    x86_64_opr_t *A = NULL;
    x86_64_opr_t *B = NULL;
    x86_64_opr_t *C = NULL;
    x86_64_opr_t opr_rdi;

    // %rdi
    opr_rdi.class0 = X86_64_CLASS0_NO_MEM;
    opr_rdi.class1 = X86_64_CLASS1_REG;
    opr_rdi.opr = X86_64_R_RDI;
    opr_rdi.addrm = X86_64_ADDRM_REG;

    A = oprs;
    B = oprs + 1;
    C = oprs + 2;

    // sub:sq A, B, C

    // C in RF && C is not in B
    if (C->addrm == X86_64_ADDRM_REG && C->opr != B->opr) {
        r = C;
    } else {
        r = &opr_rdi;
    }

    // A->r if A != r
    if (! x86_64_opr_equal(A, r)) {
        x86_64_encode_movq(is_write_n, n, nl, A, r);
    }

    // subq B, r
    x86_64_encode_subq(is_write_n, n, nl, B, r);

    // r->C if C != r
    if (! x86_64_opr_equal(C, r)) {
        x86_64_encode_movq(is_write_n, n, nl, r, C);
    }

    return 1;
}

// optimized: return 1
// otherwise return 0
int opt_translate2x86_64_add(
        x86_64_opr_t *oprs, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    x86_64_opr_t *opr1 = NULL;
    x86_64_opr_t *opr2 = NULL;
    x86_64_opr_t *opr3 = NULL;
    x86_64_opr_t *tmp;
    x86_64_opr_t opr_rdi;
    uint32_t class0_comb = 0;

    // %rdi
    opr_rdi.class0 = X86_64_CLASS0_NO_MEM;
    opr_rdi.class1 = X86_64_CLASS1_REG;
    opr_rdi.opr = X86_64_R_RDI;
    opr_rdi.addrm = X86_64_ADDRM_REG;

    class0_comb = (oprs[0].class0 << 16) + (oprs[1].class0 << 8) + (oprs[2].class0);

    opr1 = oprs;
    opr2 = oprs + 1;
    opr3 = oprs + 2;

    switch(class0_comb) {
        case (X86_64_CLASS0_MEM << 16) + (X86_64_CLASS0_MEM << 8) + (X86_64_CLASS0_MEM):
            // return 0; // debug
            x86_64_encode_movq(is_write_n, n, nl, opr1, &opr_rdi);
            x86_64_encode_addq(is_write_n, n, nl, opr2, &opr_rdi);
            x86_64_encode_movq(is_write_n, n, nl, &opr_rdi, opr3);
            return 1;
            break;
        case (X86_64_CLASS0_NO_MEM << 16) + (X86_64_CLASS0_MEM << 8) + (X86_64_CLASS0_MEM):
            // conduct to next situation
            opr1 = oprs + 1;
            opr2 = oprs;
        case (X86_64_CLASS0_MEM << 16) + (X86_64_CLASS0_NO_MEM << 8) + (X86_64_CLASS0_MEM):
            if (x86_64_opr_equal(opr1, opr3)) {
                // return 0; // debug
                x86_64_encode_addq(is_write_n, n, nl, opr2, opr3);
            } else {
                // return 0; // debug
                x86_64_encode_movq(is_write_n, n, nl, opr1, &opr_rdi);
                x86_64_encode_leaq(is_write_n, n, nl, &opr_rdi, opr2, &opr_rdi);
                x86_64_encode_movq(is_write_n, n, nl, &opr_rdi, opr3);
            }
            return 1;
            break;
        case (X86_64_CLASS0_MEM << 16) + (X86_64_CLASS0_NO_MEM << 8) + (X86_64_CLASS0_NO_MEM):
            // conduct to next situation
            opr1 = oprs + 1;
            opr2 = oprs;
        case (X86_64_CLASS0_NO_MEM << 16) + (X86_64_CLASS0_MEM << 8) + (X86_64_CLASS0_NO_MEM):
            // no reg (3rd oprand) in mem since we will change the reg
            if (opr3->opr == opr2->opr) {
                return 0;
            }
            // return 0; // debug
            x86_64_encode_movq(is_write_n, n, nl, opr1, opr3);
            x86_64_encode_addq(is_write_n, n, nl, opr2, opr3);
            return 1;
            break;
        case (X86_64_CLASS0_NO_MEM << 16) + (X86_64_CLASS0_NO_MEM << 8) + (X86_64_CLASS0_MEM):
            // return 0; // debug
            x86_64_encode_leaq(is_write_n, n, nl, opr1, opr2, &opr_rdi);
            x86_64_encode_movq(is_write_n, n, nl, &opr_rdi, opr3);
            return 1;
            break;
        case (X86_64_CLASS0_NO_MEM << 16) + (X86_64_CLASS0_NO_MEM << 8) + (X86_64_CLASS0_NO_MEM):
            // return 0; // debug
            x86_64_encode_leaq(is_write_n, n, nl, opr1, opr2, opr3);
            return 1;
            break;
        default:
            error("Should not reach here 5751\n");
            break;
    }

    return 0;
}

// optimized: return 1
// otherwise return 0
// only signed is supported
int opt_translate2x86_64_mul(
        instr_t *i,
        x86_64_opr_t *oprs, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    // return 0;

    x86_64_opr_t *r = NULL;
    x86_64_opr_t *A = NULL;
    x86_64_opr_t *B = NULL;
    x86_64_opr_t *C = NULL;
    x86_64_opr_t opr_rdi;

    instr_t tmpi;

    A = oprs;
    B = oprs + 1;
    C = oprs + 2;

    // make a copy
    tmpi = *i;

    // take care mul A, imm, C only
    if (B->addrm == X86_64_ADDRM_IMMEDIATE_SL) {
        // use shift
        switch (B->opr) {
            case 1:
                // tested okay
                // return 0;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("Optimized mul with mov.\n");
#endif
                // mov A->C
                tmpi.opcode = OP_CONV;
                tmpi.mattr1 = tmpi.attr;
                tmpi.mattr2 = tmpi.attr;
                tmpi.addrm2 = tmpi.addrm3;
                tmpi.opr2 = tmpi.opr3;
                tmpi.disp2 = tmpi.disp3;
                translate2x86_64_mov(&tmpi, is_write_n, n, nl, t, tl);
                return 1;
            case 2:
                tmpi.opr2 = 1;
                goto case_2_4_8_goto;
            case 4:
                tmpi.opr2 = 2;
                goto case_2_4_8_goto;
            case 8:
                tmpi.opr2 = 3;
case_2_4_8_goto:
                // return 0;
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("Optimized mul with shift. %d\n", tmpi.opr2);
#endif
                tmpi.opcode = OP_SHIFT;
                tmpi.option = OPT_SHIFT_L;
                tmpi.addrm2 = ADDRM_IMMEDIATE;
                translate2x86_64_shift(&tmpi, is_write_n, n, nl, t, tl);
                return 1;
            default:
                return 0;
        }
    }

    return 0;
}

// optimized: return 1
// otherwise return 0
int opt_translate2x86_64_alu_op(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    x86_64_opr_t oprs[4];

    // optimize SQ and UQ only
    if ( ! (i->attr == ATTR_SE || i->attr == ATTR_UE) ) {
        return 0;
    }
    // optimize ADD and SUB only
    if ( ! (i->opcode == OP_ADD || i->opcode == OP_SUB || i->opcode == OP_MUL) ) {
        return 0;
    }

    instr2x86_64_oprs(i, oprs, 3);

    if (
            oprs[0].class0 == X86_64_CLASS0_NOT_SUPPORTED ||
            oprs[1].class0 == X86_64_CLASS0_NOT_SUPPORTED ||
            oprs[2].class0 == X86_64_CLASS0_NOT_SUPPORTED
       ) {
        return 0;
    }

    switch (i->opcode) {
        case OP_ADD:
            return opt_translate2x86_64_add(oprs, is_write_n, n, nl, t, tl);
            break;
        case OP_SUB:
            return opt_translate2x86_64_sub(oprs, is_write_n, n, nl, t, tl);
            break;
        case OP_MUL:
            if ( i->attr == ATTR_SE ) {
                // optimize signed multiply only currently
                return opt_translate2x86_64_mul(i, oprs, is_write_n, n, nl, t, tl);
            } else {
                return 0;
            }
            break;
        default:
            return 0;
            break;
    }

    // old ones:
    //
    // int oprreg1 = -1;
    // int oprreg2 = -1;
    // int oprregcomb = 0;

    // // if there is optimization opportunity
    // // reg1 OP reg2 -> reg1 
    // // OP: add / sub
    // if ( (i->attr == ATTR_SE || i->attr == ATTR_UE) && 
    //         (i->addrm1 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr1) ) &&
    //         (i->addrm2 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr2) ) &&
    //         (i->opr1 == i->opr3) &&
    //         (i->opcode == OP_ADD || i->opcode == OP_SUB) ) {
    //     // Two register operation
    //     // determine oprreg for the opr
    //     // tricky here: just map the REG_FILEX
    //     oprreg1 = (i->opr1 - REG_FILE_BEGIN)/8;
    //     oprreg2 = (i->opr2 - REG_FILE_BEGIN)/8;

    //     // reg combination
    //     // oprregcomb = (oprreg1 << 16) + oprreg2;


    //     if (i->opcode == OP_ADD) {
    //         x86_64_encode_addq_rr(is_write_n, n, nl, oprreg2, oprreg1);
    //     } else if (i->opcode == OP_SUB) {
    //         x86_64_encode_subq_rr(is_write_n, n, nl, oprreg2, oprreg1);
    //     } else {
    //         error("Should not get here. 4827");
    //     }

    //     // return
    //     return 1;
    // }
    // // reg1 OP imm -> reg1 
    // // OP: add / sub
    // else if ( (i->attr == ATTR_SE || i->attr == ATTR_UE) && 
    //         (i->addrm1 == ADDRM_ABSOLUTE && is_in_reg_file(i->opr1) ) &&
    //         ((i->addrm2 == ADDRM_IMMEDIATE) && ! (i->opr2 & 0xFFFFFFFF00000000) /* 32bit only */ ) &&
    //         (i->opr1 == i->opr3) &&
    //         (i->opcode == OP_ADD || i->opcode == OP_SUB) ) {
    //     // determine oprreg for the opr
    //     // tricky here: just map the REG_FILEX
    //     oprreg1 = (i->opr1 - REG_FILE_BEGIN)/8;

    //     if (i->opcode == OP_ADD) {
    //         // return 0; // i0ble it
    //         x86_64_encode_addq_ir(is_write_n, n, nl, (uint32_t)(i->opr2), oprreg1);
    //     } else if (i->opcode == OP_SUB) {
    //         // return 0; // i0ble it
    //         x86_64_encode_subq_ir(is_write_n, n, nl, (uint32_t)(i->opr2), oprreg1);
    //     } else {
    //         error("Should not get here. 5011");
    //     }

    //     // return
    //     return 1;
    // }

    return 0;
}

int translate2x86_64_alu_op(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{

    if (i->attr == ATTR_FS) {
        error("translate2x86_64_op: ATTR_FS not implemented yet.\n");
    }

    // optimization opportunities
    if (opt_translate2x86_64_alu_op(i, is_write_n, n, nl, t, tl)) {
        return 0;
    }

    // // default one: handle all situation

    // READ(D1, %rdi)
    translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RDI, n, nl, is_write_n);
    // READ(D2, %rsi)
    translate2x86_64_read(i->opr2, i->disp2, i->addrm2, X86_64_R_RSI, n, nl, is_write_n);

    if ( (i->attr == ATTR_SE || i->attr == ATTR_UE) )
    {
        // optimization for add: use leq instead
#ifdef BT_OPT_ADDQ_LEAQ
        if (i->opcode == OP_ADD) {
            if (is_write_n) {
                // leaq (%rsi, %rdi), %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx leaq (%%rsi, %%rdi), %%rdi\n", n+(*nl));
#endif

                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x8d;
                n[(*nl)++] = 0x3c;
                n[(*nl)++] = 0x3e;
            } else {
                *nl += 4;
            }
        }
        else {
#endif
        // others
        // default for add

            if (is_write_n) {
                // op (integer) %rsi, %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx op (integer) %%rsi, %%rdi\n", n+(*nl));
#endif
                n[(*nl)++] = 0x48;
                if (i->opcode == OP_MUL) {
                    n[(*nl)++] = 0x0f;
                    n[(*nl)++] = 0xaf;
                    n[(*nl)++] = 0xfe;
                } else {
                    switch(i->opcode) {
                        case OP_ADD:
                            n[(*nl)++] = 0x01;
                            break;
                        case OP_SUB:
                            n[(*nl)++] = 0x29;
                            break;
                        case OP_AND:
                            n[(*nl)++] = 0x21;
                            break;
                        case OP_OR:
                            n[(*nl)++] = 0x09;
                            break;
                        case OP_XOR:
                            n[(*nl)++] = 0x31;
                            break;
                        default:
                            ; // should never reach here
                    }

                    n[(*nl)++] = 0xf7;
                }
            } else {
                if (i->opcode == OP_MUL) {
                    *nl += 4;
                } else {
                    *nl += 3;
                }
            }
#ifdef BT_OPT_ADDQ_LEAQ
        }
#endif
    }
    else if (i->attr == ATTR_FD) {
        if (is_write_n) {

            if (i->opcode == OP_SUB) {
                //  movq %rdi, %xmm1
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq %%rdi, %%xmm1\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x6e;
                n[(*nl)++] = 0xcf;

                //  movq %rsi, %xmm0
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq %%rsi, %%xmm0\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x6e;
                n[(*nl)++] = 0xc6;
                
            }
            else {
                //  movq %rdi, %xmm0
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq %%rdi, %%xmm0\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x6e;
                n[(*nl)++] = 0xc7;
                
                //  movq %rsi, %xmm1
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx movq %%rsi, %%xmm1\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x6e;
                n[(*nl)++] = 0xce;
            }

            //  op %xmm0, %xmm1
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx op () %%xmm0, %%xmm1\n", n+(*nl));
#endif
                n[(*nl)++] = 0xf2;
                n[(*nl)++] = 0x0f;

            switch(i->opcode) {
                case OP_ADD:
                    n[(*nl)++] = 0x58;
                    break;
                case OP_MUL:
                    n[(*nl)++] = 0x59;
                    break;
                case OP_SUB:
                    n[(*nl)++] = 0x5c;
                    break;
                default:
                    // should never reach here
                    error("translate2x86_64_op: not supported opcode for D.\n");
            }
            n[(*nl)++] = 0xc8;

            //  movq %xmm1, %rdi
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
            zlogf("native instr: @%#lx movq %%xmm1, %%rdi\n", n+(*nl));
#endif
                n[(*nl)++] = 0x66;
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0x0f;
                n[(*nl)++] = 0x7e;
                n[(*nl)++] = 0xcf;

        }
        else {
            *nl += 5 + 5 + 4 + 5;
        }
    }

    // WRITE(%rdi, M)
    translate2x86_64_write(i->opr3, i->disp3, i->addrm3, X86_64_R_RDI, n, nl, is_write_n);

    return 0;
}

// Note: only the least significant 8 bits of opr2 are used.
int translate2x86_64_shift(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    // check addrm2
    if (i->addrm2 != ADDRM_IMMEDIATE) {
        error("translate2x86_64_op: SHIFT only support addrm2 == ADDRM_IMMEDIATE.\n");
    }
   
    // READ(D1, %rdi)
    translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RDI, n, nl, is_write_n);

    if ( (i->attr == ATTR_SE || i->attr == ATTR_UE) )
    {
        if (is_write_n) {
            // shift (l/r) $opr2, %rdi
            if (i->option == OPT_SHIFT_L) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                zlogf("native instr: @%#lx shl $%#lx, %%rdi\n", n+(*nl), i->opr2 & 0xff);
#endif
                n[(*nl)++] = 0x48;
                n[(*nl)++] = 0xc1;
                n[(*nl)++] = 0xe7;
                n[(*nl)++] = (char)(i->opr2 & 0xff);
            }
            else if (i->option == OPT_SHIFT_R) {
                if (i->attr == ATTR_UE) {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx shr $%#lx, %%rdi\n", n+(*nl), i->opr2 & 0xff);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xc1;
                    n[(*nl)++] = 0xef;
                    n[(*nl)++] = (char)(i->opr2 & 0xff);
                }
                else {
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
                    zlogf("native instr: @%#lx sar $%#lx, %%rdi\n", n+(*nl), i->opr2 & 0xff);
#endif
                    n[(*nl)++] = 0x48;
                    n[(*nl)++] = 0xc1;
                    n[(*nl)++] = 0xff;
                    n[(*nl)++] = (char)(i->opr2 & 0xff);
                }
            }
            else {
                error("translate2x86_64_op: SHIFT not supported option.\n");
            }
        } else {
            *nl += 4;
        }
    }
    else {
        // error 
        error("translate2x86_64_op: SHIFT only support SQ or UQ.\n");
    }

    // WRITE(%rdi, M)
    translate2x86_64_write(i->opr3, i->disp3, i->addrm3, X86_64_R_RDI, n, nl, is_write_n);

    return 0;
}


int translate2x86_64_spawn(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    // READ(M1, %rdi)
    translate2x86_64_read(i->opr1, i->disp1, i->addrm1, X86_64_R_RDI, n, nl, is_write_n);
    // READ(M2, %rsi)
    translate2x86_64_read(i->opr2, i->disp2, i->addrm2, X86_64_R_RSI, n, nl, is_write_n);
    // READ(M3, %rdx)
    translate2x86_64_read(i->opr3, i->disp3, i->addrm3, X86_64_R_RDX, n, nl, is_write_n);
    // READ(M4, %rax)
    translate2x86_64_read(i->opr4, i->disp4, i->addrm4, X86_64_R_RAX, n, nl, is_write_n);
    if (is_write_n) {
        // save %rcx as it is in register file
        // pushq %rcx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx push %%rcx\n", n+(*nl));
#endif
        n[(*nl)++] = 0x51;

        // movq %rax, %rcx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq %%rax, %%rcx\n", n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0x89;
        n[(*nl)++] = 0xc1;
        
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
       
        // movq sys_new_runner, %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq sys_new_runner, %%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0xa1;
        *((uint64_t*)(n + (*nl))) = (uint64_t)(SYS_NEW_RUNNER);
        *nl += SIZE_E;

        // jmpq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx jmpq *%%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0xff;
        n[(*nl)++] = 0xe0;

        // Note: %rax is poped by sys_new_runner
        //
        // pop %rcx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx pop %%rcx\n", n+(*nl));
#endif
        n[(*nl)++] = 0x59;

    } else {
        *nl += 1 + 3 + 7 + 1 + 2 + SIZE_E + 2 + 1;
    }

    return 0;
}

int translate2x86_64_strcmp(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    x86_64_opr_t oprs[5];
    x86_64_opr_t reg;

    new_instr2x86_64_oprs(i, oprs, 5);

#ifdef _DEBUG_SCMP_
    int debug_i;
    for (debug_i = 0; debug_i < 5; debug_i++) {
        print_x86_64_opr(oprs+debug_i);
    }
#endif

    if (is_write_n) {

        // save %rcx as it is in register file
        // pushq %rcx
        // pushq %r10
        // pushq %r9
        // pushq %r8
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx push %%rcx\n", n+(*nl));
#endif
        n[(*nl)++] = 0x51;

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx push %%r10\n", n+(*nl));
#endif
        n[(*nl)++] = 0x41;
        n[(*nl)++] = 0x52;

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx push %%r9\n", n+(*nl));
#endif
        n[(*nl)++] = 0x41;
        n[(*nl)++] = 0x51;

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx push %%r8\n", n+(*nl));
#endif
        n[(*nl)++] = 0x41;
        n[(*nl)++] = 0x50;
    } else {
        *nl += 1 + 2 + 2 + 2;
    }

#ifdef _DEBUG_PRINT_NATIVE_INSTR_
    zlogf("native instr:     to %%rdi, %%rsi, %%rdx, %%rcx\n", n+(*nl));
#endif

    reg.class0 = X86_64_CLASS0_NO_MEM;
    reg.class1 = X86_64_CLASS1_REG;
    reg.addrm = X86_64_ADDRM_REG;

    reg.opr = X86_64_R_RDI;
    x86_64_encode_movq(is_write_n, n, nl, oprs, &reg);
    reg.opr = X86_64_R_RSI;
    x86_64_encode_movq(is_write_n, n, nl, oprs+1, &reg);
    reg.opr = X86_64_R_RDX;
    x86_64_encode_movq(is_write_n, n, nl, oprs+2, &reg);
    reg.opr = X86_64_R_RCX;
    x86_64_encode_movq(is_write_n, n, nl, oprs+3, &reg);

    if (is_write_n) {
        // mov strcmp %rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx movq $%#lx, %%rax\n", n+(*nl), _strncmp);
#endif
        n[(*nl)++] = 0x48;
        n[(*nl)++] = 0xb8;
        *((uint64_t*)(n + *nl)) = (uint64_t)_strncmp;
        *nl += SIZE_E;
        // callq *%rax
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx callq *%%rax\n", n+(*nl));
#endif
        n[(*nl)++] = 0xff;
        n[(*nl)++] = 0xd0;
    } else {
        *nl += 2 + SIZE_E + 2;
    }

    if (is_write_n) {
        // pop %r8
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx pop %%r10\n", n+(*nl));
#endif
        n[(*nl)++] = 0x41;
        n[(*nl)++] = 0x58;

        // pop %r9
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx pop %%r9\n", n+(*nl));
#endif
        n[(*nl)++] = 0x41;
        n[(*nl)++] = 0x59;

        // pop %r10
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx pop %%r8\n", n+(*nl));
#endif
        n[(*nl)++] = 0x41;
        n[(*nl)++] = 0x5a;

        // pop %rcx
#ifdef _DEBUG_PRINT_NATIVE_INSTR_
        zlogf("native instr: @%#lx pop %%rcx\n", n+(*nl));
#endif
        n[(*nl)++] = 0x59;

    } else {
        *nl += 2 + 2 + 2 + 1;
    }

    // result
    reg.opr = X86_64_R_RAX;
    x86_64_encode_movq(is_write_n, n, nl, &reg, oprs+4);

    // error("strcmp: not implemented yet.\n");
    return 0;
}

int translate2x86_64_(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    return 0;
}

// n: native code base
// nl: native code current len
int translate2x86_64(
        instr_t *i, int is_write_n,
        char *n, uint64_t *nl, char *t, uint64_t *tl)
{
    switch (i->opcode) {
        case OP_NOP:
            translate2x86_64_nop(i, is_write_n, n, nl, t, tl);
            break;
        case OP_B:
            translate2x86_64_br(i, is_write_n, n, nl, t, tl);
            break;
        case OP_CONV:
// #ifdef _CONV_V0_
//            translate2x86_64_mov_v0(i, is_write_n, n, nl, t, tl);
// #else
            translate2x86_64_mov(i, is_write_n, n, nl, t, tl);
// #endif
            break;
        case OP_INT:
            translate2x86_64_int(i, is_write_n, n, nl, t, tl);
            break;
        case OP_EXIT:
            translate2x86_64_exit(i, is_write_n, n, nl, t, tl);
            break;
        case OP_ADD:
        case OP_SUB:
        case OP_AND:
        case OP_OR:
        case OP_XOR:
        case OP_MUL:
            translate2x86_64_alu_op(i, is_write_n, n, nl, t, tl);
            break;
        case OP_DIV:
            translate2x86_64_div(i, is_write_n, n, nl, t, tl);
            break;
        case OP_SHIFT:
            translate2x86_64_shift(i, is_write_n, n, nl, t, tl);
            break;
        case OP_SPAWN:
            translate2x86_64_spawn(i, is_write_n, n, nl, t, tl);
            break;
        case OP_SCMP:
            translate2x86_64_strcmp(i, is_write_n, n, nl, t, tl);
            break;
        case OP_UNDEFINED:
        default:
            zlogf_time("Translate: undefined/unsupported opcode: %#lx\n", i->opcode);
            warning("Translate: undefined/unsupported opcode\n");
            break;
    }

    return 0;
}
