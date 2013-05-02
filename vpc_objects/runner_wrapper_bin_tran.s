# .extern implemented in vpc.c
.extern _sys_new_runner
.extern _sys_idt_03
.extern _sys_idt_08
.extern _sys_idt_80
.extern _sys_stdin_q
.extern _sys_stdout_q
.extern _sys_indirect_jump_handler
.extern _sys_trampoline_handler
.extern get_native_code_address

.global sys_new_runner
.global sys_trampoline_handler
.global sys_indirect_jump_handler
.global sys_runner_wrapper
.global sys_back_runner_wrapper
.global sys_indirect_jump
.global sys_idt_03
.global sys_idt_08
.global sys_idt_80
.global sys_stdin_q
.global sys_stdout_q

sys_trampoline_handler:
# protect %rcx, %r9, %r8
# Not know whey %r10 can also be changed. For system calls, R10 is used
# instead of RCX.
# http://en.wikipedia.org/wiki/X86_calling_conventions#x86-64_calling_conventions
# Anyway, it is safe to save it here.
pushq %rcx
pushq %r10
pushq %r9
pushq %r8
call _sys_trampoline_handler
popq %r8
popq %r9
popq %r10
popq %rcx
jmpq *%rax

sys_indirect_jump_handler:
# protect %rcx, %r9, %r8
pushq %rcx
pushq %r10
pushq %r9
pushq %r8
call _sys_indirect_jump_handler
popq %r8
popq %r9
popq %r10
popq %rcx
jmpq *%rax

# indirect_jump hash and jump
# the same hash function is used by _sys_indirect_jump_handler
sys_indirect_jump:
# target address in %rdi
# l = log(table size)
# k = log(entry size)
# hash = ( %rdi & (2^(l-k) - 1) ) << k
# target in the table: base + hash
# k = 6; l = 20
movq %rdi, %rax
andq $0x3fff, %rax
shlq $0x6, %rax
movq $0x130000000, %rsi
addq %rsi, %rax
# jump to table entry
jmpq *%rax

## # protect %rcx, %r9, %r8
## pushq %rcx
## pushq %r9
## pushq %r8
# call get_native_code_address
# popq %r8
# popq %r9
# popq %rcx
# jmpq *%rax

sys_runner_wrapper:
# save callee saved registers, %rsp and etc.
pushq %rbx
pushq %r10
pushq %r13
pushq %r14
pushq %r15
jmpq *%rdi # jump to runner code

sys_back_runner_wrapper:
# TODO save register file to memory
# return value
movq %rdi, %rax
# recover calle saved registers, %rsp and etc.
popq %r15
popq %r14
popq %r13
popq %r10
popq %rbx
retq

sys_new_runner:
# return address in stack head
# protect %rcx, %r9, %r8
pushq %rcx
pushq %r10
pushq %r9
pushq %r8
# call sys call
call _sys_new_runner
popq %r8
popq %r9
popq %r10
popq %rcx
popq %rax
# skip instrs after leaq 0(%rip)
addq $13, %rax
jmp *%rax

sys_idt_03:
# return address in stack head
# protect %rcx, %r9, %r8
pushq %rcx
pushq %r10
pushq %r9
pushq %r8
call _sys_idt_03
popq %r8
popq %r9
popq %r10
popq %rcx
popq %rax
# skip instrs after leaq 0(%rip)
addq $6, %rax
jmp *%rax

sys_idt_08:
# return address in stack head
# protect %rcx, %r9, %r8
pushq %rcx
pushq %r10
pushq %r9
pushq %r8
call _sys_idt_08
popq %r8
popq %r9
popq %r10
popq %rcx
popq %rax
# skip instrs after leaq 0(%rip)
addq $6, %rax
jmp *%rax

sys_idt_80:
# return address in stack head
# protect %rcx, %r9, %r8
pushq %rcx
pushq %r10
pushq %r9
pushq %r8
call _sys_idt_80
popq %r8
popq %r9
popq %r10
popq %rcx
popq %rax
# skip instrs after leaq 0(%rip)
addq $6, %rax
jmp *%rax

sys_stdin_q:
# return address in stack head
# protect %rcx, %r9, %r8
pushq %rcx
pushq %r10
pushq %r9
pushq %r8
call _sys_stdin_q
movq %rax, %rdi
popq %r8
popq %r9
popq %r10
popq %rcx
popq %rax
# skip instrs after leaq 0(%rip)
addq $16, %rax
jmp *%rax


sys_stdout_q:
# return address in stack head
# protect %rcx, %r9, %r8
pushq %rcx
pushq %r10
pushq %r9
pushq %r8
call _sys_stdout_q
popq %r8
popq %r9
popq %r10
popq %rcx
popq %rax
# skip instrs after leaq 0(%rip)
addq $13, %rax
jmp *%rax


