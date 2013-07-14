#ifndef NATIVE_CODE_BLOCK
#define NATIVE_CODE_BLOCK

#include <stdint.h>
#include "rbtree.h"

typedef struct _STPC_HASH_TABLE_ENTRY STPC_HASH_TABLE_ENTRY;

struct _STPC_HASH_TABLE_ENTRY
{
	uint64_t spc;
	uint64_t tpc;
	STPC_HASH_TABLE_ENTRY* pnlnk[1];
};

union _NativeCodeRelocEntry;
typedef union _NativeCodeRelocEntry NativeCodeRelocEntry;

struct _NativeCodeRelocList;
typedef struct _NativeCodeRelocList NativeCodeRelocList;

struct _NativeCodeBlockDesc;
typedef struct _NativeCodeBlockDesc NativeCodeBlockDesc;

struct _NCBAvlNode;
typedef struct _NCBAvlNode NCBAvlNode;

struct _NativeCodeRelocRealEntry;
typedef struct _NativeCodeRelocRealEntry NativeCodeRelocRealEntry;

struct _NativeCodeRelocHeader;
typedef struct _NativeCodeRelocHeader NativeCodeRelocHeader;

struct 
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
};

struct
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
};

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

struct _NativeCodeBlockDesc {
	struct rb_node DescRbNode;
	NativeCodeBlockDesc* PreBlock;
	NativeCodeBlockDesc* NxtBlock;
	void* NativeCode; //XXX:should be managed manually
	NativeCodeRefSet RefedBlocks;
	IJTabRefList RefedIJTab;
	uint32_t NativeCodeSize;
	uint32_t SourceCodeSize;
	uint64_t SourceCodeBase;
	NativeCodeRelocList Relocs;
	NativeCodePartitionList Part;
};

typedef struct _NativeCodeRelocPoint
#ifndef MSVC
	__attribute__((packed))
#endif
{
	uint16_t mov_op;
	uint64_t target;
	uint16_t jmp_call_op;
}NativeCodeRelocPoint;


typedef struct _TempRelocEntry{
	struct rb_node tree_node;
	uint64_t i0_addr;
	uint64_t native_addr;
	struct _TempRelocEntry* prev;
	struct _TempRelocEntry* next;
}TempRelocEntry;

void error(const char* err_msg) 
#ifndef MSVC __attribute__((noreturn));
#else
;
#endif
#endif