#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <memory.h>
#include <setjmp.h>
#include "bin_translator.h"
#include "I0Symbol.h"
#include "ASM_MACROS.h"
#include "PrintI0Template.h"

//#define _IN_
//#define _OUT_

typedef struct _DECODE_STATUS{
	unsigned long status;
	unsigned long detail;
}DECODE_STATUS;

#define RETURN_DECODE_STATUS(status, detail) \
	do{ DECODE_STATUS result= {status, detail}; return result;}while(0);

jmp_buf decode_longjmp_buf;

typedef void (*DECODE_OP_IM_FUNC_PTR)(char**,I0OPR*);


static void DecodeOprI8(char** codeptr,I0OPR* opr) {opr->val=*((uint8_t*)(*codeptr)); (*codeptr)+=4;}
static void DecodeOprI32(char** codeptr,I0OPR* opr) {opr->val=*((uint32_t*)(*codeptr)); (*codeptr)+=4;}
static void DecodeOprI64(char** codeptr,I0OPR* opr) {opr->val=*((uint64_t*)(*codeptr)); (*codeptr)+=8;}
static void DecodeOprIUndefined(char** codeptr,I0OPR* opr){(void)codeptr; (void)opr; longjmp(decode_longjmp_buf,OPR_I_MATTR_UNDEFINED);}
static void DecodeOprINotImplement(char** codeptr,I0OPR* opr){(void)codeptr; (void)opr; longjmp(decode_longjmp_buf,OPR_I_MATTR_NOTIMPLEMENT);}

static void DecodeOprMAbsOrIndirect(char** codeptr,I0OPR* opr){opr->val=*((uint64_t*)(*codeptr)); (*codeptr)+=8;}
static void DecodeOprMDisplacement(char** codeptr,I0OPR* opr){opr->disp32=*((uint32_t*)(*codeptr)); (*codeptr)+=4; opr->val=*((uint64_t*)(*codeptr)); (*codeptr)+=8;}
static void DecodeOprMUndefined(char** codeptr,I0OPR* opr){(void)codeptr; (void)opr; longjmp(decode_longjmp_buf,OPR_M_ADDRM_UNDEFINED);}
static void DecodeOprMException(char** codeptr,I0OPR* opr){(void)codeptr; (void)opr; printf("opr.addrm==%x\n",opr->addrm); longjmp(decode_longjmp_buf,OPR_M_ADDRM_EXCEPTION);}

static const DECODE_OP_IM_FUNC_PTR decode_op_i_ptr[1<<BIT_LEN_MATTR]=
{
	DecodeOprI8,			//SB
	DecodeOprI64, 			//SE
	DecodeOprINotImplement,	//SS
	DecodeOprI32,			//SF
	DecodeOprI8,			//UB
	DecodeOprI64,			//UE
	DecodeOprINotImplement,	//US
	DecodeOprI32,			//UF
	DecodeOprI32,			//FS
	DecodeOprI64,			//FD
	DecodeOprIUndefined,	//0x0a
	DecodeOprIUndefined,	//0x0b
	DecodeOprIUndefined,	//0x0c
	DecodeOprIUndefined,	//0x0d
	DecodeOprIUndefined,	//0x0e
	DecodeOprIUndefined		//0x0f
};

static const DECODE_OP_IM_FUNC_PTR decode_op_m_ptr[1<<BIT_LEN_ADDRM]=
{
	DecodeOprMException,		//IMM
	DecodeOprMAbsOrIndirect,	//ABS
	DecodeOprMAbsOrIndirect,	//INDIRECT
	DecodeOprMDisplacement,		//DISPLACEMENT
	DecodeOprMUndefined,		//0x04
	DecodeOprMUndefined,		//0x05
	DecodeOprMUndefined,		//0x06
	DecodeOprMUndefined,		//0x07
};

static uint8_t I0OprISize[0x10]=
{
	0x08, 0x40, 0x00, 0x20,
	0x08, 0x40, 0x00, 0x20,
	0x20, 0x40, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
};

static uint8_t I0OprMSize[0x08]=
{
	0x00, 0x08, 0x08, 0x0c,
	0x00, 0x00, 0x00, 0x00
};

