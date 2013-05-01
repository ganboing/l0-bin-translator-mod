#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bin_translator.h"
#include "I0Symbol.h"
#include "ASM_MACROS.h"

const char* const string_ra[2]=
{
	":absolute ",
	":relative "
};

const char* const string_shift[4]=
{
	"shl ",
	"shr ",
	"sh? ",
	"sh? "
};

const char* const string_b_types[0x0d]=
{
	"bj",
	"",
	"bl",
	"ble",
	"be",
	"bne",
	"bz",
	"bnz",
	"bsl",
	"",
	"",
	"",
	"bij "
};

const char* const string_attrs[0x0a] =
{
	":sb",
	":se",
	":ss",
	":sf",
	":ub",
	":ue",
	":us",
	":uf",
	":fs",
	":fd"
};

const char* const string_aluops[10] =
{
	"add",
	"sub",
	"mul",
	"div",
	"",
	"",
	"and",
	"or",
	"xor"
};


static void PrintHexInt8(uint32_t x)
{
	printf("0x");
	x<<=24;
	char c[3];
	memset(c,0,3);
	int i=0;
	for(i=0;i<2;i++)
	{
		uint32_t t;
		SHLD_SHL(t,x,4);
		if(t<0x0a)
		{
			c[i]=(char)(t+0x30);
		}
		else
		{
			c[i]=(char)(t+0x37);
		}
	}
	printf("%s",c);
}

static void PrintHexInt32(uint32_t x)
{
	printf("0x");
	char c[9];
	memset(c,0,9);
	int i=0;
	for(i=0;i<8;i++)
	{
		uint32_t t;
		SHLD_SHL(t,x,4);
		if(t<0x0a)
		{
			c[i]=(char)(t+0x30);
		}
		else
		{
			c[i]=(char)(t+0x37);
		}
	}
	printf("%s",c);
}

static void PrintHexInt64(uint64_t x)
{
	printf("0x");
	char c[17];
	memset(c,0,17);
	int i=0;
	for(i=0;i<16;i++)
	{
		uint64_t t;
		SHLD_SHL(t,x,4);
		if(t<0x0a)
		{
			c[i]=(char)(t+0x30);
		}
		else
		{
			c[i]=(char)(t+0x37);
		}
	}
	printf("%s",c);
}

static void PrintI0Opr(uint32_t mattr,const I0OPR* opr)
{
	switch(opr->addrm)
	{
	case ADDRM_IMMEDIATE:
		switch(mattr)
		{
		case MATTR_SB:
		case MATTR_UB:
			putchar('$');
			PrintHexInt8(opr->val);
			break;
		case MATTR_SF:
		case MATTR_UF:
		case MATTR_FS:
			putchar('$');
			PrintHexInt32(opr->val);
			break;
		case MATTR_SE:
		case MATTR_UE:
		case MATTR_FD:
			putchar('$');
			PrintHexInt64(opr->val);
			break;
		default:
			putchar('?');
			return;
		}
		break;
	case ADDRM_ABSOLUTE:
		putchar('(');
		PrintHexInt64(opr->val);
		putchar(')');
		break;
	case ADDRM_INDIRECT:
		printf("((");
		PrintHexInt64(opr->val);
		printf("))");
		break;
	case ADDRM_DISPLACEMENT:
		putchar('(');
		PrintHexInt32(opr->disp32);
		putchar('(');
		PrintHexInt64(opr->val);
		printf("))");
		break;
	default:
		putchar('?');
		return;
	}
	if(mattr<0x0a)
	{
		printf("%s",string_attrs[mattr]);
	}
}

