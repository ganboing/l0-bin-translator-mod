.global start

start:
  movl $0xDEADBEEF, %ebx
  movl $0x12345678, %eax
  movl $0xaabbccdd, %ecx
  movw %ax, %bx
  bswapl %ebx
  movb %cl, %bl
  rolw %8, %bx
  mov %al, %bl
