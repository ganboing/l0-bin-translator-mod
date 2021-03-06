#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <malloc.h>
#include <sys/mman.h>
#include "ASM_MACROS.h"
#include "rbtree.h"
#include "DecodeI0.h"
#include "NativeCodeBlock.h"
#include "sys_config.h"
#include "VPC_env_types.h"


#define NC_BLOCK_CR_TAIL_JMP			0x01
#define NC_BLOCK_CR_TAIL_UD_HANDLER		0x02

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })
#endif

static const uint8_t nop1[1] = {0x90};
static const uint8_t nop2[2] = {0x66, 0x90};
static const uint8_t nop3[3] = {0x0f, 0x1f, 0x00};
static const uint8_t nop4[4] = {0x0f, 0x1f, 0x40, 0x00};
static const uint8_t nop5[5] = {0x0f, 0x1f, 0x44, 0x00, 0x00};
static const uint8_t nop6[6] = {0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00};
static const uint8_t nop7[7] = {0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00};
static const uint8_t nop8[8] = {0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t nop9[9] = {0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t*  const MultiNopTemplate[9] = \
{nop1, nop2, nop3, nop4, nop5, nop6, nop7, nop8, nop9};

static const uint16_t mov_imm_rax_op = 0xb848;
static const uint16_t jmp_rax_op = 0xe0ff;
static const uint16_t call_rax_op = 0xd0ff;

//Translate time working area
static TempRelocEntry TransTimeTempRelocList[1024*1024];
static uint8_t TransTimeNativeCodeCache[(MAX_TRANSLATE_BLOCK_SIZE_I0_MOD_TRAN)*16];
static unsigned int TransTimeRLCnt;
static TempRelocEntry* TransTimeRLHead;
static TempRelocEntry* TransTimeRLTail;
static struct rb_root TransTimeRLTreeRoot;

static struct rb_root GlobalNCDescRbRoot;
static NativeCodeBlockDesc* GlobalNCDescFirst;
static NativeCodeBlockDesc* GlobalNCDescLast;


STPC_HASH_TABLE_ENTRY IndirJmpHashTab[IJ_TABLE_SIZE];

#define SIZEOF_STPC_HASH_TABLE_CHAIN_ENTRY ((sizeof(STPC_HASH_TABLE_ENTRY))+sizeof(STPC_HASH_TABLE_ENTRY*))


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


void* malloc_executable(size_t size)
{
	static unsigned long pagesize = sysconf(_SC_PAGE_SIZE);
	unsigned long addr;
	void* ptr = malloc(size);
	if(!ptr)
	{
		exit(CRITICAL_MEM_LOW);
	}
	addr = ((unsigned long)ptr);
	mprotect((addr & (~(pagesize - 1))), ((uint64_t)(size)) + (addr & (pagesize - 1)), PROT_READ | PROT_WRITE | PROT_EXEC);
	return ptr;
}

static NativeCodeRelocPointJMP* CvtRelocPoint(NativeCodeRelocPos* reloc_point)
{
	if((reloc_point->jmp.mov_op) == mov_imm_rax_op)
	{
		return (&(reloc_point->jmp));
	}
	else
	{
		return (&(reloc_point->jcc.jmp_point));
	}
}

inline uint32_t SpcHashFunc(uint64_t spc)
{
	return (spc & (IJ_TABLE_SIZE - 1));
}

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

void InitTempRelocList(void)
{
	TransTimeRLTreeRoot.rb_node = NULL;
	TransTimeRLCnt = 0;
	TransTimeRLTail = TransTimeRLHead = NULL;
}

void DeleteTempReloc(TempRelocEntry* entry)
{
	//XXX:Will not delete from the rb-tree, only delete from the linked list!!!
	if(entry->prev)
	{
		(entry->prev->next) = (entry->next);
	}
	if(entry->next)
	{
		(entry->next->prev) = (entry->prev);
	}
}

void InsertTempReloc(uint64_t i0_addr, uint64_t native_addr)
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
			if(((ent->i0_addr) > i0_addr) ||
					(((ent->i0_addr) == i0_addr) && ((ent->native_addr) >native_addr)))
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
					else
					{
						TransTimeRLTail = newent;
					}
					break;
				}
			}
		}
	}
	else
	{
		(newent->prev) = (newent->next) = NULL;
		TransTimeRLTail = TransTimeRLHead = newent;
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

void DeleteFromRefSet(NativeCodeBlockDesc* ref_to_be_deleted, NativeCodeBlockDesc* block)
{
	struct rb_node* ref_set_root = (block->RefedBlocks.tree_root.rb_node);
	while(ref_set_root)
	{
		NativeCodeRefNode* ref_node = container_of(ref_set_root, NativeCodeRefNode, treenode);
		NativeCodeBlockDesc* ref_block = (ref_node->block_ptr);
		if(ref_block == ref_to_be_deleted)
		{
			rb_erase(ref_set_root, (&(block->RefedBlocks.tree_root)));
			free(ref_node);
			block->RefedBlocks.RefCnt --;
			return;
		}
		else
		{
			if((ref_block->SourceCodeBase) < (ref_to_be_deleted->SourceCodeBase))
			{
				ref_set_root = (ref_set_root->rb_right);
			}
			else
			{
				ref_set_root = (ref_set_root->rb_left);
			}
		}
	}
	assert(0); //ref_to_be_deleted not found in ref set of the block
}

void AddToRefSet(NativeCodeBlockDesc* ref_to_be_added, NativeCodeBlockDesc* block)
{
	struct rb_node** insert_pos = (&(block->RefedBlocks.tree_root.rb_node));
	struct rb_node* parent = NULL;
	while(parent = (*insert_pos))
	{
		NativeCodeBlockDesc* parent_block = container_of(parent, NativeCodeRefNode, treenode);
		if(parent_block == ref_to_be_added)
		{
			return;
		}
		if((parent_block->SourceCodeBase) < (ref_to_be_added->SourceCodeBase))
		{
			insert_pos = (&(parent->rb_right));
		}
		else
		{
			insert_pos = (&(parent->rb_left));
		}
	}
	NativeCodeRefNode* new_ref_node = (NativeCodeRefNode*)malloc(sizeof(NativeCodeRefNode));
	new_ref_node->block_ptr = ref_to_be_added;
	rb_link_node((&(new_ref_node->treenode)), parent, insert_pos);
	rb_insert_color((&new_ref_node->treenode), (&(block->RefedBlocks.tree_root)));
	block->RefedBlocks.RefCnt++;
}

void ResolveExternalDeleting(NativeCodeBlockDesc* block, NativeCodeBlockDesc* block_to_be_delete)
{
	uint32_t reloc_pos = FindFirstRelocEntryBySpc(block->Relocs, block_to_be_delete->SourceCodeBase);
	while( reloc_pos < (block->Relocs.EffecSize))
	{
		NativeCodeRelocRealEntry* reloc_ent = (&(block->Relocs.list)[reloc_pos].realentry);
		if((reloc_ent->i0_addr) < ((block_to_be_delete->SourceCodeBase) + (block_to_be_delete->SourceCodeSize)))
		{
			assert((reloc_ent->native_offset) < (block->NativeCodeSize));
			NativeCodeRelocPos* reloc_point_union  = (NativeCodeRelocPos*)(((uint64_t)(block->NativeCode)) + (reloc_ent->native_offset));
			NativeCodeRelocPointJMP* reloc_point_jmp = CvtRelocPoint(reloc_point_union);
			if((reloc_point_jmp->jmp_call_op) == jmp_rax_op)
			{
				assert((reloc_point_jmp->target) >= ((uint64_t)(block_to_be_delete->NativeCode)));
				assert((reloc_point_jmp->target) < (((uint64_t)(block_to_be_delete->NativeCode)) + (block_to_be_delete->NativeCodeSize)));
				(reloc_point_jmp->jmp_call_op) = call_rax_op;
				(reloc_point_jmp->target) = ((uint64_t)(&(reloc_ent->mov_rsi_op)));
			}
			else
			{
				assert((reloc_point_jmp->jmp_call_op) == call_rax_op);
				assert((reloc_point_jmp->target) == ((uint64_t)(&(reloc_ent->mov_rsi_op))));
			}
		}
		else
		{
			break;
		}
		reloc_pos++;
	}
}

void ResolveExternalRebasing(NativeCodeBlockDesc* block, NativeCodeBlockDesc* block_to_be_rebase, uint64_t newbase)
{
	uint64_t off64 = (newbase - ((uint64_t)block_to_be_rebase->NativeCode));
	uint32_t reloc_pos = FindFirstRelocEntryBySpc(block->Relocs, block_to_be_rebase->SourceCodeBase);
	while( reloc_pos < (block->Relocs.EffecSize))
	{
		NativeCodeRelocRealEntry* reloc_ent = (&(block->Relocs.list)[reloc_pos].realentry);
		if((reloc_ent->i0_addr) < ((block_to_be_rebase->SourceCodeBase) + (block_to_be_rebase->SourceCodeSize)))
		{
			assert((reloc_ent->native_offset) < (block->NativeCodeSize));
			NativeCodeRelocPos* reloc_point_union  = (NativeCodeRelocPos*)(((uint64_t)(block->NativeCode)) + (reloc_ent->native_offset));
			NativeCodeRelocPointJMP* reloc_point_jmp = CvtRelocPoint(reloc_point_union);
			if((reloc_point_jmp->jmp_call_op) == jmp_rax_op)
			{
				assert((reloc_point_jmp->target) >= ((uint64_t)(block_to_be_rebase->NativeCode)));
				assert((reloc_point_jmp->target) < (((uint64_t)(block_to_be_rebase->NativeCode)) + (block_to_be_rebase->NativeCodeSize)));
				(reloc_point_jmp->target) += off64;
			}
			else
			{
				assert((reloc_point_jmp->jmp_call_op) == call_rax_op);
				assert((reloc_point_jmp->target) == ((uint64_t)(&(reloc_ent->mov_rsi_op))));
			}
		}
		else
		{
			break;
		}
		reloc_pos++;
	}
}

void TraverRefBlockPrepDel(NativeCodeBlockDesc* block_to_be_delete)
{
	NativeCodeRefSet ref_set = block_to_be_delete->RefedBlocks;
	struct rb_node* node_st[64];
	uint32_t sp = 0;
	struct rb_node* node_root = ref_set.tree_root.rb_node;
	while(1)
	{
		if(node_root)
		{
			NativeCodeRefNode* ref_node = container_of(node_root, NativeCodeRefNode, treenode);
			node_st[sp++] = (node_root->rb_left);
			struct rb_node* next_root = (node_root->rb_right);
			ResolveExternalDeleting(ref_node->block_ptr, block_to_be_delete);
			free(ref_node);
			node_root = next_root;
		}
		else
		{
			if(sp)
			{
				node_root = node_st[--sp];
			}
			else
			{
				return;
			}
		}
	}
}

void TraverRefBlockRebase(NativeCodeBlockDesc* block_to_be_rebase, uint64_t newbase)
{
	uint64_t off64 = (newbase - ((uint64_t)(block_to_be_rebase->NativeCode)));
	struct rb_node* node_st[64];
	uint32_t sp = 0;
	struct rb_node*	node_root = (block_to_be_rebase->RefedBlocks.tree_root.rb_node);
	while(1)
	{
		if(node_root)
		{
			NativeCodeRefNode* ref_node = container_of(node_root, NativeCodeRefNode, treenode);
			node_st[sp++] = (node_root->rb_left);
			struct rb_node* next_root = (node_root->rb_right);
			ResolveExternalRebasing(ref_node->block_ptr, block_to_be_rebase, newbase);
			node_root = next_root;
		}
		else
		{
			if(sp)
			{
				node_root = node_st[--sp];
			}
			else
			{
				return;
			}
		}
	}
}

//#define BLOCK_DESC_TRAM_OFFSET ((uint64_t)(&((*((NativeCodeBlockDesc*)NULL)).EndOfBlockDesc[0])))

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

	//for debug
	if(newblock->PreBlock)
	{
		assert((newblock->SourceCodeBase) >= ((newblock->PreBlock->SourceCodeBase) + (newblock->PreBlock->SourceCodeSize)));
	}
	if(newblock->NxtBlock)
	{
		assert((newblock->NxtBlock->SourceCodeBase) >= ((newblock->SourceCodeBase) + (newblock->SourceCodeSize)));
	}
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

uint64_t FakeStepI0(uint64_t* i0_addr, uint64_t* native_addr)
{
	TranslateI0ToNative((uint8_t**)i0_addr,(uint8_t**)native_addr,(uint8_t*)ULLONG_MAX,(uint8_t*)ULLONG_MAX,0);
}


void DeleteNativeCodeBlock(NativeCodeBlockDesc* block)
{
	TraverRefBlockPrepDel(block);
	//remove from hash table
	uint32_t i;
	for(i=0;i<(block->RefedIJTab.EffecSize);i++)
	{
		STPC_HASH_TABLE_ENTRY* tab_ent = (block->RefedIJTab.list)[i].tabent;
		if( (((uint64_t)tab_ent) >= ((uint64_t)IndirJmpHashTab)) && (((uint64_t)tab_ent) < (((uint64_t)IndirJmpHashTab) + sizeof(IndirJmpHashTab))))
		{
			(tab_ent->spc) = 0;
		}
		else
		{
			((tab_ent->pnlnk[1])->pnlnk[0]) = (tab_ent->pnlnk[0]);
			if(tab_ent->pnlnk[0])
			{
				((tab_ent->pnlnk[0])->pnlnk[1]) = (tab_ent->pnlnk[1]);
			}
			free(tab_ent);
		}
	}
	free(block->RefedIJTab.list);
	free(block->Part.list);
	i = 1; //caution! the first entry is a header!
	while(1)
	{
		NativeCodeRelocRealEntry* reloc_ent ;
		NativeCodeRelocPointJMP* reloc_point ; 
		while( 1 )
		{
			if(i==(block->Relocs.EffecSize))
			{
				break;
			}
			reloc_ent = (&(block->Relocs.list)[i].realentry);
			assert((reloc_ent->native_offset) < (block->NativeCodeSize));
			reloc_point = CvtRelocPoint(((NativeCodeRelocPos*)(((uint64_t)(block->NativeCode)) + reloc_ent->native_offset)));
			if((reloc_point->jmp_call_op) == jmp_rax_op)
			{
				break;
			}
			assert((reloc_point->jmp_call_op) == call_rax_op);
			assert((reloc_point->target) == ((uint64_t)(&(reloc_ent->mov_rsi_op))));
			i++;
		};
		if( i == (block->Relocs.EffecSize))
		{
			break;
		}
		NativeCodeBlockDesc* target_block = FindTargetBlock(reloc_ent->i0_addr);
		assert(target_block);
		assert((reloc_ent->i0_addr) < ((target_block->SourceCodeBase) + (target_block->SourceCodeSize)));
		DeleteFromRefSet(block, target_block);
		while((reloc_ent->i0_addr) <((target_block->SourceCodeBase) + (target_block->SourceCodeSize)))
		{
			if((reloc_point->jmp_call_op) == jmp_rax_op)
			{
				assert((reloc_point->target) >= ((uint64_t)(target_block->NativeCode)));
				assert((reloc_point->target) < (((uint64_t)(target_block->NativeCode)) + (target_block->NativeCodeSize)));
			}
			else
			{
				assert((reloc_point->jmp_call_op) == call_rax_op);
				assert((reloc_point->target) == ((uint64_t)(&(reloc_ent->mov_rsi_op))));
			}
			reloc_ent++;
			assert((reloc_ent->native_offset) < (block->NativeCodeSize));
			reloc_point = CvtRelocPoint(((NativeCodeRelocPos*)(((uint64_t)(block->NativeCode)) + reloc_ent->native_offset)));
			i++;
			if( i == (block->Relocs.EffecSize))
			{
				break;
			}
		}
		if( i == (block->Relocs.EffecSize))
		{
			break;
		}
	}
	free(block->NativeCode);
	free(block->Relocs.list);
	if(block->PreBlock)
	{
		(block->PreBlock->NxtBlock) = block->NxtBlock;
	}
	else
	{
		GlobalNCDescFirst = block->NxtBlock;
	}
	if(block->NxtBlock)
	{
		(block->NxtBlock->PreBlock) = block->PreBlock;
	}
	else
	{
		GlobalNCDescLast = block->PreBlock;
	}
	rb_erase((&(block->DescRbNode)), (&GlobalNCDescRbRoot));
	free(block);
}

void MergeBlocks(NativeCodeBlockDesc* first, NativeCodeBlockDesc* second)
{
	assert(((first->SourceCodeBase) + (first->SourceCodeSize)) == (second->SourceCodeBase));

}

void PrintTempRelocList(void)
{
	printf("TempRelocList:\ni0 addr\t\tnative addr");
	TempRelocEntry* i = TransTimeRLHead;
	while(i)
	{
		printf("%lx\t\t%lx\n", i->i0_addr, i->native_addr);
		i = i->next;
	}
}

typedef struct
{
	uint64_t nativecode;
	NativeCodeBlockDesc* nc_block;
}TransReturnStatus;

NativeCodeBlockDesc* CreateNewNCBLock(void)
{
	NativeCodeBlockDesc* newblock = (NativeCodeBlockDesc*)malloc(sizeof(NativeCodeBlockDesc));
	memset(newblock, 0, sizeof(NativeCodeBlockDesc));
	return newblock;
}

TransReturnStatus Translate(uint8_t* spc, NativeCodeBlockDesc* from_block, NativeCodeRelocPointJMP* reloc_point)
{
	uint64_t spc_shadow = ((uint64_t)spc);
	if( (((uint64_t)spc) < I0_CODE_BEGIN) || (((uint64_t) spc) >= ((_pi0codemeta->i0_code_file_len) + I0_CODE_BEGIN)))
	{
		error("translate target out of range !!!");
	}
	NativeCodeBlockDesc* preblock = FindTargetBlock((uint64_t)spc);
	NativeCodeBlockDesc* nxtblock = NULL;
	unsigned is_expand_pre_block = 0;
	if(preblock)
	{
		nxtblock = preblock->NxtBlock;
		if(((uint64_t)spc) < ((preblock->SourceCodeBase) + (preblock->SourceCodeSize)))
		//jmp to an existing code block
		{
			uint32_t i0_offset = ( ((uint64_t)spc) - (preblock->SourceCodeBase));
			NativeCodePartitionList part_list = preblock->Part;
			NativeCodePartitionEntry part_ent = FindNCPartitionBySpc(part_list, i0_offset);
			uint64_t i0_addr = (preblock->SourceCodeBase) + part_ent.i0_offset;
			uint64_t native_addr = ((uint64_t)(preblock->NativeCode)) + part_ent.native_offset;
			while(i0_addr < ((uint64_t)spc))
			{
				FakeStepI0(&i0_addr, &native_addr);
			}
			if(i0_addr != ((uint64_t)spc))
			{
				if(from_block == preblock)
				{
					//suicide code translated!
					from_block = NULL;
				}
				NativeCodeBlockDesc* pre_new = preblock->PreBlock;
				DeleteNativeCodeBlock(preblock);
				preblock = pre_new;
				//!!! preblock translated some tricky code or data, kill the preblock
			}
			else
			{
				assert(from_block != preblock);
				if(from_block)
				{
					assert(reloc_point);
					(reloc_point->jmp_call_op) = jmp_rax_op;
					(reloc_point->target) = native_addr;
					AddToRefSet(from_block, preblock);
				}
				TransReturnStatus result = {native_addr, preblock};
				return result;
			}
		}
		else if( ((uint64_t)spc) == ((preblock->SourceCodeBase) + (preblock->SourceCodeSize)))
		{
			if(from_block == preblock)
			{
				from_block = NULL;
			}
			//expand the preblock!!
			is_expand_pre_block = 1;
		}
	}
	else
	{
		nxtblock = GlobalNCDescFirst;
	}
	uint64_t i0_limit;
	if(nxtblock)
	{
		i0_limit = (nxtblock->SourceCodeBase);
	}
	else
	{
		i0_limit = (_pi0codemeta->i0_code_file_len + I0_CODE_BEGIN);
	}
	unsigned is_block_limit_effec = 0;
	if( (i0_limit - ((uint64_t)spc)) > MAX_TRANSLATE_BLOCK_SIZE_I0_MOD_TRAN)
	{
		i0_limit = ((uint64_t)spc) + MAX_TRANSLATE_BLOCK_SIZE_I0_MOD_TRAN;
		is_block_limit_effec = 1;
	}
	uint64_t nativelimit = 0;
	DECODE_STATUS decode_status;
	TempRelocEntry* nxt_xref = NULL;
	uint64_t cr_flag = 0;
	InitTempRelocList();
	unsigned is_expand_nxt_block = 0;
	while (1)
	{
		decode_status = TranslateI0ToNative((&spc), NULL, (&nativelimit), ((uint8_t*)i0_limit), 0);
		if( decode_status.status == I0_DECODE_SEGMENT_LIMIT) 
		{
			if(((uint64_t)spc) == spc_shadow)
			{
				if(nxtblock)
				{
					if(from_block == nxtblock)
					{
						from_block = NULL;
					}
					NativeCodeBlockDesc* nxt_new = nxtblock->NxtBlock;
					DeleteNativeCodeBlock(nxtblock);
					nxtblock = nxt_new;
					if(nxtblock)
					{
						i0_limit = (nxtblock->SourceCodeBase);
					}
					else
					{
						i0_limit = (_pi0codemeta->i0_code_file_len + I0_CODE_BEGIN);
					}
					is_block_limit_effec = 0;
					if( (i0_limit - ((uint64_t)spc)) > MAX_TRANSLATE_BLOCK_SIZE_I0_MOD_TRAN)
					{
						i0_limit = ((uint64_t)spc) + MAX_TRANSLATE_BLOCK_SIZE_I0_MOD_TRAN;
						is_block_limit_effec = 1;
					}
				}
				else
				{
					decode_status.status = I0_DECODE_INVALID_INSTRUCTION;
					break;
				}
			}
			else
			{
				break;
			}
		}
		if(decode_status.status == I0_DECODE_INVALID_INSTRUCTION)
		{
			break;
		}
		if( (decode_status.status == I0_DECODE_BRANCH_UNCOND) || (decode_status.status == I0_DECODE_BRANCH_COND))
		{
			NativeCodeRelocPos* nc_reloc_point;
			switch(decode_status.detail)
			{
			case I0_DECODE_JCC:
				nc_reloc_point = ((NativeCodeRelocPos*)(decode_status.detail2));
				InsertTempReloc((nc_reloc_point->jcc.jmp_point.target) , decode_status.detail2);
				break;
			case I0_DECODE_JMP:
				nc_reloc_point = ((NativeCodeRelocPos*)(decode_status.detail2));
				InsertTempReloc((nc_reloc_point->jmp.target) , decode_status.detail2);
				break;
			}
		}
		if(nxt_xref)
		{
			do{
				if( (nxt_xref->i0_addr) < ((uint64_t)spc))
				{
					nxt_xref = nxt_xref->next;
				}
				else
				{
					break;
				}
			}while(nxt_xref);
		}
		else
		{
			if(TransTimeRLTail &&	( (TransTimeRLTail->i0_addr) >= ((uint64_t)spc)))
			{
				nxt_xref = TransTimeRLTail;
			}
		}
		if( (((uint64_t) spc) - spc_shadow) > MIN_TRANSLATE_BLOCK_SIZE_I0_MOD_TRAN)
		{
			if(decode_status.status == I0_DECODE_BRANCH_UNCOND)
			{
				if((!nxt_xref) || ( ((nxt_xref->i0_addr) - ((uint64_t)spc)) > MAX_TRANSLATE_OVER_LOOK_CODE_SIZE))
				{
					break;
				}
			}
		}
	}
	if((nxtblock) && (((uint64_t)spc) == (nxtblock->SourceCodeBase)))
	{
		is_expand_nxt_block = 1;
	}
	switch(decode_status.status)
	{
	case I0_DECODE_INVALID_INSTRUCTION:
		cr_flag = NC_BLOCK_CR_TAIL_UD_HANDLER;
		AppendUDHandler(NULL, (&nativelimit), 0,0,0); 
		break;
	case I0_DECODE_SEGMENT_LIMIT:
		if(!is_expand_nxt_block)
		{
			cr_flag = NC_BLOCK_CR_TAIL_JMP;
			AppendTailJump(NULL, (&nativelimit), 0,0);
			InsertTempReloc(((uint64_t)spc), nativelimit);
		}
		break;
	}
	switch((is_expand_pre_block<<1) + (is_expand_nxt_block))
	{
	case 0: //create a new block
		{
			
		}
		break;
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

inline NativeCodePartitionEntry FindNCPartitionBySpc(NativeCodePartitionList list, uint32_t i0_offset)
{
	uint32_t size = (list.EffecSize);
	uint32_t i = 0;
	NativeCodePartitionEntry result0 = {0,0};
	if(size)
	{
		while(size)
		{
			uint32_t value = list.list[ i + (size/2)].i0_offset;
			if(value<=i0_offset)
			{
				i += ((size/2)+1);
				size = ((size-1)/2);
			}
			else
			{
				size /= 2;
			}
		}
		if(i)
		{
			return list.list[i-1];
		}
		else
		{
			return result0;
		}
	}
	else
	{
		return result0;
	}
}

inline uint32_t FindFirstRelocEntryBySpc(NativeCodeRelocList list,uint64_t spc)
{
	if((list.list))
	{
		uint32_t size = (list.EffecSize) - 1;
		NativeCodeRelocEntry* arr = ((list.list) + 1);
		uint32_t i = 0;
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
	uint32_t i;
	uint64_t offset64 = (newbase - ((uint64_t)(desc->NativeCode)));
	if(offset64)
	{
		// relocate jmp addr in the blocks that referencing the current block
		TraverRefBlockRebase(desc, newbase);
		// rebase the hash table entries
		IJTabRefEntry* ijtabent = (desc->RefedIJTab).list;
		for(i=0;i<((desc->RefedIJTab).EffecSize);i++)
		{
			((ijtabent[i]).tabent)->tpc += offset64;
		}
	}
}
