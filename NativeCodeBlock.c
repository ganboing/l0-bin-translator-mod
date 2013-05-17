#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ASM_MACROS.h"

struct _NativeCodeModEntry;
typedef struct _NativeCodeModEntry NativeCodeModEntry;

struct _NativeCodeModEntryList;
typedef struct _NativeCodeModEntryList NativeCodeModEntryList;

struct _NativeCodeBlockDesc;
typedef struct _NativeCodeBlockDesc NativeCodeBlockDesc;

struct _NCBAvlNode;
typedef struct _NCBAvlNode NCBAvlNode;

struct _NativeCodeModEntry {
	uint64_t spc;
	int32_t offset;
	uint32_t flags;
};

struct _NativeCodeModEntryList{
	NativeCodeModEntry* Head;
	uint32_t EffecSize;
	uint32_t AllocSize;
};


struct _NativeCodeBlockDesc {
	void* NativeCodeBlock; //XXX:should be managed manually
	void* NativeCodeEntryPoint;
	NativeCodeBlockDesc** RefedBlocks;
	NCBAvlNode* Ptr2AvlNode;
	uint32_t NativeCodeSize;
	uint32_t SourceCodeSize;
	uint64_t SourceCodeBase;
	NativeCodeModEntryList Refs;
	NativeCodeModEntryList SpcMap;
	uint32_t RefedBlockEffecSize;
	uint32_t RefedBlockAllocSize;
};

struct _NCBAvlNode {
	NativeCodeBlockDesc* NCBPtr;
	NCBAvlNode* parent;
	NCBAvlNode* left;
	NCBAvlNode* right;
	uint32_t height;
	uint32_t flags;
} ;

static NCBAvlNode NCBAvlTerminator = { NULL, NULL, NULL, NULL, 0, 0};

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
				if (solvednode->height < findnode->right->height) {
					psnode->height = findnode->right->height + 1;
				} else {
					psnode->height = solvednode->height + 1;
				}
				psnode->left = solvednode;
			}
			psnode->right = findnode->right;
		}
		psnode->parent = findnode->parent;
		solvednode->parent = psnode;
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


