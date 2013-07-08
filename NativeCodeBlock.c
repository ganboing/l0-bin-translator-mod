#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ASM_MACROS.h"
#include "rbtree.h"


enum NATIVE_CODE_BLOCK_SIZE{
	NATIVE_CODE_BLOCK_4K = 0,
	NATIVE_CODE_BLOCK_8K = 1,
	NATIVE_CODE_BLOCK_16K = 2,
	NATIVE_CODE_BLOCK_32K = 3,
	NATIVE_CODE_BLOCK_64K = 4,
	NATIVE_CODE_BLOCK_128K = 5,
	NATIVE_CODE_BLOCK_256K = 6,
	NATIVE_CODE_BLOCK_512K = 7,
	NATIVE_CODE_BLOCK_SIZE_NUM
};

static unsigned char NativeCodePool[(1ULL)<<(31)] __attribute__((aligned(4096)));

static volatile uint8_t NativeCodePool4k_lock = 0;
static volatile uint64_t NativeCodePool4k_Level2BitMap[16*64] = {[0 ... 16*64-1] = -1};
static volatile uint64_t NativeCodePool4k_Level1BitMap[16] = {[0 ... 16-1] = -1};
static volatile uint16_t NativeCodePool4k_Level0BitMap = -1;

static volatile uint8_t NativeCodePool8k_lock = 0;
static volatile uint64_t NativeCodePool8k_Level2BitMap[16*32] = {[0 ... 16*32-1] = -1};
static volatile uint32_t NativeCodePool8k_Level1BitMap[16] = {[0 ... 16-1] = -1};
static volatile uint16_t NativeCodePool8k_Level0BitMap = -1;

static volatile uint8_t NativeCodePool16k_lock = 0;
static volatile uint64_t NativeCodePool16k_Level2BitMap[16*16] = {[0 ... 16*16-1] = -1};
static volatile uint16_t NativeCodePool16k_Level1BitMap[16] = {[0 ... 16-1] = -1};
static volatile uint16_t NativeCodePool16k_Level0BitMap = -1;

static volatile uint8_t NativeCodePool32k_lock = 0;
static volatile uint32_t NativeCodePool32k_Level2BitMap[16*16] = {[0 ... 16*16-1] = -1};
static volatile uint16_t NativeCodePool32k_Level1BitMap[16] = {[0 ... 16-1] = -1};
static volatile uint16_t NativeCodePool32k_Level0BitMap = -1;

static volatile uint8_t NativeCodePool64k_lock = 0;
static volatile uint64_t NativeCodePool64k_Level1BitMap[64] = {[0 ... 64-1] = -1};
static volatile uint64_t NativeCodePool64k_Level0BitMap = -1;

static volatile uint8_t NativeCodePool128k_lock = 0;
static volatile uint64_t NativeCodePool128k_Level1BitMap[32] = {[0 ... 32-1] = -1};
static volatile uint32_t NativeCodePool128k_Level0BitMap = -1;

static volatile uint8_t NativeCodePool256k_lock = 0;
static volatile uint64_t NativeCodePool256k_Level1BitMap[16] = {[0 ... 16-1] = -1};
static volatile uint16_t NativeCodePool256k_Level0BitMap = -1;

static volatile uint8_t NativeCodePool512k_lock = 0;
static volatile uint32_t NativeCodePool512k_Level1BitMap[16] = {[0 ... 16-1] = -1};
static volatile uint16_t NativeCodePool512k_Level0BitMap = -1;


void* __NativeCodePoolAlloc4k(void)
{
	/*entering CS*/
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool4k_lock, newlock);
	}while(newlock);

	if(NativeCodePool4k_Level0BitMap)
	{
		uint16_t l0idx_16;
		uint64_t l1idx_64;
		uint64_t l2idx_64;
		unsigned long l0idx;
		unsigned long l1idx;
		unsigned long l2idx;
		BIT_SCAN_16_VOLATILE(l0idx_16, NativeCodePool4k_Level0BitMap);
		l0idx = ((unsigned long)l0idx_16);
		BIT_SCAN_64_VOLATILE(l1idx_64, NativeCodePool4k_Level1BitMap[l0idx]);
		l1idx = ((unsigned long)l1idx_64);
		BIT_SCAN_64_VOLATILE(l2idx_64, NativeCodePool4k_Level2BitMap[l0idx*64+l1idx]);
		BIT_RESET_64_VOLATILE(NativeCodePool4k_Level2BitMap[l0idx*64+l1idx], l2idx_64);
		l2idx = ((unsigned long)l2idx_64);
		if(!(NativeCodePool4k_Level2BitMap[l0idx*64+l1idx]))
		{
			BIT_RESET_64_VOLATILE(NativeCodePool4k_Level1BitMap[l0idx], l1idx_64);
			if(!(NativeCodePool4k_Level1BitMap[l0idx]))
			{
				BIT_RESET_16_VOLATILE(NativeCodePool4k_Level0BitMap, l0idx_16);
			}
		}
		MFENCE();
		NativeCodePool4k_lock = 0;
		return (void*)(NativeCodePool+((1ULL)<<(28ULL))*0+(l0idx*64*64+l1idx*64+l2idx)*4ULL*1024ULL);
	}
	LFENCE();
	NativeCodePool4k_lock = 0;
	return 0;
}

