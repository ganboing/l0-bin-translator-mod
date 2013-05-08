#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

__QuickSort_Prototype(NativeCodeModEntry);

__QuickSort_Define(NativeCodeModEntry,uint64_t,spc,__SwapMkr);



void SortMarkerList(NativeCodeModEntryList* _list) {
	__QuickSort(NativeCodeModEntry,_list->Head, _list->EffecSize);
}

void InsertMarkList(NativeCodeModEntryList* list, uint64_t _spc, int32_t _offset, uint32_t _flags)
{
	if((list->AllocSize)==(list->EffecSize))
	{
		list->Head = realloc(list->Head , list->AllocSize * 2 * sizeof(NativeCodeModEntry));
	}
	list->Head[list->EffecSize].spc = _spc;
	list->Head[list->EffecSize].offset = _offset;
	list->Head[list->EffecSize].flags = _flags;
	list->EffecSize++;
}

void InsertNCBAvl(NativeCodeBlockDesc* newblock)
{
	NCBAvlNode* newnode = (NCBAvlNode*)malloc(sizeof(NCBAvlNode));
	newnode->NCBPtr = newblock;
	newblock->Prt2AvlNode = newnode;
	//__AvlInsert;
}

void _delete_tree(Tree_Node* _root) {
		if (_root != terminator) {
			_delete_tree(_root->left);
			_delete_tree(_root->right);
			delete _root;
		}
	}

#define Tree_Node int

#define __AvlTreeFindExact(NODETYPE,ROOT,KEY) \
__AvlTreeFindExact_Type_##NODETYPE( ROOT, KEY)

#define __AvlTreeFindExact_Prototype(NODETYPE,KEYTYPE) \
NODETYPE** __AvlTreeFindExact_Type_##NODETYPE( NODETYPE**, KEYTYPE);

#define __AvlTreeFindExact_Define(NODETYPE,KEYTYPE,DATAPTR,KEYNAME) \
NODETYPE** __AvlTreeFindExact_Type_##NODETYPE( NODETYPE** _root, KEYTYPE _key) { \
	while((*_root)!=NULL) { \
		if( ((*_root) -> DATAPTR -> KEYNAME) == _key) { \
			return _root; \
		} \
		else { \
			if( ((*_root) -> DATA -> KEYNAME) < key) { \
				_root = (&( (*root) -> right )); \
			} \
			else { \
				_root = (&( (*root) -> left )); \
			} \
	} \
	return _root; \
}



#define __AvlTreeInsert_Define(NODETYPE,DATATYPE,DATAPTR,KEYNAME) \
NODETYPE* __AvlTreeInsert_Type_##NODETYPE( NODETYPE** _root, DATATYPE* _value) { \
	NODETYPE** insertentry = __AvlTreeFindExact(NODETYPE, _root, _value -> KEYNAME); \
	if ((*insertentry) == NULL) { \
		(*insertentry) = (NODETYPE*)malloc(sizeof(NODETYPE)); \
		(*insertentry) -> left = NULL; \
		(*insertentry) -> right = NULL; \
		(*insertentry) -> parent = NULL; \
		(*insertentry) -> DATAPTR = _value; \
		return (*_root); \
	}

__AvlTreeInsert_Define(_NCBAvlNode,_NativeCodeBlockDesc,NCBPtr,SourceCodeBase);


