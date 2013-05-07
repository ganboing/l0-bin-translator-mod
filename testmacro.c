#define __QuickSort_Define(TYPE,KEYTYPE,KEYNAME,SWAPFUNC) \
void __QuicKSort_Type_##TYPE## ( TYPE##* list , uint32_t n ) {\
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
			__QuicKSort_Type_##TYPE##(list, l - le); \
			__QuicKSort_Type_##TYPE##(list - ge + l, ge - l); \
		} \
	}\
}

__QuickSort_Define(int,int,spc,swap)
