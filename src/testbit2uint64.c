
inline unsigned int LoadOpL32(unsigned int* ptr)
{
	unsigned int result;
	__asm__ (
			"movl 0(%1), %0\n\t"
			"bswapl %0\n\t"
			: "=r" (result)
			: "r" (ptr)
	);
	return result;
}

extern int procinst(instr_t* ptr);

inline unsigned int DecI0OprD();




inline unsigned int asm_bswap(unsigned int x)
{
	__asm__ (
			"bswapl %0\n\t"
			: "+r"(x)
			);
	return x;
}

inline unsigned int asm_shldl(unsigned int count, unsigned int src,unsigned int dest)
{
	__asm__ (
			"movl %1, %%ecx\n\t"
			"shldl %%cl, %2, %0\n\t"
			: "+r" (dest)
			: "r" (count), "r" (src)
			: "%ecx", "%rcx"
	);
	return dest;
}

inline unsigned int Bit2Uint32(char* codeptr, unsigned int bitstart, unsigned int bitlen)
{
	//__asm__("int $3\n\t");
	unsigned int result;
	unsigned int resulthi;
	int over32;
	codeptr += bitstart/8;
	bitstart %= 8;
	bitstart += (((unsigned long int)codeptr)%sizeof(int))*8;
	codeptr = (char*)((unsigned long int)(codeptr)&(-(sizeof(int))));
	result = asm_bswap(*((unsigned int*)codeptr));
	over32 = bitstart + bitlen ;
	over32 -= 8*sizeof(int);
	if(over32>0)
	{
		resulthi=asm_bswap(*((unsigned int*)(codeptr+sizeof(int))));
		result = asm_shldl(bitstart, resulthi, result);
	}
	else
	{
		result<<=bitstart;
	}
	result>>=(bitstart-over32);
	return result;
}

unsigned long int bit2uint64C(unsigned long int* _codeptr, unsigned long int bitstart, unsigned int bitlen)
{
	unsigned long int* codeptr =(unsigned long int*)(( (unsigned long int)_codeptr)+(bitstart)/8);
	//printf("codeptr == %lx\nbitstart == %lx\nbitlen == %u\n",(unsigned long int)_codeptr,bitstart, bitlen);
	bitstart %= 8;
    unsigned long int tmplo = *codeptr;
    unsigned long int a=1;
	printf("codeptr == %lx\nbitstart == %lx\nbitlen == %u\n",(unsigned long int)codeptr,bitstart, bitlen);
	printf("tmplo == %lx\n",tmplo);
    tmplo >>= bitstart;
    if((bitstart+bitlen)>64)
    {
        unsigned long int tmphi = *(codeptr+1);
        tmplo |= (tmphi<<(64-bitstart));
    }
    if(bitlen>=64)
    {
    	return tmplo;
    }
    else
    {
    	return (tmplo & ((a << bitlen)-1) );
    }
}

unsigned int _bit2uint(char *c, unsigned int bit_start, unsigned int bit_len)
{
    unsigned int result = 0;
    unsigned int tmp = 0;

    unsigned int tc1 = 0;
    unsigned int tc2 = 0;

    // go the nearest char
    c += (bit_start / 8);
    bit_start = bit_start % 8;

    // get out char
    tc1 = (unsigned int)(*c);
    tc1 = tc1 & 0xFF;

    if ((bit_start + bit_len) > 7) {
        tc2 = (unsigned int)(*(c+1));
        tc2 = tc2 & 0xFF;
        result = (tc1 << 8) + tc2;
        // aligned to int
        tmp = (16 - bit_len - bit_start);
        result = result >> tmp;
    } else {
        result = tc1;

        // aligned to int
        tmp = (8 - bit_len - bit_start);
        result = result >> tmp;
    }

    tmp = (0x1 << bit_len) - 0x1;
    result = result & tmp;

    return result;
}

int main()
{
	struct timeval starttime;
	struct timeval endtime;
	long int timeC=0;
	long int timeA=0;
	//init test zone
	void* test_area = malloc((sizeof(long int))*1024*1024+1);
	unsigned int round;
	unsigned int dataptr;
	unsigned int testno;
	//char* inputchar = malloc(100);
	for(dataptr=0;dataptr<=(1024*1024*2);dataptr++)
	{
		*(((int*)test_area)+dataptr) = rand();
	}
	for(round=0;round<0x10;round++)
	{
		//init test data

		unsigned long int result1=0;
		unsigned long int result2=0;

		unsigned int offset = rand()%(1024*1024*8);

		unsigned long int* codeptr = (unsigned long int*)(((unsigned long int)test_area)+offset);
		unsigned long int bitstart = rand()%(1024*1024*8*8-offset*8);
		unsigned int bitlen = rand()%7+1;

		gettimeofday(&starttime,NULL);
			//unsigned long int result_c = bit2uint64C(codeptr, bitstart, bitlen);
		for(testno=0;testno<0x10000000;testno++)
		{
			result1 += Bit2Uint32(codeptr, bitstart, bitlen);
		}
			//scanf("%s",inputchar);
			/*if(result_c!=result_asm)
			{
				printf("resultc gives %lx\n", result_c);
				printf("resultasm gives %lx\n", result_asm);
			}*/
		gettimeofday(&endtime,NULL);
		timeA -= (((long int)starttime.tv_sec)*1000000+starttime.tv_usec);
		timeA += (((long int)endtime.tv_sec)*1000000+endtime.tv_usec);

		//printf("Asm code using %ld us\n",timeA);

		gettimeofday(&starttime,NULL);
		for(testno=0;testno<0x10000000;testno++)
		{
			result2 += _bit2uint((char*)codeptr, bitstart, bitlen);
		}

			//unsigned long int result_c = bit2uint64C(codeptr, bitstart, bitlen);
			//unsigned long int result_asm = Bit2Uint64(codeptr, bitstart, bitlen);
			//scanf("%s",inputchar);
			/*if(result_c!=result_asm)
			{
				printf("resultc gives %lx\n", result_c);
				printf("resultasm gives %lx\n", result_asm);
			}*/
		gettimeofday(&endtime,NULL);
		timeC -= (((long int)starttime.tv_sec)*1000000+starttime.tv_usec);
		timeC += (((long int)endtime.tv_sec)*1000000+endtime.tv_usec);

		//printf("C code using %ld us\n",timeC);

		if(result1!=result2)
		{
			printf("\n----------\n");
			printf("----------%lx",bit2uint64C(codeptr,bitstart, bitlen));

			printf("error asm and c give diff result\n");
			printf("resultc gives %lx\n", result2);
			printf("resultasm gives %lx\n", result1);

			printf("----------\n");
		}

	}
	printf("Asm code using %ld us\n",timeA);
	printf("C code using %ld us\n",timeC);
	return 0;
}

