#ifndef ASM_MACROS_H_
#define ASM_MACROS_H_

#define STR(s) #s
#define XSTR(s) STR(s)

#define SHLD_SHL(dest,src,n) \
	__asm__ (\
		"and $0, %0\n\t"\
		"shld $("STR(n)"), %1, %0\n\t"\
		"shl $("STR(n)"), %1\n\t"\
		:"=g" (dest), "+r"(src) \
	)

#define GET_INST_FIELD_NW(dest,src,n) \
	__asm__ (\
		"andl $0, %0\n\t"\
		"shldl $("STR(n)"), %1, %0\n\t"\
		: "=&g" (dest)\
		: "r" (src)\
	)

#define GET_INST_FIELD_SH(dest,src,n) \
	__asm__ (\
		"andl $0, %0\n\t"\
		"shldl $("STR(n)"), %1, %0\n\t"\
		"shll $("STR(n)"), %1\n\t"\
		:"=g" (dest), "+r"(src) \
	)

#define GET_INST_FIELD_ZO(dest,src,n) \
	__asm__ (\
		"andl $0, %0\n\t"\
		"shldl $("STR(n)"), %1, %0\n\t"\
		"andl $(((1<<32)-1)>>("STR(n)")), %1\n\t"\
		:"=g" (dest), "+r"(src)\
	)

#define LOAD_OP_DWORD_AND_SH(op,ptr,n) \
	__asm__ (\
		"movl 0(%1), %0\n\t"\
		"bswapl %0\n\t"\
		"shll $("STR(n)"), %0\n\t"\
		: "=r" (op)\
		: "r" (ptr)\
	)

#define LOAD_OP_WORD0(op,ptr) \
	__asm__ (\
		"movw 0(%1), %w0\n\t"\
		"bswapl %0\n\t"\
		:"=r" (op)\
		:"r" (ptr)\
	)

#define LOAD_OP_BYTE2(op,ptr) \
	__asm__ (\
		"movw 1(%1), %w0\n\t"\
		:"+r" (op)\
		:"r" (ptr)\
	)

#define LOAD_OP_BYTE3(op,ptr) \
	__asm__ (\
		"movb 3(%1), %b0\n\t"\
		:"+r" (op)\
		:"r" (ptr)\
	)

#define GET_INST_OPR_I64(opr,t) \
	do{ \
		(opr).val = *((uint64_t*)(t)); \
		(t) += 8; \
	}while(0)

#define GET_INST_OPR_I8(opr,t) \
	do{ \
		(opr).val = *((uint8_t*)(t)); \
		(t) += 1; \
	}while(0)

#endif /* ASM_MACROS_H_ */