static uint8_t I0OprDSize[0x10][0x08]=
{
	0x08, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00,
	0x40, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00,
	0x40, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00,
	0x40, 0x08, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static inline void DecodeOprI(char** codeptr,I0OPR* opr,uint32_t mattr)
{
	(*(decode_op_i_ptr[mattr]))(codeptr,opr);
}

static inline void DecodeOprM(char** codeptr,I0OPR* opr,uint32_t addrm)
{
	(*(decode_op_m_ptr[addrm]))(codeptr,opr);
}

static inline void DecodeOprD(char** codeptr,I0OPR* opr,uint32_t addrm,uint32_t mattr)
{
	if(addrm==ADDRM_IMMEDIATE)
	{
		(*(decode_op_i_ptr[mattr]))(codeptr,opr);
	}
	else
	{
		(*(decode_op_m_ptr[addrm]))(codeptr,opr);
	}
}

void PrintI0Opr(uint32_t mattr, I0OPR* opr);

void DecodeI0ToTemplate(char** spc, I0INSTR* instr)
{
    unsigned int op;
    instr->addr=(uint64_t)(*spc);
	LOAD_OP_WORD0(op,(*spc));
	GET_INST_FIELD_ZO(instr->opcode,op,BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
	switch(instr->opcode)
	{
	case OP_NOP:
		(*spc) += BYTE_OP_NOP;
		break;
	case OP_ADD:
	case OP_SUB:
	case OP_MUL:
	case OP_DIV:
		LOAD_OP_BYTE2(op,(*spc));
		GET_INST_FIELD_SH(instr->attr,op,BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
		GET_INST_FIELD_SH((instr->opr[0]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_SH((instr->opr[1]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_NW((instr->opr[2]).addrm,op,BIT_LEN_ADDRM);
		(*spc) += BYTE_OP_ALU;
		DecodeOprD(spc,&(instr->opr[0]),(instr->opr[0]).addrm,instr->attr);
		DecodeOprD(spc,&(instr->opr[1]),(instr->opr[1]).addrm,instr->attr);
		DecodeOprM(spc,&(instr->opr[2]),(instr->opr[2]).addrm);
		break;
	case OP_B:
		GET_INST_FIELD_ZO(instr->option,op,BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
		switch(instr->option)
		{
			case OPT_B_J:
				GET_INST_FIELD_NW(instr->ra,op,BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_RA);
				(*spc) += BYTE_OP_BJ;
				//(instr->opr[0]).addrm=ADDRM_IMMEDIATE;
				GET_INST_OPR_I64((instr->opr[0]),(*spc));
				break;
			case OPT_B_L:
			case OPT_B_LE:
			case OPT_B_E:
			case OPT_B_NE:
				LOAD_OP_DWORD_AND_SH(op,(*spc),BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
				GET_INST_FIELD_SH(instr->attr,op,BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr->opr[0]).addrm,op,BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr->opr[1]).addrm,op,BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr->ra),op,BIT_LEN_RA);
				(*spc) += BYTE_OP_BCMP;
				DecodeOprD(spc,&(instr->opr[0]),(instr->opr[0]).addrm,instr->attr);
				DecodeOprD(spc,&(instr->opr[1]),(instr->opr[1]).addrm,instr->attr);
				//(instr->opr[2]).addrm=ADDRM_IMMEDIATE;
				GET_INST_OPR_I64((instr->opr[2]),(*spc));
				break;
			case OPT_B_Z:
			case OPT_B_NZ:
				LOAD_OP_BYTE2(op,(*spc));
				GET_INST_FIELD_SH(instr->attr,op,BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr->opr[0]).addrm,op,BIT_LEN_ADDRM);
				GET_INST_FIELD_NW(instr->ra,op,BIT_LEN_RA);
				(*spc) += BYTE_OP_BZNZ;
				DecodeOprD(spc,&(instr->opr[0]),(instr->opr[0]).addrm,instr->attr);
				GET_INST_OPR_I64((instr->opr[1]),(*spc));
				break;
			case OPT_B_SL:
				LOAD_OP_DWORD_AND_SH(op,(*spc),BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
				GET_INST_FIELD_SH(instr->attr,op,BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr->opr[0]).addrm,op,BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr->opr[1]).addrm,op,BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr->ra),op,BIT_LEN_RA);
				(*spc) += BYTE_OP_BCMP;
				DecodeOprD(spc,&(instr->opr[0]),(instr->opr[0]).addrm,instr->attr);
				DecodeOprD(spc,&(instr->opr[1]),(instr->opr[1]).addrm,instr->attr);
				//(instr->opr[2]).addrm=ADDRM_IMMEDIATE;
				GET_INST_OPR_I64((instr->opr[2]),(*spc));
				break;
			case OPT_B_IJ:
				LOAD_OP_BYTE2(op,(*spc));
				GET_INST_FIELD_NW((instr->opr[0]).addrm,op,BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_ADDRM);
				(*spc) += BYTE_OP_BIJ;
				DecodeOprM(spc,&(instr->opr[0]),(instr->opr[0]).addrm);
				break;
			default:
				longjmp(decode_longjmp_buf,OPCODE_B_UNDEFINED);
		}
		break;
	case OP_AND:
	case OP_OR:
	case OP_XOR:
		LOAD_OP_BYTE2(op,(*spc));
		GET_INST_FIELD_SH(instr->attr,op,BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
		GET_INST_FIELD_SH((instr->opr[0]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_SH((instr->opr[1]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_NW((instr->opr[2]).addrm,op,BIT_LEN_ADDRM);
		(*spc) += BYTE_OP_ALU;
		DecodeOprD(spc,&(instr->opr[0]),(instr->opr[0]).addrm,instr->attr);
		DecodeOprD(spc,&(instr->opr[1]),(instr->opr[1]).addrm,instr->attr);
		DecodeOprM(spc,&(instr->opr[1]),(instr->opr[2]).addrm);
		break;
	case OP_CONV:
		LOAD_OP_DWORD_AND_SH(op,(*spc),BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
		GET_INST_FIELD_SH(instr->attr,op,BIT_LEN_MATTR);
		GET_INST_FIELD_SH(instr->attr2,op,BIT_LEN_MATTR);
		GET_INST_FIELD_SH((instr->opr[0]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_NW((instr->opr[1]).addrm,op,BIT_LEN_ADDRM);
		(*spc) += BYTE_OP_CONV;
		DecodeOprD(spc,&(instr->opr[0]),(instr->opr[0]).addrm,instr->attr);
		DecodeOprM(spc,&(instr->opr[1]),(instr->opr[1]).addrm);
		break;
	case OP_INT:
		(*spc) += BYTE_OP_INT;
		GET_INST_OPR_I8((instr->opr[0]),(*spc));
		break;
	case OP_SPAWN:
		LOAD_OP_BYTE2(op,(*spc));
		GET_INST_FIELD_SH((instr->opr[0]).addrm,op,BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_ADDRM);
		GET_INST_FIELD_SH((instr->opr[1]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_SH((instr->opr[2]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_NW((instr->opr[3]).addrm,op,BIT_LEN_ADDRM);
		(*spc) += BYTE_OP_SPAWN;
		DecodeOprM(spc,&(instr->opr[0]),(instr->opr[0]).addrm);
		DecodeOprM(spc,&(instr->opr[1]),(instr->opr[1]).addrm);
		DecodeOprM(spc,&(instr->opr[2]),(instr->opr[2]).addrm);
		DecodeOprM(spc,&(instr->opr[3]),(instr->opr[3]).addrm);
		break;
	case OP_SHIFT:
		LOAD_OP_DWORD_AND_SH(op,(*spc),BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
		GET_INST_FIELD_SH(instr->option,op,BIT_LEN_OPT_SHIFT);
		GET_INST_FIELD_SH(instr->attr,op,BIT_LEN_MATTR);
		GET_INST_FIELD_SH((instr->opr[0]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_SH((instr->opr[1]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_NW((instr->opr[2]).addrm,op,BIT_LEN_ADDRM);
		(*spc) += BYTE_OP_SHIFT;
		DecodeOprD(spc,&(instr->opr[0]),(instr->opr[0]).addrm,instr->attr);
		DecodeOprD(spc,&(instr->opr[1]),(instr->opr[1]).addrm,MATTR_UB);
		DecodeOprM(spc,&(instr->opr[2]),(instr->opr[2]).addrm);
		break;
	case OP_SCMP:
		LOAD_OP_DWORD_AND_SH(op,(*spc),BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
		GET_INST_FIELD_SH((instr->opr[0]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_SH((instr->opr[1]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_SH((instr->opr[2]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_SH((instr->opr[3]).addrm,op,BIT_LEN_ADDRM);
		GET_INST_FIELD_NW((instr->opr[4]).addrm,op,BIT_LEN_ADDRM);
		(*spc) += BYTE_OP_SCMP;
		instr->attr = MATTR_UE;
		DecodeOprM(spc,&(instr->opr[0]),(instr->opr[0]).addrm);
		DecodeOprD(spc,&(instr->opr[1]),(instr->opr[1]).addrm,instr->attr);
		DecodeOprM(spc,&(instr->opr[2]),(instr->opr[2]).addrm);
		DecodeOprD(spc,&(instr->opr[3]),(instr->opr[3]).addrm,instr->attr);
		DecodeOprM(spc,&(instr->opr[4]),(instr->opr[4]).addrm);
		break;
	case OP_EXIT:
		GET_INST_FIELD_NW(instr->option,op,BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_EXIT);
		(*spc) += BYTE_OP_EXIT;
		break;
	default:
		fprintf(stderr,"unexcepted op code %x\n",instr->opcode);
		longjmp(decode_longjmp_buf,OPCODE_UNDEFINED);
	}
}

int main(int argc, char** argv)
{
	int i0bin = open(argv[1],O_RDONLY);
	struct stat filestat;
	char* loadaddr;
	char* codeptr;
	I0INSTR I0Template;
	int returnvalue;
	if(i0bin!=-1)
	{
		if(fstat(i0bin,&filestat)!=-1)
		{
			loadaddr = (char*)mmap(NULL,filestat.st_size,PROT_READ,MAP_PRIVATE,i0bin,0);
			if(loadaddr!=MAP_FAILED)
			{
				if((returnvalue=setjmp(decode_longjmp_buf)))
				{
					fprintf(stderr,"#UD return value == %x\naddr== %lx\n",returnvalue,(uint64_t)(codeptr-loadaddr));
					return 4;
				}
				else
				{
					printf("now decoding\n");
					codeptr = loadaddr;
					while(((int64_t)(codeptr-loadaddr))<filestat.st_size)
					{
						DecodeI0ToTemplate(&codeptr,&I0Template);
						PrintI0Template(&I0Template);
					}
				}
			}
			else
			{
				fprintf(stderr,"mmap failed\n");
				return 3;
			}
		}
		else
		{
			fprintf(stderr,"file status unexcepted\n");
			return 2;
		}
	}
	else
	{
		fprintf(stderr,"failed to open file\n");
		return 1;
	}
	return 0;
}

typedef DECODE_STATUS (*TranslateNative)(I0INSTR*,void**,void*,unsigned int);

static inline DECODE_STATUS TranslateNOP_NW(I0INSTR* instr, void** tpc, void* nativelimit) {
	(void) instr;
	unsigned long nativelen = (((unsigned long) nativelimit) - ((unsigned long) (*tpc)));
	if (nativelen > 0) {
		(*tpc)++;
		RETURN_DECODE_STATUS(I0_DECODE_SUCCESSFUL, 0);
	} else {
		RETURN_DECODE_STATUS(NATIVE_CODE_SEGMENT_LIMIT, 0);
	}
}

static inline DECODE_STATUS TranslateNOP_WR(I0INSTR* instr, void** tpc, void* nativelimit) {
	(void) instr;
	unsigned long nativelen = (((unsigned long) nativelimit) - ((unsigned long) (*tpc)));
	if (nativelen > 0) {
		(*tpc)++;
		(*((unsigned char*) (*tpc))) = 0x90;
		RETURN_DECODE_STATUS(I0_DECODE_SUCCESSFUL, 0);
	} else {
		RETURN_DECODE_STATUS(NATIVE_CODE_SEGMENT_LIMIT, 0);
	}
}

inline DECODE_STATUS TranslateEXIT_WR(I0INSTR* instr, uint8_t** tpc, uint8_t* nativelimit)
{
	//movl $option, %edi
	//movl $back_runner_wrapper, %eax
	//jmp *%rax
	unsigned long nativelen = (((unsigned long)nativelimit)-((unsigned long)(*tpc)));
	if(nativelen<0x0c)
	{
		RETURN_DECODE_STATUS(NATIVE_CODE_SEGMENT_LIMIT, 0);
	}
	else
	{
		(*((*tpc)++)) = 0xbf;
		(*((uint32_t)(*tpc))) = instr->option;
		(*tpc) += 4;
		(*((*tpc)++)) = 0xb8;
		(*((uint32_t)(*tpc))) = ((uint32_t)sys_back_runner_wrapper);
		(*tpc) += 4;
		(*((uint16_t)(*tpc))) = 0xe0ff;
		(*tpc) += 2;
		RETURN_DECODE_STATUS(I0_DECODE_SUCCESSFUL, 0);
	}
}

inline DECODE_STATUS TranslateEXIT_NW(I0INSTR* instr, uint8_t** tpc, uint8_t* nativelimit)
{
	unsigned long nativelen = (((unsigned long)nativelimit)-((unsigned long)(*tpc)));
	if(nativelen<0x0c)
	{
		RETURN_DECODE_STATUS(NATIVE_CODE_SEGMENT_LIMIT, 0);
	}
	else
	{
		(*tpc) += 0x0c;
		RETURN_DECODE_STATUS(I0_DECODE_SUCCESSFUL, 0);
	}

}

DECODE_STATUS TranslateNOP_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateADD_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateSUB_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateMUL_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateDIV_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateBJ_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateBL_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateBLE_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateBE_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateBNE_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateBZ_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateBNZ_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateBSL_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateBIJ_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateAND_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateOR_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateXOR_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateCONV_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateINT_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateSPAWN_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateSHIFT_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateSCMP_NW(I0INSTR*, void**, void*);
DECODE_STATUS TranslateEXIT_NW(I0INSTR*, void**, void*);

DECODE_STATUS TranslateI0ToNative(void** spc, void** tpc, void* i0limit, void* nativelimit, unsigned int is_write) {
	I0INSTR instr;
	unsigned int op;
	unsigned long i0instrlen = 0;
	unsigned long i0len = (((unsigned long) i0limit) - ((unsigned long) (*spc)));
	if (i0len > 1) {
		LOAD_OP_WORD0(op, (*spc));
	} else {
		RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
	}
	instr.addr = (uint64_t) (*spc);
	GET_INST_FIELD_ZO(instr.opcode, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
	if (is_write) {
		switch (instr.opcode) {
		case OP_NOP:
			(*spc) += BYTE_OP_NOP;
			return TranslateNOP_WR(&instr, tpc, nativelimit);
			break;
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
			LOAD_OP_BYTE2(op, (*spc));
			GET_INST_FIELD_SH(instr->attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr->opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr->opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr->opr[2]).addrm, op, BIT_LEN_ADDRM);
			(*spc) += BYTE_OP_ALU;
			DecodeOprD(spc, &(instr->opr[0]), (instr->opr[0]).addrm, instr->attr);
			DecodeOprD(spc, &(instr->opr[1]), (instr->opr[1]).addrm, instr->attr);
			DecodeOprM(spc, &(instr->opr[2]), (instr->opr[2]).addrm);
			break;
		case OP_B:
			GET_INST_FIELD_ZO(instr->option, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
			switch (instr->option) {
			case OPT_B_J:
				GET_INST_FIELD_NW(instr->ra, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_RA);
				(*spc) += BYTE_OP_BJ;
				//(instr->opr[0]).addrm=ADDRM_IMMEDIATE;
				GET_INST_OPR_I64((instr->opr[0]), (*spc));
				break;
			case OPT_B_L:
			case OPT_B_LE:
			case OPT_B_E:
			case OPT_B_NE:
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
				GET_INST_FIELD_SH(instr->attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr->opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr->opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr->ra), op, BIT_LEN_RA);
				(*spc) += BYTE_OP_BCMP;
				DecodeOprD(spc, &(instr->opr[0]), (instr->opr[0]).addrm, instr->attr);
				DecodeOprD(spc, &(instr->opr[1]), (instr->opr[1]).addrm, instr->attr);
				//(instr->opr[2]).addrm=ADDRM_IMMEDIATE;
				GET_INST_OPR_I64((instr->opr[2]), (*spc));
				break;
			case OPT_B_Z:
			case OPT_B_NZ:
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr->attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr->opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW(instr->ra, op, BIT_LEN_RA);
				(*spc) += BYTE_OP_BZNZ;
				DecodeOprD(spc, &(instr->opr[0]), (instr->opr[0]).addrm, instr->attr);
				GET_INST_OPR_I64((instr->opr[1]), (*spc));
				break;
			case OPT_B_SL:
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
				GET_INST_FIELD_SH(instr->attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr->opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr->opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr->ra), op, BIT_LEN_RA);
				(*spc) += BYTE_OP_BCMP;
				DecodeOprD(spc, &(instr->opr[0]), (instr->opr[0]).addrm, instr->attr);
				DecodeOprD(spc, &(instr->opr[1]), (instr->opr[1]).addrm, instr->attr);
				//(instr->opr[2]).addrm=ADDRM_IMMEDIATE;
				GET_INST_OPR_I64((instr->opr[2]), (*spc));
				break;
			case OPT_B_IJ:
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_NW((instr->opr[0]).addrm, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_ADDRM);
				(*spc) += BYTE_OP_BIJ;
				DecodeOprM(spc, &(instr->opr[0]), (instr->opr[0]).addrm);
				break;
			default:
				longjmp(decode_longjmp_buf, OPCODE_B_UNDEFINED);
			}
			break;
		case OP_AND:
		case OP_OR:
		case OP_XOR:
			LOAD_OP_BYTE2(op, (*spc));
			GET_INST_FIELD_SH(instr->attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr->opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr->opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr->opr[2]).addrm, op, BIT_LEN_ADDRM);
			(*spc) += BYTE_OP_ALU;
			DecodeOprD(spc, &(instr->opr[0]), (instr->opr[0]).addrm, instr->attr);
			DecodeOprD(spc, &(instr->opr[1]), (instr->opr[1]).addrm, instr->attr);
			DecodeOprM(spc, &(instr->opr[1]), (instr->opr[2]).addrm);
			break;
		case OP_CONV:
			LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
			GET_INST_FIELD_SH(instr->attr, op, BIT_LEN_MATTR);
			GET_INST_FIELD_SH(instr->attr2, op, BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr->opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr->opr[1]).addrm, op, BIT_LEN_ADDRM);
			(*spc) += BYTE_OP_CONV;
			DecodeOprD(spc, &(instr->opr[0]), (instr->opr[0]).addrm, instr->attr);
			DecodeOprM(spc, &(instr->opr[1]), (instr->opr[1]).addrm);
			break;
		case OP_INT:
			(*spc) += BYTE_OP_INT;
			GET_INST_OPR_I8((instr->opr[0]), (*spc));
			break;
		case OP_SPAWN:
			LOAD_OP_BYTE2(op, (*spc));
			GET_INST_FIELD_SH((instr->opr[0]).addrm, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr->opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr->opr[2]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr->opr[3]).addrm, op, BIT_LEN_ADDRM);
			(*spc) += BYTE_OP_SPAWN;
			DecodeOprM(spc, &(instr->opr[0]), (instr->opr[0]).addrm);
			DecodeOprM(spc, &(instr->opr[1]), (instr->opr[1]).addrm);
			DecodeOprM(spc, &(instr->opr[2]), (instr->opr[2]).addrm);
			DecodeOprM(spc, &(instr->opr[3]), (instr->opr[3]).addrm);
			break;
		case OP_SHIFT:
			LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
			GET_INST_FIELD_SH(instr->option, op, BIT_LEN_OPT_SHIFT);
			GET_INST_FIELD_SH(instr->attr, op, BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr->opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr->opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr->opr[2]).addrm, op, BIT_LEN_ADDRM);
			(*spc) += BYTE_OP_SHIFT;
			DecodeOprD(spc, &(instr->opr[0]), (instr->opr[0]).addrm, instr->attr);
			DecodeOprD(spc, &(instr->opr[1]), (instr->opr[1]).addrm, MATTR_UB);
			DecodeOprM(spc, &(instr->opr[2]), (instr->opr[2]).addrm);
			break;
		case OP_SCMP:
			LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
			GET_INST_FIELD_SH((instr->opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr->opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr->opr[2]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr->opr[3]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr->opr[4]).addrm, op, BIT_LEN_ADDRM);
			(*spc) += BYTE_OP_SCMP;
			instr->attr = MATTR_UE;
			DecodeOprM(spc, &(instr->opr[0]), (instr->opr[0]).addrm);
			DecodeOprD(spc, &(instr->opr[1]), (instr->opr[1]).addrm, instr->attr);
			DecodeOprM(spc, &(instr->opr[2]), (instr->opr[2]).addrm);
			DecodeOprD(spc, &(instr->opr[3]), (instr->opr[3]).addrm, instr->attr);
			DecodeOprM(spc, &(instr->opr[4]), (instr->opr[4]).addrm);
			break;
		case OP_EXIT:
			GET_INST_FIELD_NW(instr->option, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_EXIT);
			(*spc) += BYTE_OP_EXIT;
			break;
		default:
			fprintf(stderr, "unexcepted op code %x\n", instr->opcode);
			longjmp(decode_longjmp_buf, OPCODE_UNDEFINED);
		}
	} else {
		switch(instr.opcode)
		{
		case OP_NOP:
			i0instrlen = BYTE_OP_NOP;
			(*spc) += i0instrlen;
			return TranslateNOP_NW(&instr, tpc, nativelimit);
			break;
		case OP_ADD:
			i0instrlen = BYTE_OP_ALU;
			if(i0len>=i0instrlen)
			{
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			}
			else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if(!I0OprMSize[instr.opr[2].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			(*spc) += i0instrlen;
			return TranslateADD_NW(&instr, tpc, nativelimit);
			break;
		case OP_SUB:
			i0instrlen = BYTE_OP_ALU;
			if(i0len>=i0instrlen)
			{
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			}
			else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if(!I0OprMSize[instr.opr[2].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			(*spc) += i0instrlen;
			return TranslateSUB_NW(&instr, tpc, nativelimit);
			break;
		case OP_MUL:
			i0instrlen = BYTE_OP_ALU;
			if(i0len>=i0instrlen)
			{
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			}
			else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if(!I0OprMSize[instr.opr[2].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			(*spc) += i0instrlen;
			return TranslateMUL_NW(&instr, tpc, nativelimit);
			break;
		case OP_DIV:
			i0instrlen = BYTE_OP_ALU;
			if(i0len>=i0instrlen)
			{
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			}
			else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if(!I0OprMSize[instr.opr[2].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			(*spc) += i0instrlen;
			return TranslateDIV_NW(&instr, tpc, nativelimit);
			break;
		case OP_B:
			GET_INST_FIELD_ZO(instr.option, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
			switch (instr.option) {
			case OPT_B_J:
				GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_RA);
				i0instrlen = BYTE_OP_BJ;
				i0instrlen += 8;
				if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				(*spc) += i0instrlen;
				return TranslateBJ_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_L:
				i0instrlen = BYTE_OP_BCMP;
				if(i0len>=i0instrlen)
				{
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				}
				else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				(*spc) += i0instrlen;
				GET_INST_OPR_I64((instr->opr[2]), (*spc));
				return TranslateBL_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_LE:
				i0instrlen = BYTE_OP_BCMP;
				if(i0len>=i0instrlen)
				{
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				}
				else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				(*spc) += i0instrlen;
				GET_INST_OPR_I64((instr->opr[2]), (*spc));
				return TranslateBLE_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_E:
				i0instrlen = BYTE_OP_BCMP;
				if(i0len>=i0instrlen)
				{
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				}
				else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				(*spc) += i0instrlen;
				GET_INST_OPR_I64((instr->opr[2]), (*spc));
				return TranslateBE_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_NE:
				i0instrlen = BYTE_OP_BCMP;
				if(i0len>=i0instrlen)
				{
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				}
				else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				(*spc) += i0instrlen;
				GET_INST_OPR_I64((instr->opr[2]), (*spc));
				return TranslateBNE_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_Z:
				i0instrlen = BYTE_OP_BZNZ;
				if(i0len>=i0instrlen)
				{
					LOAD_OP_BYTE2(op, (*spc));
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_RA);
				}
				else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += (I0OprDSize[instr.attr][instr.opr[0].addrm] + 8);
				if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				(*spc) += i0instrlen;
				GET_INST_OPR_I64((instr->opr[2]), (*spc));
				return TranslateBZ_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_NZ:
				i0instrlen = BYTE_OP_BZNZ;
				if(i0len>=i0instrlen)
				{
					LOAD_OP_BYTE2(op, (*spc));
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_RA);
				}
				else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += (I0OprDSize[instr.attr][instr.opr[0].addrm] + 8);
				if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				(*spc) += i0instrlen;
				GET_INST_OPR_I64((instr->opr[2]), (*spc));
				return TranslateBNZ_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_SL:
				i0instrlen = BYTE_OP_BCMP;
				if(i0len>=i0instrlen)
				{
					LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
					GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
					GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
					GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
				}
				else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
				if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
				i0instrlen += 8;
				if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				(*spc) += i0instrlen;
				GET_INST_OPR_I64((instr->opr[2]), (*spc));
				return TranslateBSL_NW(&instr, tpc, nativelimit);
				break;
			case OPT_B_IJ:
				i0instrlen = BYTE_OP_BIJ;
				if(i0len>=i0instrlen)
				{
					LOAD_OP_BYTE2(op, (*spc));
					GET_INST_FIELD_NW((instr.opr[0]).addrm, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_ADDRM);
				}
				else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				if(!I0OprMSize[instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
				i0instrlen += I0OprMSize[instr.opr[0].addrm];
				if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
				(*spc) += i0instrlen;
				return TranslateBIJ_NW(&instr, tpc, nativelimit);
				break;
			default:
				RETURN_DECODE_STATUS(OPCODE_B_UNDEFINED, 0);
				break;
			}
			break;
		case OP_AND:
			i0instrlen = BYTE_OP_ALU;
			if(i0len>=i0instrlen)
			{
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			}
			else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if(!I0OprMSize[instr.opr[2].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			(*spc) += i0instrlen;
			return TranslateAND_NW(&instr, tpc, nativelimit);
			break;
		case OP_OR:
			i0instrlen = BYTE_OP_ALU;
			if(i0len>=i0instrlen)
			{
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			}
			else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if(!I0OprMSize[instr.opr[2].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			(*spc) += i0instrlen;
			return TranslateOR_NW(&instr, tpc, nativelimit);
			break;
		case OP_XOR:
			i0instrlen = BYTE_OP_ALU;
			if(i0len>=i0instrlen)
			{
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			}
			else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if(!I0OprMSize[instr.opr[2].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			(*spc) += i0instrlen;
			return TranslateXOR_NW(&instr, tpc, nativelimit);
			break;
		case OP_CONV:
			i0instrlen = BYTE_OP_CONV;
			if(i0len>=i0instrlen)
			{
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH(instr.attr2, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			}
			else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if(!I0OprMSize[instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[1].addrm];
			if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			(*spc) += i0instrlen;
			return TranslateCONV_NW(&instr, tpc, nativelimit);
			break;
		case OP_INT:
			i0instrlen = BYTE_OP_INT;
			i0instrlen += 8;
			if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			(*spc) += i0instrlen;
			return TranslateINT_NW(&instr, tpc, nativelimit);
			break;
		case OP_SPAWN:
			i0instrlen = BYTE_OP_SPAWN;
			if(i0len>=i0instrlen)
			{
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[3]).addrm, op, BIT_LEN_ADDRM);
			}
			else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			if(!I0OprMSize[instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[0].addrm];
			if(!I0OprMSize[instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[1].addrm];
			if(!I0OprMSize[instr.opr[2].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if(!I0OprMSize[instr.opr[3].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[3].addrm];
			if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			(*spc) += i0instrlen;
			return TranslateSPAWN_NW(&instr, tpc, nativelimit);
			break;
		case OP_SHIFT:
			i0instrlen = BYTE_OP_SHIFT;
			if(i0len>=i0instrlen)
			{
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
				GET_INST_FIELD_SH(instr.option, op, BIT_LEN_OPT_SHIFT);
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			}
			else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			if(!I0OprDSize[instr.attr][instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if(!I0OprDSize[MATTR_UB][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[MATTR_UB][instr.opr[1].addrm];
			if(!I0OprMSize[instr.opr[2].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			(*spc) += i0instrlen;
			return TranslateSHIFT_NW(&instr, tpc, nativelimit);
			break;
		case OP_SCMP:
			i0instrlen = BYTE_OP_SCMP;
			if(i0len>=i0instrlen)
			{
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[3]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.opr[4]).addrm, op, BIT_LEN_ADDRM);
				instr.attr = MATTR_UE;
			}
			else{RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			if(!I0OprMSize[instr.opr[0].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[0].addrm];
			if(!I0OprDSize[instr.attr][instr.opr[1].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			if(!I0OprMSize[instr.opr[2].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[2].addrm];
			if(!I0OprDSize[instr.attr][instr.opr[3].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[3].addrm];
			if(!I0OprMSize[instr.opr[4].addrm]){RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);}
			i0instrlen += I0OprMSize[instr.opr[4].addrm];
			if(i0instrlen>i0len){RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);}
			(*spc) += i0instrlen;
			return TranslateSCMP_NW(&instr, tpc, nativelimit);
			break;
		case OP_EXIT:
			i0instrlen = BYTE_OP_EXIT;
			GET_INST_FIELD_NW(instr.option, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_EXIT);
			(*spc) += i0instrlen;
			return TranslateEXIT_NW(&instr, tpc, nativelimit);
			break;
		default:
			RETURN_DECODE_STATUS(OPCODE_UNDEFINED, 0);
			break;
		}
	}
}

DECODE_STATUS TranslateI0ToNative(uint8_t** spc, uint8_t** tpc, uint8_t* nativelimit,uint8_t* i0limit, unsigned int is_write) {
	I0INSTR instr;
	unsigned int op;
	unsigned long i0instrlen = 0;
	unsigned long i0len = (((unsigned long) i0limit) - ((unsigned long) (*spc)));
	if (i0len >= ((BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE)/8)) {
		LOAD_OP_WORD0(op, (*spc));
	} else {
		RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
	}
	instr.addr = (uint64_t) (*spc);
	GET_INST_FIELD_ZO(instr.opcode, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);

	switch (instr.opcode) {
	case OP_NOP:
		i0instrlen = BYTE_OP_NOP;
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateNOP_NW(&instr, tpc, nativelimit);
		break;
	case OP_ADD:
		i0instrlen = BYTE_OP_ALU;
		if (i0len >= i0instrlen) {
			LOAD_OP_BYTE2(op, (*spc));
			GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
		} else {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
		if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateADD_NW(&instr, tpc, nativelimit);
		break;
	case OP_SUB:
		i0instrlen = BYTE_OP_ALU;
		if (i0len >= i0instrlen) {
			LOAD_OP_BYTE2(op, (*spc));
			GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
		} else {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
		if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateSUB_NW(&instr, tpc, nativelimit);
		break;
	case OP_MUL:
		i0instrlen = BYTE_OP_ALU;
		if (i0len >= i0instrlen) {
			LOAD_OP_BYTE2(op, (*spc));
			GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
		} else {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
		if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateMUL_NW(&instr, tpc, nativelimit);
		break;
	case OP_DIV:
		i0instrlen = BYTE_OP_ALU;
		if (i0len >= i0instrlen) {
			LOAD_OP_BYTE2(op, (*spc));
			GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
		} else {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
		if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateDIV_NW(&instr, tpc, nativelimit);
		break;
	case OP_B:
		GET_INST_FIELD_ZO(instr.option, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
		switch (instr.option) {
		case OPT_B_J:
			GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_RA);
			i0instrlen = BYTE_OP_BJ;
			i0instrlen += 8;
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += (i0instrlen-8);
			GET_INST_OPR_I64((instr.opr[0]), (*spc));
			return TranslateBJ_NW(&instr, tpc, nativelimit);
			break;
		case OPT_B_L:
			i0instrlen = BYTE_OP_BCMP;
			if (i0len >= i0instrlen) {
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			i0instrlen += 8;
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += (i0instrlen-8);
			GET_INST_OPR_I64((instr.opr[2]), (*spc));
			return TranslateBL_NW(&instr, tpc, nativelimit);
			break;
		case OPT_B_LE:
			i0instrlen = BYTE_OP_BCMP;
			if (i0len >= i0instrlen) {
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			i0instrlen += 8;
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += (i0instrlen-8);
			GET_INST_OPR_I64((instr.opr[2]), (*spc));
			return TranslateBLE_NW(&instr, tpc, nativelimit);
			break;
		case OPT_B_E:
			i0instrlen = BYTE_OP_BCMP;
			if (i0len >= i0instrlen) {
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			i0instrlen += 8;
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += (i0instrlen-8);
			GET_INST_OPR_I64((instr.opr[2]), (*spc));
			return TranslateBE_NW(&instr, tpc, nativelimit);
			break;
		case OPT_B_NE:
			i0instrlen = BYTE_OP_BCMP;
			if (i0len >= i0instrlen) {
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			i0instrlen += 8;
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += (i0instrlen-8);
			GET_INST_OPR_I64((instr.opr[2]), (*spc));
			return TranslateBNE_NW(&instr, tpc, nativelimit);
			break;
		case OPT_B_Z:
			i0instrlen = BYTE_OP_BZNZ;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_RA);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += (I0OprDSize[instr.attr][instr.opr[0].addrm] + 8U);
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += (i0instrlen-8);
			GET_INST_OPR_I64((instr.opr[2]), (*spc));
			return TranslateBZ_NW(&instr, tpc, nativelimit);
			break;
		case OPT_B_NZ:
			i0instrlen = BYTE_OP_BZNZ;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW(instr.ra, op, BIT_LEN_RA);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += (I0OprDSize[instr.attr][instr.opr[0].addrm] + 8U);
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += (i0instrlen-8);
			GET_INST_OPR_I64((instr.opr[2]), (*spc));
			return TranslateBNZ_NW(&instr, tpc, nativelimit);
			break;
		case OPT_B_SL:
			i0instrlen = BYTE_OP_BCMP;
			if (i0len >= i0instrlen) {
				LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B);
				GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
				GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
				GET_INST_FIELD_NW((instr.ra), op, BIT_LEN_RA);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
			if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
			i0instrlen += 8;
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += (i0instrlen-8);
			GET_INST_OPR_I64((instr.opr[2]), (*spc));
			return TranslateBSL_NW(&instr, tpc, nativelimit);
			break;
		case OPT_B_IJ:
			i0instrlen = BYTE_OP_BIJ;
			if (i0len >= i0instrlen) {
				LOAD_OP_BYTE2(op, (*spc));
				GET_INST_FIELD_NW((instr.opr[0]).addrm, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_B+BIT_LEN_ADDRM);
			} else {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			if (!I0OprMSize[instr.opr[0].addrm]) {
				RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
			}
			i0instrlen += I0OprMSize[instr.opr[0].addrm];
			if (i0instrlen > i0len) {
				RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
			}
			(*spc) += i0instrlen;
			return TranslateBIJ_NW(&instr, tpc, nativelimit);
			break;
		default:
			RETURN_DECODE_STATUS(OPCODE_B_UNDEFINED, 0)
			;
			break;
		}
		break;
	case OP_AND:
		i0instrlen = BYTE_OP_ALU;
		if (i0len >= i0instrlen) {
			LOAD_OP_BYTE2(op, (*spc));
			GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
		} else {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
		if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateAND_NW(&instr, tpc, nativelimit);
		break;
	case OP_OR:
		i0instrlen = BYTE_OP_ALU;
		if (i0len >= i0instrlen) {
			LOAD_OP_BYTE2(op, (*spc));
			GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
		} else {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
		if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateOR_NW(&instr, tpc, nativelimit);
		break;
	case OP_XOR:
		i0instrlen = BYTE_OP_ALU;
		if (i0len >= i0instrlen) {
			LOAD_OP_BYTE2(op, (*spc));
			GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
		} else {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
		if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateXOR_NW(&instr, tpc, nativelimit);
		break;
	case OP_CONV:
		i0instrlen = BYTE_OP_CONV;
		if (i0len >= i0instrlen) {
			LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
			GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
			GET_INST_FIELD_SH(instr.attr2, op, BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
		} else {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
		if (!I0OprMSize[instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[1].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateCONV_NW(&instr, tpc, nativelimit);
		break;
	case OP_INT:
		i0instrlen = BYTE_OP_INT;
		i0instrlen += 8;
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateINT_NW(&instr, tpc, nativelimit);
		break;
	case OP_SPAWN:
		i0instrlen = BYTE_OP_SPAWN;
		if (i0len >= i0instrlen) {
			LOAD_OP_BYTE2(op, (*spc));
			GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr.opr[3]).addrm, op, BIT_LEN_ADDRM);
		} else {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		if (!I0OprMSize[instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[0].addrm];
		if (!I0OprMSize[instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (!I0OprMSize[instr.opr[3].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[3].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateSPAWN_NW(&instr, tpc, nativelimit);
		break;
	case OP_SHIFT:
		i0instrlen = BYTE_OP_SHIFT;
		if (i0len >= i0instrlen) {
			LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
			GET_INST_FIELD_SH(instr.option, op, BIT_LEN_OPT_SHIFT);
			GET_INST_FIELD_SH(instr.attr, op, BIT_LEN_MATTR);
			GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
		} else {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		if (!I0OprDSize[instr.attr][instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[0].addrm];
		if (!I0OprDSize[MATTR_UB][instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[MATTR_UB][instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateSHIFT_NW(&instr, tpc, nativelimit);
		break;
	case OP_SCMP:
		i0instrlen = BYTE_OP_SCMP;
		if (i0len >= i0instrlen) {
			LOAD_OP_DWORD_AND_SH(op, (*spc), BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE);
			GET_INST_FIELD_SH((instr.opr[0]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[1]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[2]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_SH((instr.opr[3]).addrm, op, BIT_LEN_ADDRM);
			GET_INST_FIELD_NW((instr.opr[4]).addrm, op, BIT_LEN_ADDRM);
			instr.attr = MATTR_UE;
		} else {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		if (!I0OprMSize[instr.opr[0].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[0].addrm];
		if (!I0OprDSize[instr.attr][instr.opr[1].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[1].addrm];
		if (!I0OprMSize[instr.opr[2].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[2].addrm];
		if (!I0OprDSize[instr.attr][instr.opr[3].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprDSize[instr.attr][instr.opr[3].addrm];
		if (!I0OprMSize[instr.opr[4].addrm]) {
			RETURN_DECODE_STATUS(I0_INSTR_OPERAND_UNDEFINED, 0);
		}
		i0instrlen += I0OprMSize[instr.opr[4].addrm];
		if (i0instrlen > i0len) {
			RETURN_DECODE_STATUS(I0_CODE_SEGMENT_LIMIT, 0);
		}
		(*spc) += i0instrlen;
		return TranslateSCMP_NW(&instr, tpc, nativelimit);
		break;
	case OP_EXIT:
		i0instrlen = BYTE_OP_EXIT;
		GET_INST_FIELD_NW(instr.option, op, BIT_LEN_ADDR_SIZE_MODE+BIT_LEN_OPCODE+BIT_LEN_OPT_EXIT);
		(*spc) += i0instrlen;
		return TranslateEXIT_NW(&instr, tpc, nativelimit);
		break;
	default:
		RETURN_DECODE_STATUS(OPCODE_UNDEFINED, 0)
		;
		break;
	}
}