void PrintI0Template(const I0INSTR* instr)
{
	switch(instr->opcode)
	{
	case OP_NOP:
		printf("nop\n");
		break;
	case OP_ADD:
	case OP_SUB:
	case OP_MUL:
	case OP_DIV:
		printf("%s ",string_aluops[(instr->opcode)/3-1]);
		PrintI0Opr(instr->attr,&(instr->opr[0]));
		printf(", ");
		PrintI0Opr(instr->attr,&(instr->opr[1]));
		printf(", ");
		PrintI0Opr(instr->attr,&(instr->opr[2]));
		putchar('\n');
		break;
	case OP_B:
		switch(instr->option)
		{
			case OPT_B_J:
				printf("%s%s",string_b_types[instr->option],string_ra[instr->ra]);
				PrintHexInt64((instr->opr[0]).val);
				putchar('\n');
				break;
			case OPT_B_L:
			case OPT_B_LE:
			case OPT_B_E:
			case OPT_B_NE:
				printf("%s%s",string_b_types[instr->option],string_ra[instr->ra]);
				PrintI0Opr(instr->attr,&(instr->opr[0]));
				printf(", ");
				PrintI0Opr(instr->attr,&(instr->opr[1]));
				printf(", ");
				PrintHexInt64((instr->opr[2]).val);
				putchar('\n');
				break;
			case OPT_B_Z:
			case OPT_B_NZ:
				printf("%s%s",string_b_types[instr->option],string_ra[instr->ra]);
				PrintI0Opr(instr->attr,&(instr->opr[0]));
				printf(", ");
				PrintHexInt64((instr->opr[1]).val);
				putchar('\n');
				break;
			case OPT_B_SL:
				printf("%s%s",string_b_types[instr->option],string_ra[instr->ra]);
				PrintI0Opr(instr->attr,&(instr->opr[0]));
				printf(", ");
				PrintI0Opr(instr->attr,&(instr->opr[1]));
				printf(", ");
				PrintHexInt64((instr->opr[2]).val);
				putchar('\n');
				break;
			case OPT_B_IJ:
				printf("%s ",string_b_types[instr->option]);
				PrintI0Opr(MATTR_UE,&(instr->opr[0]));
				putchar('\n');
				break;
			default:
				printf("b:?\n");
				break;
		}
		break;
	case OP_AND:
	case OP_OR:
	case OP_XOR:
		printf("%s ",string_aluops[(instr->opcode)/3-1]);
		PrintI0Opr(instr->attr,&(instr->opr[0]));
		printf(", ");
		PrintI0Opr(instr->attr,&(instr->opr[1]));
		printf(", ");
		PrintI0Opr(instr->attr,&(instr->opr[2]));
		putchar('\n');
		break;
	case OP_CONV:
		printf("conv ");
		PrintI0Opr(instr->attr,&(instr->opr[0]));
		printf(", ");
		PrintI0Opr(instr->attr2,&(instr->opr[1]));
		putchar('\n');
		break;
	case OP_INT:
		printf("int ");
		PrintHexInt8((instr->opr[0]).val);
		putchar('\n');
		break;
	case OP_SPAWN:
		printf("spwawn ");
		PrintI0Opr(MATTR_UE,&(instr->opr[0]));
		printf(", ");
		PrintI0Opr(MATTR_UE,&(instr->opr[1]));
		printf(", ");
		PrintI0Opr(MATTR_UE,&(instr->opr[2]));
		printf(", ");
		PrintI0Opr(MATTR_UE,&(instr->opr[3]));
		putchar('\n');
		break;
	case OP_SHIFT:
		printf("%s",string_shift[instr->option]);
		PrintI0Opr(instr->attr,&(instr->opr[0]));
		printf(", ");
		PrintI0Opr(MATTR_UB,&(instr->opr[1]));
		printf(", ");
		PrintI0Opr(instr->attr,&(instr->opr[2]));
		putchar('\n');
		break;
	case OP_SCMP:
		printf("scmp ");
		PrintI0Opr(MATTR_UE,&(instr->opr[0]));
		printf(", ");
		PrintI0Opr(MATTR_UE,&(instr->opr[1]));
		printf(", ");
		PrintI0Opr(MATTR_UE,&(instr->opr[2]));
		printf(", ");
		PrintI0Opr(MATTR_UE,&(instr->opr[3]));
		printf(", ");
		PrintI0Opr(MATTR_UE,&(instr->opr[4]));
		putchar('\n');
		break;
	case OP_EXIT:
		printf("exit ");
		PrintHexInt8(instr->option);
		putchar('\n');
		break;
	default:
		printf("?\n");
		break;
	}
}
