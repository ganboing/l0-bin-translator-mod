#include <stdint.h>

enum X86_64_REG{
	x64_AX,  x64_CX,  x64_DX,  x64_BX,
	x64_SP,  x64_BP,  x64_SI,  x64_DI,
	x64_R8,  x64_R9,  x64_R10, x64_R11,
	x64_R12, x64_R13, x64_R14, x64_R15
};

#define REX_B_BIT 0
#define REX_X_BIT 1
#define REX_R_BIT 2
#define REX_W_BIT 3

inline  uint8_t EncodeModRM_NoCheck(uint8_t mod, uint8_t rm, uint8_t reg)
{
	uint8_t modrm = ((mod<<6)|(reg<<3)|rm);
	return modrm;
}
inline  uint8_t EncodeSIB_NoCheck(uint8_t ss, uint8_t idx, uint8_t base)
{
	uint8_t sib = ((ss<<6)|(idx<<3)|base);
	return sib;
}
inline  uint8_t EncodeRR_NoCheck(uint8_t src, uint8_t dest)
{
	return EncodeModRM_NoCheck(0x03, dest, src);
}
inline  void EncodeMovMI64ToAX_BYTE(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = 0xa0;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline  void EncodeMovMI64ToAX_WORD(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = 0x66;
	(*((*tpc)++)) = 0xa1;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline  void EncodeMovMI64ToAX_DWORD(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = 0xa1;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline  void EncodeMovMI64ToAX_QWORD(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = (0x40|(1<<REX_W_BIT));
	(*((*tpc)++)) = 0xa1;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline  void EncodeMovAxToMI64_BYTE(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = 0xa2;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline  void EncodeMovAxToMI64_WORD(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = 0x66;
	(*((*tpc)++)) = 0xa3;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline  void EncodeMovAxToMI64_DWORD(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = 0xa3;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline  void EncodeMovAxToMI64_QWORD(uint8_t** tpc, uint64_t moffs)
{
	(*((*tpc)++)) = (0x40|(1<<REX_W_BIT));
	(*((*tpc)++)) = 0xa3;
	(*((uint64_t*)(*tpc))) = moffs;
	(*tpc) += 8;
}
inline  void EncodeMovRR_BYTE(uint8_t** tpc, uint8_t src, uint8_t dest)
{
	uint8_t rex = 0;
	if(src>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		src &= 0x07;
	}
	if(dest>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		dest &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x88;
	(*((*tpc)++)) = EncodeRR_NoCheck(src, dest);
}
inline  void EncodeMovRR_WORD(uint8_t** tpc, uint8_t src, uint8_t dest)
{
	uint8_t rex = 0;
	(*((*tpc)++)) = 0x66;
	if(src>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		src &= 0x07;
	}
	if(dest>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		dest &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x89;
	(*((*tpc)++)) = EncodeRR_NoCheck(src, dest);
}
inline  void EncodeMovRR_DWORD(uint8_t** tpc, uint8_t src, uint8_t dest)
{
	uint8_t rex = 0;
	if(src>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		src &= 0x07;
	}
	if(dest>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		dest &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x89;
	(*((*tpc)++)) = EncodeRR_NoCheck(src, dest);
}
inline  void EncodeMovRR_QWORD(uint8_t** tpc, uint8_t src, uint8_t dest)
{
	uint8_t rex = 0;
	if(src>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		src &= 0x07;
	}
	if(dest>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		dest &= 0x07;
	}
	(*((*tpc)++)) = (0x40|rex|(1<<REX_W_BIT));
	(*((*tpc)++)) = 0x89;
	(*((*tpc)++)) = EncodeRR_NoCheck(src, dest);
}
inline  void EncodeMovMOff32ToR_BYTE(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x8a;
	(*((*tpc)++)) = EncodeModRM_NoCheck(2, base, reg);
	(*((uint32_t*)(*tpc))) = offset;
	(*tpc) += 4;
}
inline  void EncodeMovMOff32ToR_WORD(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x66;
	(*((*tpc)++)) = 0x8b;
	(*((*tpc)++)) = EncodeModRM_NoCheck(2, base, reg);
	(*((uint32_t*)(*tpc))) = offset;
	(*tpc) += 4;
}
inline  void EncodeMovMOff32ToR_DWORD(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x8b;
	(*((*tpc)++)) = EncodeModRM_NoCheck(2, base, reg);
	(*((uint32_t*)(*tpc))) = offset;
	(*tpc) += 4;
}
inline  void EncodeMovMOff32ToR_QWORD(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	(*((*tpc)++)) = (0x40|rex|(1<<REX_W_BIT));
	(*((*tpc)++)) = 0x8b;
	(*((*tpc)++)) = EncodeModRM_NoCheck(2, base, reg);
	(*((uint32_t*)(*tpc))) = offset;
	(*tpc) += 4;
}
inline  void EncodeMovMToR_BYTE(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x8a;
	if(base==x64_BP)
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(1, base, reg);
		(*((*tpc)++)) = 0x00;
	}
	else
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(0, base, reg);
	}
}
inline  void EncodeMovMToR_WORD(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x66;
	(*((*tpc)++)) = 0x8b;
	if(base==x64_BP)
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(1, base, reg);
		(*((*tpc)++)) = 0x00;
	}
	else
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(0, base, reg);
	}
}
inline  void EncodeMovMToR_DOWRD(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	if(rex)
	{
		(*((*tpc)++)) = (0x40|rex);
	}
	(*((*tpc)++)) = 0x8b;
	if(base==x64_BP)
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(1, base, reg);
		(*((*tpc)++)) = 0x00;
	}
	else
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(0, base, reg);
	}
}
inline  void EncodeMovMToR_QWORD(uint8_t** tpc, uint8_t reg, uint8_t base, uint32_t offset)
{
	uint8_t rex = 0;
	if(reg>=0x08)
	{
		rex |= (1<<REX_R_BIT);
		reg &= (0x07);
	}
	if(base>=0x08)
	{
		rex |= (1<<REX_B_BIT);
		base &= 0x07;
	}
	(*((*tpc)++)) = (0x40|rex|(1<<REX_W_BIT));
	(*((*tpc)++)) = 0x8b;
	if(base==x64_BP)
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(1, base, reg);
		(*((*tpc)++)) = 0x00;
	}
	else
	{
		(*((*tpc)++)) = EncodeModRM_NoCheck(0, base, reg);
	}
}
