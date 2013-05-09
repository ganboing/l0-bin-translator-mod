#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _NativeCodeModEntry NativeCodeModEntry;

typedef struct _NativeCodeModEntryList NativeCodeModEntryList;

typedef struct _NativeCodeBlockDesc NativeCodeBlockDesc;

typedef struct _NCBAvlNode NCBAvlNode;

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


typedef struct _NativeCodeBlockDesc {
	void* NativeCodeBlock; //XXX:should be managed manually
	void* NativeCodeEntryPoint;
	NativeCodeBlockDesc** RefedBlocks;
	NCBAvlNode* Prt2AvlNode;
	uint32_t NativeCodeSize;
	uint32_t SourceCodeSize;
	uint64_t SourceCodeBase;
	NativeCodeModEntryList Refs;
	NativeCodeModEntryList SpcMap;
	uint32_t RefedBlockEffecSize;
	uint32_t RefedBlockAllocSize;
} NativeCodeBlockDesc;

typedef struct _NCBAvlNode {
	NativeCodeBlockDesc* NCBPtr;
	NCBAvlNode* parent;
	NCBAvlNode* left;
	NCBAvlNode* right;
	uint32_t height;
	uint32_t flags;
} NCBAvlNode;

static const NCBAvlNode NCBAvlTerminator = { NULL, NULL, NULL, NULL, 0, 0};

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

#define NCBAVLNODE_RIGHT_PTR_OFFSET (((uint32_t)(&(((NCBAvlNode*)(NULL))->right))))

#define NCBAVLNODE_LEFT_PTR_OFFSET (((uint32_t)(&(((NCBAvlNode*)(NULL))->left))))

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
		int32_t sp = -2;
		NCBAvlNode* origroot = (*_root);
		NCBAvlNode* pendingnode = origroot;
		uint32_t ptroffset[0x40];
		while (1) {
			sp += 2;
			if ((pendingnode->NCBPtr->SourceCodeBase)
					< (_newblock->SourceCodeBase)) {
				ptroffset[sp] = NCBAVLNODE_RIGHT_PTR_OFFSET;
				ptroffset[sp + 1] = NCBAVLNODE_LEFT_PTR_OFFSET;
				if (pendingnode->right != (&NCBAvlTerminator)) {
					pendingnode->right = solvednode;
					solvednode->parent = pendingnode;
					break;
				} else {
					pendingnode = pendingnode->right;
				}
			} else {
				ptroffset[sp] = NCBAVLNODE_LEFT_PTR_OFFSET;
				ptroffset[sp + 1] = NCBAVLNODE_RIGHT_PTR_OFFSET;
				if (pendingnode->left != (&NCBAvlTerminator)) {
					pendingnode->left = solvednode;
					solvednode->parent = pendingnode;
					break;
				} else {
					pendingnode = pendingnode->left;
				}
			}
		}
		while (sp >= 0) {
			NCBAvlNode* L1d = (*(NCBAvlNode**) (((void*) pendingnode)
					+ ptroffset[sp + 1]));
			if (solvednode->height > (L1d->height + 1)) {
				NCBAvlNode* L2d = (*(NCBAvlNode**) (((void*) solvednode)
						+ ptroffset[sp + 1]));
				NCBAvlNode* L2s = (*(NCBAvlNode**) (((void*) solvednode)
						+ ptroffset[sp]));
				if (L2d->height > L2s->height) {
					L2d->height++;
					solvednode->height--;
					pendingnode->height--;
					NCBAvlNode** L3dp = (NCBAvlNode**) (((void*) L2d)
							+ ptroffset[sp + 1]);
					NCBAvlNode** L3sp = (NCBAvlNode**) (((void*) L2d)
							+ ptroffset[sp]);
					(*((NCBAvlNode**) (((void*) pendingnode) + ptroffset[sp]))) =
							(*L3dp);
					(*L3dp) = pendingnode;
					L2d->parent = pendingnode->parent;
					pendingnode->parent = L2d;
					(*((NCBAvlNode**) (((void*) solvednode) + ptroffset[sp + 1]))) =
							(*L3sp);
					(*L3sp) = solvednode;
					solvednode->parent = L2d;
					solvednode = L2d;
				} else {
					pendingnode->height--;
					solvednode->parent = pendingnode->parent;
					pendingnode->parent = solvednode;
					(*((NCBAvlNode**) (((void*) pendingnode) + ptroffset[sp]))) =
							L2d;
					(*((NCBAvlNode**) (((void*) solvednode) + ptroffset[sp + 1]))) =
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


