#pragma once

#define KERNEL_IS_VM_CLONE (1UL << 0)

#define K_RES_IS_ERR(res) ((uint64_t)res >= -4096UL)

#define K_ERR_NO_SPACE_FOR_FILE (uint64_t)-1
