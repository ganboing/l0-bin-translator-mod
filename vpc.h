#ifndef _VPC_H_
#define _VPC_H_

#include <stdint.h>
#include "types.h"

void sync_pmem();

struct program_args_t
{
    vpc_id_t id;
    char* program_file;
    uint64_t program_file_len;
};

// Command line parameter
enum {
    ROLE_MASTER = 1,
    ROLE_SLAVE = 2
};

uint64_t _strncmp(char *s1, uint64_t l1, char *s2, uint64_t l2);

#endif // _VPC_H_