void __NativeCodePoolFree4k(unsigned long pooloffset)
{
	pooloffset /= (4ULL*1024ULL);
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool4k_lock, newlock);
	}while(newlock);

	unsigned long l2start = (pooloffset/64);
	unsigned long l2idx = (pooloffset%64);
	uint64_t l2idx_64 = (uint64_t)l2idx;
	pooloffset/=64;
	unsigned long l1start = (pooloffset/64);
	unsigned long l1idx = (pooloffset%64);
	uint64_t l1idx_64 = (uint64_t)l1idx;
	pooloffset/=64;
	unsigned long l0idx = pooloffset;
	uint16_t l0idx_16 = (uint16_t)l0idx;

	BIT_SET_64_VOLATILE(NativeCodePool4k_Level2BitMap[l2start],l2idx_64);
	BIT_SET_64_VOLATILE(NativeCodePool4k_Level1BitMap[l1start],l1idx_64);
	BIT_SET_16_VOLATILE(NativeCodePool4k_Level0BitMap,l0idx_16);

	SFENCE();
	NativeCodePool4k_lock = 0;
	return;
}

void* __NativeCodePoolAlloc8k(void)
{
	/*entering CS*/
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool8k_lock, newlock);
	}while(newlock);

	if(NativeCodePool8k_Level0BitMap)
	{
		uint16_t l0idx_16;
		uint32_t l1idx_32;
		uint64_t l2idx_64;
		unsigned long l0idx;
		unsigned long l1idx;
		unsigned long l2idx;
		BIT_SCAN_16_VOLATILE(l0idx_16, NativeCodePool8k_Level0BitMap);
		l0idx = ((unsigned long)l0idx_16);
		BIT_SCAN_32_VOLATILE(l1idx_32, NativeCodePool8k_Level1BitMap[l0idx]);
		l1idx = ((unsigned long)l1idx_32);
		BIT_SCAN_64_VOLATILE(l2idx_64, NativeCodePool8k_Level2BitMap[l0idx*32+l1idx]);
		BIT_RESET_64_VOLATILE(NativeCodePool8k_Level2BitMap[l0idx*32+l1idx], l2idx_64);
		l2idx = ((unsigned long)l2idx_64);
		if(!(NativeCodePool8k_Level2BitMap[l0idx*32+l1idx]))
		{
			BIT_RESET_32_VOLATILE(NativeCodePool8k_Level1BitMap[l0idx], l1idx_32);
			if(!(NativeCodePool8k_Level1BitMap[l0idx]))
			{
				BIT_RESET_16_VOLATILE(NativeCodePool8k_Level0BitMap, l0idx_16);
			}
		}
		MFENCE();
		NativeCodePool8k_lock = 0;
		return (void*)(NativeCodePool+((1ULL)<<(28ULL))*1+(l0idx*32*64+l1idx*64+l2idx)*8ULL*1024ULL);
	}
	LFENCE();
	NativeCodePool8k_lock = 0;
	return 0;
}

void __NativeCodePoolFree8k(unsigned long pooloffset)
{
	pooloffset /= (8ULL*1024ULL);
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool8k_lock, newlock);
	}while(newlock);

	unsigned long l2start = (pooloffset/64);
	unsigned long l2idx = (pooloffset%64);
	uint64_t l2idx_64 = (uint64_t)l2idx;
	pooloffset/=64;
	unsigned long l1start = (pooloffset/32);
	unsigned long l1idx = (pooloffset%32);
	uint32_t l1idx_32 = (uint32_t)l1idx;
	pooloffset/=32;
	unsigned long l0idx = pooloffset;
	uint16_t l0idx_16 = (uint16_t)l0idx;

	BIT_SET_64_VOLATILE(NativeCodePool8k_Level2BitMap[l2start],l2idx_64);
	BIT_SET_32_VOLATILE(NativeCodePool8k_Level1BitMap[l1start],l1idx_32);
	BIT_SET_16_VOLATILE(NativeCodePool8k_Level0BitMap,l0idx_16);

	SFENCE();
	NativeCodePool8k_lock = 0;
	return;
}

void* __NativeCodePoolAlloc16k(void)
{
	/*entering CS*/
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool16k_lock, newlock);
	}while(newlock);

	if(NativeCodePool16k_Level0BitMap)
	{
		uint16_t l0idx_16;
		uint16_t l1idx_16;
		uint64_t l2idx_64;
		unsigned long l0idx;
		unsigned long l1idx;
		unsigned long l2idx;
		BIT_SCAN_16_VOLATILE(l0idx_16, NativeCodePool16k_Level0BitMap);
		l0idx = ((unsigned long)l0idx_16);
		BIT_SCAN_16_VOLATILE(l1idx_16, NativeCodePool16k_Level1BitMap[l0idx]);
		l1idx = ((unsigned long)l1idx_16);
		BIT_SCAN_64_VOLATILE(l2idx_64, NativeCodePool16k_Level2BitMap[l0idx*16+l1idx]);
		BIT_RESET_64_VOLATILE(NativeCodePool16k_Level2BitMap[l0idx*16+l1idx], l2idx_64);
		l2idx = ((unsigned long)l2idx_64);
		if(!(NativeCodePool16k_Level2BitMap[l0idx*16+l1idx]))
		{
			BIT_RESET_16_VOLATILE(NativeCodePool16k_Level1BitMap[l0idx], l1idx_16);
			if(!(NativeCodePool16k_Level1BitMap[l0idx]))
			{
				BIT_RESET_16_VOLATILE(NativeCodePool16k_Level0BitMap, l0idx_16);
			}
		}
		MFENCE();
		NativeCodePool16k_lock = 0;
		return (void*)(NativeCodePool+((1ULL)<<(28ULL))*2+(l0idx*16*64+l1idx*64+l2idx)*16ULL*1024ULL);
	}
	LFENCE();
	NativeCodePool16k_lock = 0;
	return 0;
}

