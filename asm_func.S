;uint64_t MapSpcToTpc(uint64_t spc)
.extern SpcTpcHashTable
.extern MapSpcToTpc
.extern JmpFromNativeBlock

.global MapSpcToTpc_Thunk
MapSpcToTpc_Thunk:
pushq %rcx
pushq %r8
pushq %r9
pushq %r10
call MapSpcToTpc
popq %r10
popq %r9
popq %r8
popq %rcx
jmpq *%rax

.global JmpFromNativeBlock_Thunk
JmpFromNativeBlock_Thunk:
popq %rdi
popq %rsi
pushq %rcx
pushq %r8
pushq %r9
pushq %r10
call JmpFromNativeBlock
popq %r10
popq %r9
popq %r8
popq %rcx
jmpq *%rax
