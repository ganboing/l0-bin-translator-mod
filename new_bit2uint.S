.global Bit2Uint64

Bit2Uint64:
  shrdl $3, %esi, %eax
  shrq $3, %rsi
  addq %rsi, %rdi
  movl %edi, %ecx
  andq $-4, %rdi
  andl $3, %ecx
  shldl $3, %eax, %ecx
  movl 0(%rdi), %eax
  bswapl %eax
  leal -32(%ecx, %edx), %esi
  negl %esi
  js int32x
  shl %cl, %eax
  jmp finalfix
int32x:
  movl 4(%rdi), %edi
  bswapl %edi
  shldl %cl, %edi, %eax
finalfix:
  addl %ecx, %esi
  shrl %cl, %eax
  rep
  ret
