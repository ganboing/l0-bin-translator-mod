#include <stdint.h>

struct NativeCodeBlock
{
	void* NativeCodePtr;
	uint32_t NativeCodeSize;
	uint32_t SourceCodeSize;
	uint64_t SourceCodeBase;
	uint64_t NativeCodeBase;
	TABLE PCConvertTable;
	TABLE Realloctions;
};
