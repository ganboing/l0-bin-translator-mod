#define __QuickSort_Prototype(TYPE,PTR,N) \
	__QuickSort_Type_##TYPE##(PTR,N)

__QuickSort_Prototype(int,a,n);