void __NativeCodePoolFree16k(unsigned long pooloffset)
{
	pooloffset /= (16ULL*1024ULL);
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool16k_lock, newlock);
	}while(newlock);

	unsigned long l2start = (pooloffset/64);
	unsigned long l2idx = (pooloffset%64);
	uint64_t l2idx_64 = (uint64_t)l2idx;
	pooloffset/=64;
	unsigned long l1start = (pooloffset/16);
	unsigned long l1idx = (pooloffset%16);
	uint16_t l1idx_16 = (uint16_t)l1idx;
	pooloffset/=16;
	unsigned long l0idx = pooloffset;
	uint16_t l0idx_16 = (uint16_t)l0idx;

	BIT_SET_64_VOLATILE(NativeCodePool16k_Level2BitMap[l2start],l2idx_64);
	BIT_SET_16_VOLATILE(NativeCodePool16k_Level1BitMap[l1start],l1idx_16);
	BIT_SET_16_VOLATILE(NativeCodePool16k_Level0BitMap,l0idx_16);

	SFENCE();
	NativeCodePool16k_lock = 0;
	return;
}

void* __NativeCodePoolAlloc32k(void)
{
	/*entering CS*/
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool32k_lock, newlock);
	}while(newlock);

	if(NativeCodePool32k_Level0BitMap)
	{
		uint16_t l0idx_16;
		uint16_t l1idx_16;
		uint32_t l2idx_32;
		unsigned long l0idx;
		unsigned long l1idx;
		unsigned long l2idx;
		BIT_SCAN_16_VOLATILE(l0idx_16, NativeCodePool32k_Level0BitMap);
		l0idx = ((unsigned long)l0idx_16);
		BIT_SCAN_16_VOLATILE(l1idx_16, NativeCodePool32k_Level1BitMap[l0idx]);
		l1idx = ((unsigned long)l1idx_16);
		BIT_SCAN_32_VOLATILE(l2idx_32, NativeCodePool32k_Level2BitMap[l0idx*16+l1idx]);
		BIT_RESET_32_VOLATILE(NativeCodePool32k_Level2BitMap[l0idx*16+l1idx], l2idx_32);
		l2idx = ((unsigned long)l2idx_32);
		if(!(NativeCodePool32k_Level2BitMap[l0idx*16+l1idx]))
		{
			BIT_RESET_16_VOLATILE(NativeCodePool32k_Level1BitMap[l0idx], l1idx_16);
			if(!(NativeCodePool32k_Level1BitMap[l0idx]))
			{
				BIT_RESET_16_VOLATILE(NativeCodePool32k_Level0BitMap, l0idx_16);
			}
		}
		MFENCE();
		NativeCodePool32k_lock = 0;
		return (void*)(NativeCodePool+((1ULL)<<(28ULL))*3+(l0idx*16*32+l1idx*32+l2idx)*32ULL*1024ULL);
	}
	LFENCE();
	NativeCodePool32k_lock = 0;
	return 0;
}

void __NativeCodePoolFree32k(unsigned long pooloffset)
{
	pooloffset /= (32ULL*1024ULL);
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool32k_lock, newlock);
	}while(newlock);

	unsigned long l2start = (pooloffset/32);
	unsigned long l2idx = (pooloffset%32);
	uint32_t l2idx_32 = (uint32_t)l2idx;
	pooloffset/=32;
	unsigned long l1start = (pooloffset/16);
	unsigned long l1idx = (pooloffset%16);
	uint16_t l1idx_16 = (uint16_t)l1idx;
	pooloffset/=16;
	unsigned long l0idx = pooloffset;
	uint16_t l0idx_16 = (uint16_t)l0idx;

	BIT_SET_32_VOLATILE(NativeCodePool32k_Level2BitMap[l2start],l2idx_32);
	BIT_SET_16_VOLATILE(NativeCodePool32k_Level1BitMap[l1start],l1idx_16);
	BIT_SET_16_VOLATILE(NativeCodePool32k_Level0BitMap,l0idx_16);

	SFENCE();
	NativeCodePool32k_lock = 0;
	return;
}

void* __NativeCodePoolAlloc64k(void)
{
	/*entering CS*/
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool64k_lock, newlock);
	}while(newlock);

	if(NativeCodePool64k_Level0BitMap)
	{
		uint64_t l0idx_64;
		uint64_t l1idx_64;
		unsigned long l0idx;
		unsigned long l1idx;
		BIT_SCAN_64_VOLATILE(l0idx_64, NativeCodePool64k_Level0BitMap);
		l0idx = ((unsigned long)l0idx_64);
		BIT_SCAN_64_VOLATILE(l1idx_64, NativeCodePool64k_Level1BitMap[l0idx]);
		BIT_RESET_64_VOLATILE(NativeCodePool64k_Level1BitMap[l0idx], l1idx_64);
		l1idx = ((unsigned long)l1idx_64);
		if(!(NativeCodePool64k_Level1BitMap[l0idx]))
		{
			BIT_RESET_64_VOLATILE(NativeCodePool64k_Level0BitMap, l0idx_64);
		}
		MFENCE();
		NativeCodePool64k_lock = 0;
		return (void*)(NativeCodePool+((1ULL)<<(28ULL))*4+(l0idx*64+l1idx)*64ULL*1024ULL);
	}
	LFENCE();
	NativeCodePool64k_lock = 0;
	return 0;
}

