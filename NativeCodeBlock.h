#ifndef NATIVE_CODE_BLOCK
#define NATIVE_CODE_BLOCK

#include <stdint.h>
#include "rbtree.h"

#define TEST_STRUCT_SIZE(TYPE, SIZE)\
	struct __TestTheStructSizeUsingZeroLengthArray_Struct_##TYPE { \
	char TYPE##SizeSmallerThanExpected[(SIZE) - (long)sizeof(TYPE)];\
	char TYPE##SizeLargerThanExpected[(long)sizeof(TYPE) - (SIZE)];\
};

typedef struct _STPC_HASH_TABLE_ENTRY STPC_HASH_TABLE_ENTRY;

struct _STPC_HASH_TABLE_ENTRY
{
	uint64_t spc;
	uint64_t tpc;
	STPC_HASH_TABLE_ENTRY* pnlnk[1];
};

TEST_STRUCT_SIZE(STPC_HASH_TABLE_ENTRY, 3*8);

union _NativeCodeRelocEntry;
typedef union _NativeCodeRelocEntry NativeCodeRelocEntry;

struct _NativeCodeRelocList;
typedef struct _NativeCodeRelocList NativeCodeRelocList;

struct _NativeCodeBlockDesc;
typedef struct _NativeCodeBlockDesc NativeCodeBlockDesc;

struct _NCBAvlNode;
typedef struct _NCBAvlNode NCBAvlNode;

typedef struct 
#ifndef MSVC 
__attribute__ ((aligned(4),packed))  
#endif
_NativeCodeRelocRealEntry {
	uint32_t native_offset;
	uint8_t pad1;
	uint16_t mov_rsi_op; //0x48 0xbe movq imm64, %rsi
	uint64_t i0_addr;
	uint8_t jmp_op; // 0xe9 jmp rel32
	uint32_t rel32;
}NativeCodeRelocRealEntry;

TEST_STRUCT_SIZE(NativeCodeRelocRealEntry, 20);

typedef struct
#ifndef MSVC
__attribute__ ((aligned(4),packed))  
#endif
_NativeCodeRelocHeader{
	uint16_t mov_rdi_op; //0x48 0xbf movq imm64, %rdi
	uint64_t NC_desc_ptr;
	uint8_t mov_eax_op; // 0xb8 movl imm32, %eax
	uint32_t func_ptr;
	uint16_t call_rax_op; //0xff 0xd0
	uint8_t padding[3];
}NativeCodeRelocHeader;

TEST_STRUCT_SIZE(NativeCodeRelocHeader, 20);

union
#ifndef MSVC 
__attribute__ ((aligned(4),packed))  
#endif
_NativeCodeRelocEntry{
	NativeCodeRelocRealEntry realentry;
	NativeCodeRelocHeader header;
};

struct _NativeCodeRelocList{
	NativeCodeRelocEntry* list;
	uint32_t EffecSize;
	uint32_t AllocSize;
};

typedef struct _IJTabRefEntry{
	STPC_HASH_TABLE_ENTRY* tabent;
}IJTabRefEntry;

typedef struct _IJTabRefList{
	IJTabRefEntry* list;
	uint32_t EffecSize;
	uint32_t AllocSize;
}IJTabRefList;

typedef struct _NativeCodeRefNode{
	struct rb_node treenode;
	NativeCodeBlockDesc* block_ptr;
}NativeCodeRefNode;

typedef struct _NativeCodeRefSet{
	struct rb_root tree_root;
}NativeCodeRefSet;

typedef struct _NativeCodePartitionEntry
{
	uint32_t native_offset;
	uint32_t i0_offset;
}NativeCodePartitionEntry;

typedef struct _NativeCodePartitionList
{
	NativeCodePartitionEntry* list;
	uint32_t EffecSize;
	uint32_t AllocSize;
}NativeCodePartitionList;

#define NC_BLOCK_CR_TAIL_JMP	0x01

struct _NativeCodeBlockDesc {
	struct rb_node DescRbNode;
	NativeCodeBlockDesc* PreBlock;
	NativeCodeBlockDesc* NxtBlock;
	void* NativeCode; //XXX:should be managed manually
	uint64_t SourceCodeBase;
	NativeCodeRefSet RefedBlocks;
	IJTabRefList RefedIJTab;
	uint32_t NativeCodeSize;
	uint32_t SourceCodeSize;
	NativeCodeRelocList Relocs;
	NativeCodePartitionList Part;
	uint64_t CR_FLAGS;
};

typedef struct
#ifndef MSVC
	__attribute__((aligned(1),packed))
#endif
	_NativeCodeRelocPointJMP
{
	uint16_t mov_op;
	uint64_t target;
	uint16_t jmp_call_op;
}NativeCodeRelocPointJMP;

TEST_STRUCT_SIZE(NativeCodeRelocPointJMP, 12);

typedef struct
#ifndef MSVC
	__attribute__((aligned(1), packed))
#endif
	 _NativeaCodeRelocPointJCC
{
	uint8_t jcc_op_tttn;
	uint8_t off8;
	NativeCodeRelocPointJMP jmp_point;
}NativeCodeRelocPointJCC;

TEST_STRUCT_SIZE(NativeCodeRelocPointJCC, 12+2);

typedef union
#ifndef MSVC
	__attribute__((aligned(1), packed))
#endif
	 _NativeaCodeRelocPos
{
	NativeCodeRelocPointJMP jmp;
	NativeCodeRelocPointJCC jcc;
}NativeCodeRelocPos;

typedef struct _TempRelocEntry{
	struct rb_node tree_node;
	uint64_t i0_addr;
	uint64_t native_addr;
	struct _TempRelocEntry* prev;
	struct _TempRelocEntry* next;
}TempRelocEntry;

void error(const char* err_msg) 
#ifndef MSVC
__attribute__((noreturn));
#else
;
#endif

#endif