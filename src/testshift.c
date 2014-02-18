#define GET_INST_FIELD_ZO(dest,src,n) \
	__asm__ (\
		"shldl $"#n", %1, %0\n\t"\
		"andl $(((1<<32)-1)>>"#n"), %1\n\t"\
		:"=g" (dest), "+r"(src)\
	)

unsigned int test(unsigned int a,unsigned int b)
{
	GET_INST_FIELD_ZO(a,b,10);
	return a;
}