void __NativeCodePoolFree64k(unsigned long pooloffset)
{
	pooloffset /= (64ULL*1024ULL);
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool64k_lock, newlock);
	}while(newlock);

	unsigned long l1start = (pooloffset/64);
	unsigned long l1idx = (pooloffset%64);
	uint64_t l1idx_64 = (uint64_t)l1idx;
	pooloffset/=64;
	unsigned long l0idx = pooloffset;
	uint64_t l0idx_64 = (uint64_t)l0idx;

	BIT_SET_64_VOLATILE(NativeCodePool64k_Level1BitMap[l1start],l1idx_64);
	BIT_SET_64_VOLATILE(NativeCodePool64k_Level0BitMap,l0idx_64);

	SFENCE();
	NativeCodePool64k_lock = 0;
	return;
}

void* __NativeCodePoolAlloc128k(void)
{
	/*entering CS*/
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool128k_lock, newlock);
	}while(newlock);

	if(NativeCodePool128k_Level0BitMap)
	{
		uint32_t l0idx_32;
		uint64_t l1idx_64;
		unsigned long l0idx;
		unsigned long l1idx;
		BIT_SCAN_32_VOLATILE(l0idx_32, NativeCodePool128k_Level0BitMap);
		l0idx = ((unsigned long)l0idx_32);
		BIT_SCAN_64_VOLATILE(l1idx_64, NativeCodePool128k_Level1BitMap[l0idx]);
		BIT_RESET_64_VOLATILE(NativeCodePool128k_Level1BitMap[l0idx], l1idx_64);
		l1idx = ((unsigned long)l1idx_64);
		if(!(NativeCodePool128k_Level1BitMap[l0idx]))
		{
			BIT_RESET_32_VOLATILE(NativeCodePool128k_Level0BitMap, l0idx_32);
		}
		MFENCE();
		NativeCodePool128k_lock = 0;
		return (void*)(NativeCodePool+((1ULL)<<(28ULL))*5+(l0idx*64+l1idx)*128ULL*1024ULL);
	}
	LFENCE();
	NativeCodePool128k_lock = 0;
	return 0;
}

void __NativeCodePoolFree128k(unsigned long pooloffset)
{
	pooloffset /= (128ULL*1024ULL);
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool128k_lock, newlock);
	}while(newlock);

	unsigned long l1start = (pooloffset/64);
	unsigned long l1idx = (pooloffset%64);
	uint64_t l1idx_64 = (uint64_t)l1idx;
	pooloffset/=64;
	unsigned long l0idx = pooloffset;
	uint32_t l0idx_32 = (uint32_t)l0idx;

	BIT_SET_64_VOLATILE(NativeCodePool128k_Level1BitMap[l1start],l1idx_64);
	BIT_SET_32_VOLATILE(NativeCodePool128k_Level0BitMap,l0idx_32);

	SFENCE();
	NativeCodePool128k_lock = 0;
	return;
}

void* __NativeCodePoolAlloc256k(void)
{
	/*entering CS*/
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool256k_lock, newlock);
	}while(newlock);

	if(NativeCodePool256k_Level0BitMap)
	{
		uint16_t l0idx_16;
		uint64_t l1idx_64;
		unsigned long l0idx;
		unsigned long l1idx;
		BIT_SCAN_16_VOLATILE(l0idx_16, NativeCodePool256k_Level0BitMap);
		l0idx = ((unsigned long)l0idx_16);
		BIT_SCAN_64_VOLATILE(l1idx_64, NativeCodePool256k_Level1BitMap[l0idx]);
		BIT_RESET_64_VOLATILE(NativeCodePool256k_Level1BitMap[l0idx], l1idx_64);
		l1idx = ((unsigned long)l1idx_64);
		if(!(NativeCodePool256k_Level1BitMap[l0idx]))
		{
			BIT_RESET_16_VOLATILE(NativeCodePool256k_Level0BitMap, l0idx_16);
		}
		MFENCE();
		NativeCodePool256k_lock = 0;
		return (void*)(NativeCodePool+((1ULL)<<(28ULL))*6+(l0idx*64+l1idx)*256ULL*1024ULL);
	}
	LFENCE();
	NativeCodePool256k_lock = 0;
	return 0;
}

void __NativeCodePoolFree256k(unsigned long pooloffset)
{
	pooloffset /= (256ULL*1024ULL);
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool256k_lock, newlock);
	}while(newlock);

	unsigned long l1start = (pooloffset/64);
	unsigned long l1idx = (pooloffset%64);
	uint64_t l1idx_64 = (uint64_t)l1idx;
	pooloffset/=64;
	unsigned long l0idx = pooloffset;
	uint16_t l0idx_16 = (uint16_t)l0idx;

	BIT_SET_64_VOLATILE(NativeCodePool256k_Level1BitMap[l1start],l1idx_64);
	BIT_SET_16_VOLATILE(NativeCodePool256k_Level0BitMap,l0idx_16);

	SFENCE();
	NativeCodePool256k_lock = 0;
	return;
}

void* __NativeCodePoolAlloc512k(void)
{
	/*entering CS*/
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool512k_lock, newlock);
	}while(newlock);

	if(NativeCodePool512k_Level0BitMap)
	{
		uint16_t l0idx_16;
		uint32_t l1idx_32;
		unsigned long l0idx;
		unsigned long l1idx;
		BIT_SCAN_16_VOLATILE(l0idx_16, NativeCodePool512k_Level0BitMap);
		l0idx = ((unsigned long)l0idx_16);
		BIT_SCAN_32_VOLATILE(l1idx_32, NativeCodePool512k_Level1BitMap[l0idx]);
		BIT_RESET_32_VOLATILE(NativeCodePool512k_Level1BitMap[l0idx], l1idx_32);
		l1idx = ((unsigned long)l1idx_32);
		if(!(NativeCodePool512k_Level1BitMap[l0idx]))
		{
			BIT_RESET_16_VOLATILE(NativeCodePool512k_Level0BitMap, l0idx_16);
		}
		MFENCE();
		NativeCodePool512k_lock = 0;
		return (void*)(NativeCodePool+((1ULL)<<(28ULL))*7+(l0idx*32+l1idx)*512ULL*1024ULL);
	}
	LFENCE();
	NativeCodePool512k_lock = 0;
	return 0;
}

