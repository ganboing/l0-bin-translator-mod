#ifndef I0_MEM_LAYOUT_H_
#define I0_MEM_LAYOUT_H_

#define I0_MEMSPACE_PROGLOAD_BASE			(0x400000000ULL)
#define I0_MEMSPACE_PROGLOAD_LIMIT			(0x43fffffffULL)
#define I0_MEMSPACE_PROGLOAD_SIZE			(0x040000000ULL)

#define I0_MEMSPACE_REGFILE_BASE			(0x200000000ULL)
#define I0_MEMSPACE_REGFILE_LIMIT			(0x20000003fULL)
#define I0_MEMSPACE_REGFILE_SIZE			(0x000000040ULL)

#define I0_MEMSPACE_STDIN					(0x100000200ULL)
#define I0_MEMSPACE_STDOUT					(0X100000208ULL)
#define I0_MEMSPACE_SAVED_TASK_WRAPPER_SP	(0x100000000ULL)
#define I0_MEMSPACE_SAVED_TASK_SP_SYSCALL	(0x100000008ULL)
#define I0_MEMSPACE_CURRENT_TASK_STACKBASE	(0x100000100ULL)
#define I0_MEMSPACE_CURRENT_TASK_STACKLEN	(0x100000108ULL)
#define I0_MEMSPACE_CURRENT_TASK_FI			(0x100000118ULL)
#define I0_MEMSPACE_CURRENT_TASK_ID			(0x100000120ULL)

#define I0_MEMSPACE_PR_ADDR_REGISTER_START 	(0x200000000ULL)
#define I0_MEMSPACE_FRAME_POINTER			(I0_MEMSPACE_PR_ADDR_REGISTER_START)
#define I0_MEMSPACE_STACK_POINTER			(I0_MEMSPACE_PR_ADDR_REGISTER_START + 0x08)
#define I0_MEMSPACE_GENERAL_REG_1			(I0_MEMSPACE_PR_ADDR_REGISTER_START + 0x10)
#define I0_MEMSPACE_GENERAL_REG_2			(I0_MEMSPACE_PR_ADDR_REGISTER_START + 0x18)
#define I0_MEMSPACE_GENERAL_REG_3			(I0_MEMSPACE_PR_ADDR_REGISTER_START + 0x20)
#define I0_MEMSPACE_GENERAL_REG_4			(I0_MEMSPACE_PR_ADDR_REGISTER_START + 0x28)
#define I0_MEMSPACE_GENERAL_REG_5			(I0_MEMSPACE_PR_ADDR_REGISTER_START + 0x30)
#define I0_MEMSPACE_GENERAL_REG_6			(I0_MEMSPACE_PR_ADDR_REGISTER_START + 0x38)

#endif /* I0_MEM_LAYOUT_H_ */
