#include <stdint.h>

typedef struct _NativeCodeModEntry {
	uint64_t spc;
	int32_t offset;
	uint32_t flags;
}NativeCodeModEntry;

typedef struct _NativeCodeModEntryList{
	NativeCodeModEntry* Head;
	uint32_t EffecSize;
	uint32_t AllocSize;
}NativeCodeModEntryList;

typedef struct _NCBAvlNode {
	struct _NativeCodeBlockDesc* NCBPtr;
	struct _NCBAvlNode* parent;
	struct _NCBAvlNode* left;
	struct _NCBAvlNode* right;
} NCBAvlNode;

typedef struct _NativeCodeBlockDesc {
	void* NativeCodeBlock; //XXX:should be managed manually
	void* NativeCodeEntryPoint;
	struct NativeCodeBlockDesc** RefedBlocks;
	NCBAvlNode* Prt2AvlNode;
	uint32_t NativeCodeSize;
	uint32_t SourceCodeSize;
	uint64_t SourceCodeBase;
	NativeCodeModEntryList Refs;
	NativeCodeModEntryList SpcMap;
	uint32_t RefedBlockEffecSize;
	uint32_t RefedBlockAllocSize;
} NativeCodeBlockDesc;

inline void __SwapMkr(NativeCodeModEntry* x, NativeCodeModEntry* y) {
	if (x != y) {
		NativeCodeModEntry tmp;
		tmp = *x;
		*x = *y;
		*y = tmp;
	}
}

#define __QuickSort_Prototype(TYPE,PTR,N) \
__QuickSort_Type_##TYPE (PTR,N)

#define __QuickSort_Define(TYPE,KEYTYPE,KEYNAME,SWAPFUNC) \
void __QuicKSort_Type_##TYPE ( TYPE *list , uint32_t n ) {\
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
		__QuicKSort_Type_##TYPE (list, l - le); \
		__QuicKSort_Type_##TYPE (list - ge + l, ge - l); \
	}\
}

__QuickSort_Define(NativeCodeModEntry,uint64_t,spc,__SwapMkr)



void SortMarkerList(NativeCodeModEntryList* _list) {
	__SortList(_list->Head, _list->EffecSize);
}

void InsertMarkList(NativeCodeModEntryList* list, uint64_t _spc, int32_t _offset, uint32_t _flags)
{
	if((list->AllocSize)==(list->EffecSize))
	{
		list->Head = realloc(list->Head , AllocSize * 2 * sizeof(NativeCodeModEntry));
	}
	list->Head[EffecSize].spc = _spc;
	list->Head[EffecSize].offset = _offset;
	list->Head[EffecSize].flags = _flags;
	list->EffecSize++;
}

void InsertNCBAvl(NativeCodeBlockDesc* newblock)
{
	NCBAvlNode* newnode = (NCBAvlNode*)malloc(sizeof(NCBAvlNode));
	newnode->NCBPtr = newblock;
	newblock->Prt2AvlNode = newnode;
	//__AvlInsert;
}