void __NativeCodePoolFree512k(unsigned long pooloffset)
{
	pooloffset /= (512ULL*1024ULL);
	register uint8_t newlock = 1;
	do{
		XCHG_LOCK(NativeCodePool512k_lock, newlock);
	}while(newlock);

	unsigned long l1start = (pooloffset/32);
	unsigned long l1idx = (pooloffset%32);
	uint32_t l1idx_32 = (uint64_t)l1idx;
	pooloffset/=32;
	unsigned long l0idx = pooloffset;
	uint16_t l0idx_16 = (uint16_t)l0idx;

	BIT_SET_32_VOLATILE(NativeCodePool512k_Level1BitMap[l1start],l1idx_32);
	BIT_SET_16_VOLATILE(NativeCodePool512k_Level0BitMap,l0idx_16);

	SFENCE();
	NativeCodePool512k_lock = 0;
	return;
}

typedef void* (*NativeCodePoolAllocFuncType)(void);
typedef void (*NativeCodePoolFreeFuncType)(unsigned long);

NativeCodePoolAllocFuncType NativeCodePoolAllocSize[NATIVE_CODE_BLOCK_SIZE_NUM]={
		__NativeCodePoolAlloc4k,
		__NativeCodePoolAlloc8k,
		__NativeCodePoolAlloc16k,
		__NativeCodePoolAlloc32k,
		__NativeCodePoolAlloc64k,
		__NativeCodePoolAlloc128k,
		__NativeCodePoolAlloc256k,
		__NativeCodePoolAlloc512k
};

NativeCodePoolFreeFuncType NativeCodePoolFreeSize[NATIVE_CODE_BLOCK_SIZE_NUM]={
		__NativeCodePoolFree4k,
		__NativeCodePoolFree8k,
		__NativeCodePoolFree16k,
		__NativeCodePoolFree32k,
		__NativeCodePoolFree64k,
		__NativeCodePoolFree128k,
		__NativeCodePoolFree256k,
		__NativeCodePoolFree512k,
};

void* NativeCodePoolAlloc(enum NATIVE_CODE_BLOCK_SIZE size)
{
	if(((unsigned int)size) < ((unsigned int)NATIVE_CODE_BLOCK_SIZE_NUM))
	{
		return (*(NativeCodePoolAllocSize[size]))();
	}
	return NULL;
}

void NativeCodePoolFree(void* poolptr)
{
	unsigned long pooloffset = (((unsigned long)poolptr)-((unsigned long)NativeCodePool));
	(*(NativeCodePoolFreeSize[pooloffset/((1ULL)<<(28ULL))]))(pooloffset%((1ULL)<<(28ULL)));
}

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

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
			newptr = (__typeof((listdesc).list))realloc(((listdesc).EffecSize)*sizeof(__typeof((listdesc).list[0])) + (1024ULL));\
		}\
		else\
		{\
			newptr = (__typeof((listdesc).list))realloc(((listdesc).EffecSize)*sizeof(__typeof((listdesc).list[0]))*2);\
		}\
		if(newptr == NULL)\
		{\
			exit(CRITICAL_MEM_LOW);\
		}\
		(listdesc).list = newptr;\
	}\
}while(0)

struct _NativeCodeRelocEntry;
typedef struct _NativeCodeRelocEntry NativeCodeRelocEntry;

struct _NativeCodeRelocList;
typedef struct _NativeCodeRelocList NativeCodeRelocList;

struct _NativeCodeBlockDesc;
typedef struct _NativeCodeBlockDesc NativeCodeBlockDesc;

struct _NCBAvlNode;
typedef struct _NCBAvlNode NCBAvlNode;

struct _NativeCodeRelocEntry {
	uint64_t spc;
	uint32_t offset;
	uint32_t flags;
};

struct _NativeCodeRelocList{
	NativeCodeRelocEntry* arr;
	uint32_t EffecSize;
	uint32_t AllocSize;
};

