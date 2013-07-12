#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "ASM_MACROS.h"
#include "rbtree.h"
#include "DecodeI0.h"

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })
#endif

const uint8_t nop1[1] = {0x90};
const uint8_t nop2[2] = {0x66, 0x90};
const uint8_t nop3[3] = {0x0f, 0x1f, 0x00};
const uint8_t nop4[4] = {0x0f, 0x1f, 0x40, 0x00};
const uint8_t nop5[5] = {0x0f, 0x1f, 0x44, 0x00, 0x00};
const uint8_t nop6[6] = {0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00};
const uint8_t nop7[7] = {0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00};
const uint8_t nop8[8] = {0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t nop9[9] = {0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00};

const uint8_t*  const MultiNopTemplate[9] = \
{nop1, nop2, nop3, nop4, nop5, nop6, nop7, nop8, nop9};



void* malloc_executable(size_t size)
{
	static unsigned long pagesize = sysconf(_SC_PAGE_SIZE);
	uint64_t addr;
	void* ptr = malloc(size);
	addr = ((uint64_t)ptr);
	mprotect((addr & (~(pagesize - 1))), ((uint64_t)(size)) + (addr & (pagesize - 1)), PROT_READ | PROT_WRITE | PROT_EXEC);
	return ptr;
}

typedef struct _STPC_HASH_TABLE_ENTRY STPC_HASH_TABLE_ENTRY;

struct _STPC_HASH_TABLE_ENTRY
{
	uint64_t spc;
	uint64_t tpc;
	STPC_HASH_TABLE_ENTRY* pnlnk[1];
};

#define IJ_TABLE_SIZE 0x4000

STPC_HASH_TABLE_ENTRY IndirJmpHashTab[IJ_TABLE_SIZE];

#define SIZEOF_STPC_HASH_TABLE_CHAIN_ENTRY ((sizeof(STPC_HASH_TABLE_ENTRY))+sizeof(STPC_HASH_TABLE_ENTRY*))

inline uint32_t SpcHashFunc(uint64_t spc)
{
	return (spc & (IJ_TABLE_SIZE - 1));
}

#define CRITICAL_MEM_LOW 0x02

#define INC_LIST_ENTRY(listdesc) \
do{\
	__typeof((listdesc).list) newptr;\
	if((listdesc).EffecSize == (listdesc).AllocSize)\
	{\
		if( ((listdesc).EffecSize) >= ((4ULL*1024ULL) / sizeof(__typeof((listdesc).list[0]))))\
		{\
			newptr = (__typeof((listdesc).list))realloc(\
				((listdesc).EffecSize) * sizeof(__typeof((listdesc).list[0]))\
				+ ((4ULL*1024ULL)\
				/ sizeof(__typeof((listdesc).list[0]))\
				* sizeof(__typeof((listdesc).list[0])))\
			);\
			if(newptr == NULL)\
			{\
				exit(CRITICAL_MEM_LOW);\
			}\
			else\
			{\
				(listdesc).EffecSize += ((4ULL*1024ULL) / sizeof(__typeof((listdesc).list[0])));\
			}\
		}\
		else\
		{\
			newptr = (__typeof((listdesc).list))realloc(((listdesc).EffecSize)*sizeof(__typeof((listdesc).list[0]))*2);\
			if(newptr == NULL)\
			{\
				exit(CRITICAL_MEM_LOW);\
			}\
			else\
			{\
				(listdesc).EffecSize *= 2;\
			}\
		}\
		(listdesc).list = newptr;\
	}\
}while(0)

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
	uint8_t mov_rsi_op[2]; //0x48 0xbe movq imm64, %rsi
	uint64_t i0_addr;
	uint8_t jmp_op; // 0xe9 jmp rel32
	uint32_t rel32;
};

struct
#ifndef MSVC
__attribute__ ((aligned(4),packed))  
#endif
_NativeCodeRelocHeader{
	uint8_t mov_rdi_op[2]; //0x48 0xbf movq imm64, %rdi
	uint64_t NC_desc_ptr;
	uint8_t mov_eax_op; // 0xb8 movl imm32, %eax
	uint32_t func_ptr;
	uint8_t call_rax_op[2]; //0xff 0xd0
	uint8_t padding[3];
};

void __swap_NativeCodeRelocRealEntry(NativeCodeRelocRealEntry* a, NativeCodeRelocRealEntry* b)
{
	uint32_t tmp_native_offset;
	uint64_t tmp_i0_addr;
	tmp_native_offset = a->native_offset;
	a->native_offset = b->native_offset;
	b->native_offset = tmp_native_offset;
	tmp_i0_addr = a->i0_addr;
	a->i0_addr = b->i0_addr;
	b->i0_addr = tmp_i0_addr;
}

typedef struct _TempRelocEntry{
	struct rb_node tree_node;
	uint64_t i0_addr;
	uint64_t native_addr;
	struct _TempRelocEntry* prev;
	struct _TempRelocEntry* next;
}TempRelocEntry;

TempRelocEntry TransTimeTempRelocList[1024*1024];
unsigned int TransTimeRLCnt;
TempRelocEntry* TransTimeRLHead;
struct rb_root TransTimeRLTreeRoot;

