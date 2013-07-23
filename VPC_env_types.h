/*
 * VPC_env_types.h
 *
 *  Created on: Jul 24, 2013
 *      Author: PROGMAN
 */

#ifndef VPC_ENV_TYPES_H_
#define VPC_ENV_TYPES_H_

#include <stdint.h>

typedef struct _i0_code_meta_t {
    uint64_t i0_code_file_len;
} i0_code_meta_t;

extern i0_code_meta_t *_pi0codemeta;

#endif /* VPC_ENV_TYPES_H_ */