typedef struct _IJTabRefEntry{
	uint64_t spc;
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

struct _NativeCodeBlockDesc {
	struct rb_node;
	NativeCodeBlockDesc* PreBlock;
	NativeCodeBlockDesc* NxtBlock;
	void* NativeCode; //XXX:should be managed manually
	RefedBlockList RefedBlocks;
	uint32_t NativeCodeSize;
	uint32_t SourceCodeSize;
	uint64_t SourceCodeBase;
	NativeCodeRelocList Refs;
	NativeCodeRelocList SpcMap;
};

void AddIndirJmpTabEntry(uint64_t spc, uint64_t tpc, NativeCodeBlockDesc* desc)
{
	STPC_HASH_TABLE_ENTRY* entry = (IndirJmpHashTab + SpcHashFunc(spc));
	if((entry->spc) != spc)
	{
		if(entry->spc)
		{

		}
		else
		{
			(entry->tpc) = tpc;
			(entry->spc) = spc;
		}
	}

inline uint32_t FindFirstEntryBySpc(NativeCodeRelocList list,uint64_t spc)
{
	uint32_t size = (list.EffecSize);
	NativeCodeRelocEntry* head = (list.arr);
	uint32_t i = 0;
	if(size)
	{
		while(size)
		{
			uint64_t value = head[ i + (size/2)].spc;
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
	}
	else
	{
		return 0;
	}
}

#define RELOC_ENTRY_VALID	0x01\



void RebaseNativeCodeBlock(NativeCodeBlockDesc* desc, uint64_t newbase)
{
	uint32_t i;
	uint64_t offset64 = (newbase - ((uint64_t)(desc->NativeCode)));
	for(i=0;i<(desc->RefedBlockEffecSize);i++)
	{
		NativeCodeBlockDesc* refer = (desc->RefedBlocks)[i];
		NativeCodeRelocList reloclist = refer->Refs;
		uint32_t j = FindFirstEntryBySpc(reloclist, (desc->SourceCodeBase));
		while(j<(reloclist.EffecSize))
		{
			if((reloclist.arr)[j].flags & RELOC_ENTRY_VALID)
			{
				(*((uint64_t*)(((uint64_t)(refer->NativeCode))+((reloclist.arr)[j]).offset + 2))) += offset64;
			}
			j++;
		}
	}
}

struct _NCBAvlNode {
	NativeCodeBlockDesc* NCBPtr;
	NCBAvlNode* parent;
	NCBAvlNode* left;
	NCBAvlNode* right;
	uint32_t height;
	uint32_t flags;
} ;

static NCBAvlNode NCBAvlTerminator = { NULL, NULL, NULL, NULL, 0, 0};

inline void __SwapMkr(NativeCodeRelocEntry* x, NativeCodeRelocEntry* y) {
	if (x != y) {
		NativeCodeRelocEntry tmp;
		tmp = *x;
		*x = *y;
		*y = tmp;
	}
}

#define __QuickSort(TYPE,PTR,N) \
__QuickSort_Type_##TYPE(PTR,N)

#define __QuickSort_Prototype(TYPE) \
void __QuickSort_Type_##TYPE( TYPE * , int32_t )

#define __QuickSort_Define(TYPE,KEYTYPE,KEYNAME,SWAPFUNC) \
void __QuickSort_Type_##TYPE( TYPE *list , int32_t n ) {\
	if (n > 1) { \
		int32_t i; \
		int32_t pos = rand(); \
		KEYTYPE pvt = list[pos %= n].KEYNAME; \
		int32_t le = 0; \
		int32_t l = -1; \
		int32_t ge = n; \
		int32_t  g = n; \
		while (1) { \
			while (((++l) < g) && (list[l].KEYNAME <= pvt)) { \
					if (list[l].KEYNAME == pvt) { \
						SWAPFUNC(list + (le++), list + l); \
					} \
				} \
				while ((l < (--g)) && list[g].KEYNAME >= pvt) { \
					if (list[g].KEYNAME == pvt) { \
						SWAPFUNC(list + (--ge), list + g); \
					} \
				} \
				if (g <= l) { \
					break; \
				} \
				SWAPFUNC(list + l, list + g); \
			} \
		if (2 * le > l) {\
			for (i = le; i < l; i++) {\
				SWAPFUNC(list + i, list + l - 1 - i); }}\
		else {\
			for (i = 0; i < le; i++) {\
				SWAPFUNC(list + i, list + l - 1 - i); }}\
		if (2 * ge < (n + l)) {\
			for (i = l; i < ge;  i++) {\
				SWAPFUNC(list + i, list + n + l - 1 - i); }}\
		else {\
			for (i = ge; i < n; i++) {\
				SWAPFUNC(list + i, list + n + l - 1 - i); }}\
		__QuickSort(TYPE,list, l - le); \
		__QuickSort(TYPE,list - ge + l, ge - l); \
	}\
}

__QuickSort_Prototype(NativeCodeRelocEntry);

__QuickSort_Define(NativeCodeRelocEntry,uint64_t,spc,__SwapMkr);



void SortMarkerList(NativeCodeRelocList* _list) {
	__QuickSort(NativeCodeRelocEntry,_list->arr, _list->EffecSize);
}

void InsertMarkList(NativeCodeRelocList* list, uint64_t _spc, int32_t _offset, uint32_t _flags)
{
	if((list->AllocSize)==(list->EffecSize))
	{
		list->arr = realloc(list->arr , list->AllocSize * 2 * sizeof(NativeCodeRelocEntry));
	}
	list->arr[list->EffecSize].spc = _spc;
	list->arr[list->EffecSize].offset = _offset;
	list->arr[list->EffecSize].flags = _flags;
	list->EffecSize++;
}

void InsertNCBAvl(NativeCodeBlockDesc* newblock)
{
	NCBAvlNode* newnode = (NCBAvlNode*)malloc(sizeof(NCBAvlNode));
	newnode->NCBPtr = newblock;
	newblock->Ptr2AvlNode = newnode;
	//__AvlInsert;
}

#define NCBAVLNODE_RIGHT_PTR_OFFSET ((uint32_t)((int64_t)(&(((NCBAvlNode*)(NULL))->right))))

#define NCBAVLNODE_LEFT_PTR_OFFSET ((uint32_t)((int64_t)(&(((NCBAvlNode*)(NULL))->left))))

NCBAvlNode* InsertNCBNode(NCBAvlNode** const _root,
		NativeCodeBlockDesc* _newblock) {
	NCBAvlNode* solvednode = (NCBAvlNode*) malloc(sizeof(NCBAvlNode));
	NCBAvlNode* const newnode = solvednode;
	solvednode->NCBPtr = _newblock;
	solvednode->height = 1;
	solvednode->left = (*_root)->right = (&NCBAvlTerminator);
	if ((*_root) == (&NCBAvlTerminator)) {
		(*_root) = solvednode;
		solvednode->parent = NULL;
	} else {
		int64_t sp = -2;
		NCBAvlNode* origroot = (*_root);
		NCBAvlNode* pendingnode = origroot;
		uint32_t ptroffset[0x40];
		while (1) {
			sp += 2;
			if ((pendingnode->NCBPtr->SourceCodeBase)
					< (_newblock->SourceCodeBase)) {
				ptroffset[sp] = NCBAVLNODE_RIGHT_PTR_OFFSET;
				ptroffset[sp + 1] = NCBAVLNODE_LEFT_PTR_OFFSET;
				if (pendingnode->right == (&NCBAvlTerminator)) {
					pendingnode->right = solvednode;
					solvednode->parent = pendingnode;
					break;
				} else {
					pendingnode = pendingnode->right;
				}
			} else {
				ptroffset[sp] = NCBAVLNODE_LEFT_PTR_OFFSET;
				ptroffset[sp + 1] = NCBAVLNODE_RIGHT_PTR_OFFSET;
				if (pendingnode->left == (&NCBAvlTerminator)) {
					pendingnode->left = solvednode;
					solvednode->parent = pendingnode;
					break;
				} else {
					pendingnode = pendingnode->left;
				}
			}
		}
		while (sp >= 0) {
			NCBAvlNode* L1d = (*(NCBAvlNode**) (((uint64_t) pendingnode)
					+ ptroffset[sp + 1]));
			if (solvednode->height > (L1d->height + 1)) {
				NCBAvlNode* L2d = (*(NCBAvlNode**) (((uint64_t) solvednode)
						+ ptroffset[sp + 1]));
				NCBAvlNode* L2s = (*(NCBAvlNode**) (((uint64_t) solvednode)
						+ ptroffset[sp]));
				if (L2d->height > L2s->height) {
					L2d->height++;
					solvednode->height--;
					pendingnode->height--;
					NCBAvlNode** L3dp = (NCBAvlNode**) (((uint64_t) L2d)
							+ ptroffset[sp + 1]);
					NCBAvlNode** L3sp = (NCBAvlNode**) (((uint64_t) L2d)
							+ ptroffset[sp]);
					(*((NCBAvlNode**) (((uint64_t) pendingnode) + ptroffset[sp]))) =
							(*L3dp);
					(*L3dp) = pendingnode;
					L2d->parent = pendingnode->parent;
					pendingnode->parent = L2d;
					(*((NCBAvlNode**) (((uint64_t) solvednode) + ptroffset[sp + 1]))) =
							(*L3sp);
					(*L3sp) = solvednode;
					solvednode->parent = L2d;
					solvednode = L2d;
				} else {
					pendingnode->height--;
					solvednode->parent = pendingnode->parent;
					pendingnode->parent = solvednode;
					(*((NCBAvlNode**) (((uint64_t) pendingnode) + ptroffset[sp]))) =
							L2d;
					(*((NCBAvlNode**) (((uint64_t) solvednode) + ptroffset[sp + 1]))) =
							pendingnode;
				}
				pendingnode = solvednode->parent;
				if (pendingnode == NULL ) {
					(*_root) = solvednode;
				}
				break;
			} else {
				if (solvednode->height == pendingnode->height) {
					pendingnode->height++;
					sp -= 2;
					solvednode = pendingnode;
					pendingnode = pendingnode->parent;
				} else {
					break;
				}
			}
		}
	}
	return newnode;
}

static inline uint64_t __NCBAvl_DEL_Balance_L(NCBAvlNode** pendingnodep,
		NCBAvlNode** solvednodep, NCBAvlNode** oldrootp) {
	(*oldrootp) = (*pendingnodep);
	NCBAvlNode* oldroot = (*oldrootp);
	NCBAvlNode* pendingnode = oldroot->parent;
	(*pendingnodep) = pendingnode;
	NCBAvlNode* solvednode;
	if (oldroot->right->left->height > oldroot->right->right->height) {
		NCBAvlNode* middlenode = oldroot->right;
		solvednode = middlenode->left;

		middlenode->height--;
		solvednode->height++;

		oldroot->right = solvednode->left;
		solvednode->left->parent = oldroot;

		middlenode->left = solvednode->right;
		solvednode->right->parent = middlenode;

		solvednode->left = oldroot;
		solvednode->right = middlenode;
		solvednode->parent = pendingnode;

		middlenode->parent = solvednode;
		oldroot->parent = solvednode;

	} else {
		solvednode = oldroot->right;

		oldroot->right = solvednode->left;
		solvednode->left->parent = oldroot;

		solvednode->left = oldroot;
		solvednode->parent = pendingnode;
		oldroot->parent = solvednode;

		if (oldroot->right->height == solvednode->right->height) {
			oldroot->height--;
			solvednode->height++;
			(*solvednodep) = solvednode;
			return 0;
		}
	}
	pendingnode->height -= 2;
	(*solvednodep) = solvednode;
	return 1;
}


static inline uint64_t __NCBAvl_DEL_Balance_R(NCBAvlNode** pendingnodep,
		NCBAvlNode** solvednodep, NCBAvlNode** oldrootp) {
	(*oldrootp) = (*pendingnodep);
	NCBAvlNode* oldroot = (*oldrootp);
	NCBAvlNode* pendingnode = oldroot->parent;
	(*pendingnodep) = pendingnode;
	NCBAvlNode* solvednode;
	if (oldroot->left->right->height > oldroot->left->left->height) {
		NCBAvlNode* middlenode = oldroot->left;
		solvednode = middlenode->right;

		middlenode->height--;
		solvednode->height++;

		oldroot->left = solvednode->right;
		solvednode->right->parent = oldroot;

		middlenode->right = solvednode->left;
		solvednode->left->parent = middlenode;

		solvednode->right = oldroot;
		solvednode->left = middlenode;
		solvednode->parent = pendingnode;

		middlenode->parent = solvednode;
		oldroot->parent = solvednode;

	} else {
		solvednode = oldroot->left;

		oldroot->left = solvednode->right;
		solvednode->right->parent = oldroot;

		solvednode->right = oldroot;
		solvednode->parent = pendingnode;
		oldroot->parent = solvednode;

		if (oldroot->left->height == solvednode->left->height) {
			oldroot->height--;
			solvednode->height++;
			(*solvednodep) = solvednode;
			return 0;
		}
	}
	oldroot->height -= 2;
	(*solvednodep) = solvednode;
	return 1;
}

void DeleteNCBAvlNode(NCBAvlNode** const _root,
		NativeCodeBlockDesc* _targetblock) {
	NCBAvlNode* const findnode = _targetblock->Ptr2AvlNode;
	NCBAvlNode* pendingnode;
	NCBAvlNode* oldroot;
	NCBAvlNode* solvednode;
	NCBAvlNode* psnode;
	uint64_t shouldcontinue = 1;
	uint64_t rotated = 1;
	if (findnode->height > 1) {
		if (findnode->right->height > findnode->left->height) {
			psnode = findnode->right;
			if (psnode->left->height) {
				do {
					pendingnode = psnode;
					psnode = psnode->left;
				} while (psnode->left->height);
				solvednode = psnode->right;
				while (pendingnode != findnode) {
					if (rotated) {
						pendingnode->left = solvednode;
						solvednode->parent = pendingnode;
					}
					if (!shouldcontinue) {
						solvednode = findnode->right;
						break;
					}
					if (solvednode->height < (pendingnode->right->height - 1)) {
						rotated = 1;
						shouldcontinue = __NCBAvl_DEL_Balance_L(&pendingnode,
								&solvednode, &oldroot);
					} else {
						rotated = 0;
						if ((solvednode->height == pendingnode->right->height)
								&& (solvednode->height
										< (pendingnode->height - 1))) {
							pendingnode->height--;
							solvednode = pendingnode;
							pendingnode = pendingnode->parent;
						} else {
							shouldcontinue = 0;
						}
					}
				}
				psnode->height = solvednode->height + 1;
				psnode->right = solvednode;
				solvednode->parent = psnode;
			}
			psnode->left = findnode->left;

		} else {
			psnode = findnode->left;
			if (psnode->right->height) {
				do {
					pendingnode = psnode;
					psnode = psnode->right;
				} while (psnode->right->height);
				solvednode = psnode->left;
				while (pendingnode != findnode) {
					if (rotated) {
						pendingnode->right = solvednode;
						solvednode->parent = pendingnode;
					}
					if (!shouldcontinue) {
						solvednode = findnode->left;
						break;
					}
					if (solvednode->height < (pendingnode->left->height - 1)) {
						rotated = 1;
						shouldcontinue = __NCBAvl_DEL_Balance_R(&pendingnode,
								&solvednode, &oldroot);
					} else {
						rotated = 0;
						if ((solvednode->height == pendingnode->left->height)
								&& (solvednode->height
										< (pendingnode->height - 1))) {
							pendingnode->height--;
							solvednode = pendingnode;
							pendingnode = pendingnode->parent;
						} else {
							shouldcontinue = 0;
						}
					}
				}
				if (solvednode->height < findnode->height-1) {
					psnode->height = findnode->height;
					shouldcontinue = 0;
				} else {
					psnode->height = solvednode->height + 1;
				}
				psnode->left = solvednode;
				solvednode->parent = psnode;
			}
			psnode->right = findnode->right;
		}
		psnode->parent = findnode->parent;
		solvednode = psnode;
	} else {
		solvednode = (&NCBAvlTerminator);
	}
	pendingnode = findnode->parent;
	oldroot = findnode;
	free(findnode);
	while (1) {
		if (pendingnode == NULL ) {
			(*_root) = solvednode;
			break;
		} else {
			if (pendingnode->left == oldroot) {
				pendingnode->left = solvednode;
				if (shouldcontinue) {
					if (solvednode->height < (pendingnode->right->height - 1)) {
						shouldcontinue = __NCBAvl_DEL_Balance_L(&pendingnode,
								&solvednode, &oldroot);
					} else {
						if ((solvednode->height == pendingnode->right->height)
								&& (solvednode->height
										< (pendingnode->height - 1))) {
							pendingnode->height--;
							oldroot = solvednode = pendingnode;
						} else {
							return;
						}
					}
				} else {
					return;
				}
			} else {
				pendingnode->right = solvednode;
				if (shouldcontinue) {
					if (solvednode->height < (pendingnode->left->height - 1)) {
						shouldcontinue = __NCBAvl_DEL_Balance_R(&pendingnode,
								&solvednode, &oldroot);
					} else {
						if ((solvednode->height == pendingnode->left->height)
								&& (solvednode->height
										< (pendingnode->height - 1))) {
							pendingnode->height--;
							oldroot = solvednode = pendingnode;
						} else {
							return;
						}
					}
				} else {
					return;
				}
			}
		}
	}
}