void InitTempRelocList(void)
{
	TransTimeRLTreeRoot.rb_node = NULL;
	TransTimeRLCnt = 0;
	TransTimeRLHead = NULL;
}


void InsertTempRelocList(uint64_t i0_addr, uint64_t native_addr)
{
	struct rb_node** insert_pos = (&(TransTimeRLTreeRoot.rb_node));
	TempRelocEntry* newent = TransTimeTempRelocList + TransTimeRLCnt;
	(newent->native_addr) = native_addr;
	(newent->i0_addr) = i0_addr;
	struct rb_node* parent = NULL;
	if(TransTimeRLCnt)
	{
		while(1)
		{
			parent = (*insert_pos);
			TempRelocEntry* ent = container_of(parent, TempRelocEntry,tree_node);
			if((ent->i0_addr) > i0_addr)
			{
				insert_pos = (&(parent->rb_left));
				if(!(*insert_pos))
				{
					newent->next = ent;
					newent->prev = ent->prev;
					ent->prev = newent;
					if(newent->prev)
					{
						(newent->prev->next) = newent;
					}
					else
					{
						TransTimeRLHead = newent;
					}
					break;
				}
			}
			else
			{
				insert_pos = (&(parent->rb_right));
				if(!(*insert_pos))
				{
					newent->prev = ent;
					newent->next = ent->next;
					ent->next = newent;
					if(newent->next)
					{
						(newent->next->prev) = newent;
					}
					break;
				}
			}
		}
	}
	else
	{
		(newent->prev) = (newent->next) = NULL;
		TransTimeRLHead = newent;
	}
	rb_link_node((&(newent->tree_node)), parent, insert_pos);
	rb_insert_color((&(newent->tree_node)),(&(TransTimeRLTreeRoot))); 
	TransTimeRLCnt++;
}

int __cmp_NativeCodeRelocRealEntry(NativeCodeRelocRealEntry* a, NativeCodeRelocRealEntry* b)
{
	if((a->i0_addr) != (b->i0_addr))
	{
		if((a->i0_addr) < (b->i0_addr))
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	return 0;
}

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

typedef struct _RefedBlockList{
	NativeCodeBlockDesc** list;
	uint32_t EffecSize;
	uint32_t AllocSize;
}RefedBlockList;

struct rb_root GlobalNCDescRbRoot;
NativeCodeBlockDesc* GlobalNCDescFirst;
NativeCodeBlockDesc* GlobalNCDescLast;

struct _NativeCodeBlockDesc {
	struct rb_node DescRbNode;
	NativeCodeBlockDesc* PreBlock;
	NativeCodeBlockDesc* NxtBlock;
	void* NativeCode; //XXX:should be managed manually
	RefedBlockList RefedBlocks;
	IJTabRefList RefedIJTab;
	uint32_t NativeCodeSize;
	uint32_t SourceCodeSize;
	uint64_t SourceCodeBase;
	NativeCodeRelocList Refs;
	uint8_t Trampoline[2+8+2];
	uint8_t EndOfBlockDesc[0];
};


#define BLOCK_DESC_TRAM_OFFSET ((uint64_t)(&((*((NativeCodeBlockDesc*)NULL)).EndOfBlockDesc[0])))

void InsertNativeCodeBlock(NativeCodeBlockDesc* newblock)
{
	struct rb_node** insert_pos = (&(GlobalNCDescRbRoot.rb_node));
	struct rb_node* parent = NULL;
	if(GlobalNCDescRbRoot.rb_node)
	{
		while(1)
		{
			parent = (*insert_pos);
			NativeCodeBlockDesc* parent_block = container_of(parent, NativeCodeBlockDesc, DescRbNode);
			if((parent_block->SourceCodeBase) < (newblock->SourceCodeBase))
			{
				insert_pos = (&(parent->rb_right));
				if(!(*insert_pos))
				{
					newblock->PreBlock = parent_block;
					newblock->NxtBlock = parent_block->NxtBlock;
					parent_block->NxtBlock = newblock;
					if(newblock->NxtBlock)
					{
						(newblock->NxtBlock->PreBlock) = newblock;
					}
					else
					{
						GlobalNCDescLast = newblock;
					}
					break;
				}
			}
			else
			{
				insert_pos = (&(parent->rb_left));
				if(!(*insert_pos))
				{
					newblock->NxtBlock = parent_block;
					newblock->PreBlock = parent_block->PreBlock;
					parent_block->PreBlock = newblock;
					if(newblock->PreBlock)
					{
						(newblock->PreBlock->NxtBlock) = newblock;
					}
					else
					{
						GlobalNCDescFirst = newblock;
					}
					break;
				}
			}
		}
	}
	else
	{
		GlobalNCDescFirst = GlobalNCDescLast = newblock;
	}
	rb_link_node((&(newblock->DescRbNode)), parent, insert_pos);
	rb_insert_color((&(newblock->DescRbNode)), (&(GlobalNCDescRbRoot)));
	return;
}

NativeCodeBlockDesc* FindTargetBlock(uint64_t spc)
{
	struct rb_node* root = GlobalNCDescRbRoot.rb_node;
	if(!root)
	{
		return NULL;
	}
	while(1)
	{
		NativeCodeBlockDesc* ncblock = container_of(root, NativeCodeBlockDesc, DescRbNode);
		if(((uint64_t)(ncblock->SourceCodeBase)) == spc)
		{
			return ncblock;
		}
		else
		{
			if(((uint64_t)(ncblock->SourceCodeBase)) < spc)
			{
				if(root->rb_right)
				{
					root = (root->rb_right);
				}
				else
				{
					return ncblock;
				}
			}
			else
			{
				if(root->rb_left)
				{
					root = (root->rb_left);
				}
				else
				{
					return ncblock->PreBlock;
				}
			}
		}
	}
}

uint64_t FakeStepI0(uint64_t i0_addr)
{
	uint64_t spc = i0_addr;
	TranslateI0ToNative();
}

uint64_t Translate(uint64_t spc, NativeCodeBlockDesc* from_block)
{
	NativeCodeBlockDesc* preblock = FindTargetBlock(spc);
	NativeCodeBlockDesc* nxtblock ;
	if(preblock)
	{
		if(spc<((preblock->SourceCodeBase) + (preblock->SourceCodeSize)))
		//jmp to a existing code block
		{

		}
		nxtblock = preblock->NxtBlock;
	}
	else
	{
		nxtblock = GlobalNCDescFirst;
	}
	
}

uint64_t JmpFromNativeBlock(uint64_t srcblock, uint64_t spc, uint64_t tpc)
{
	srcblock -= (BLOCK_DESC_TRAM_OFFSET);
	NativeCodeBlockDesc* blockdesc = (NativeCodeBlockDesc*)(srcblock);

}

void AddIndirJmpTabEntry(uint64_t spc, uint64_t tpc, NativeCodeBlockDesc* desc)
{
	STPC_HASH_TABLE_ENTRY* entry = (IndirJmpHashTab + SpcHashFunc(spc));
	STPC_HASH_TABLE_ENTRY* i;
	if((entry->spc) != spc)
	{
		if(entry->spc)
		{
			i = (entry->pnlnk)[0];
			while(i)
			{
				if((i->spc) == spc)
				{
					return;
				}
			}
			i = (STPC_HASH_TABLE_ENTRY*)malloc(SIZEOF_STPC_HASH_TABLE_CHAIN_ENTRY);
			(i->pnlnk[0]) = (entry->pnlnk[0]);
			(i->pnlnk[1]) = entry;
			(entry->pnlnk[0]) = i;
			if((i->pnlnk[0]))
			{
				(i->pnlnk[0])->pnlnk[1] = i;
			}
			(i->spc) = spc;
			(i->tpc) = tpc;
		}
		else
		{
			(entry->tpc) = tpc;
			(entry->spc) = spc;
		}
	}
}

inline uint32_t FindFirstEntryBySpc(NativeCodeRelocList list,uint64_t spc)
{
	uint32_t size = (list.EffecSize) - 1;
	NativeCodeRelocEntry* arr = ((list.list) + 1);
	uint32_t i = 0;
	if(size)
	{
		while(size)
		{
			uint64_t value = arr[ i + (size/2)].realentry.i0_addr;
			if(value<spc)
			{
				i += ((size/2)+1);
				size = ((size - 1)/2);
			}
			else
			{
				size /= 2;
			}
		}
		return (i+1);
	}
	else
	{
		return 0;
	}
}

#define RELOC_ENTRY_VALID	0x01


void RebaseNativeCodeBlock(NativeCodeBlockDesc* desc, uint64_t newbase)
{
	uint64_t nativecodelimit = (((uint64_t)(desc->NativeCode)) + (desc->NativeCodeSize));
	uint32_t i;
	uint64_t offset64 = (newbase - ((uint64_t)(desc->NativeCode)));
	// relocate jmp addr in the blocks that referencing the current block
	for(i=0;i<((desc->RefedBlocks).EffecSize);i++)
	{
		NativeCodeBlockDesc* refer = ((desc->RefedBlocks).list)[i];
		NativeCodeRelocList reloclist = refer->Refs;
		uint32_t j = FindFirstEntryBySpc(reloclist, (desc->SourceCodeBase));
		while((j<(reloclist.EffecSize)))
		{
			if((reloclist.list)[j].realentry.i0_addr < nativecodelimit)
			{
				uint8_t* reloc_point = (uint8_t*)(((uint64_t)(refer->NativeCode)) + (reloclist.list)[j].realentry.native_offset);
				if(((*reloc_point) == 0x48) && ((*(reloc_point + 1)) == 0xb8) && ((*(reloc_point + 11)) == 0xe0))
				{
					(*((uint64_t*)(reloc_point + 2))) += offset64;
				}
				j++;
			}
			else
			{
				break;
			}
		}
	}
	// rebase the hash table entries
	IJTabRefEntry* ijtabent = (desc->RefedIJTab).list;
	for(i=0;i<((desc->RefedIJTab).EffecSize);i++)
	{
		((ijtabent[i]).tabent)->tpc += offset64;
	}
}
