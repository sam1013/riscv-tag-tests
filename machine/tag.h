/* Copyright (c) Samuel Weiser */
/* See LICENSE for license details. */

#ifndef _TAG_H
#define _TAG_H

#include "bits.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

enum tag_type {
  T_NORMAL = 0,
  T_CALLABLE = 1,
  T_UTRUSTED = 2,
  T_STRUSTED = 3
};

inline int load_tag(void *addr) {
  int rv = 32;
  asm volatile ("ltag %0, 0(%1)"
                :"=r"(rv)
                :"r"(addr)
                );
  return rv;
}


inline void store_tag(void *addr, int tag) {
  asm volatile ("stag %0, 0(%1)"
                :
                :"r"(tag), "r"(addr)
                );
}

bool tagify_mem(void* saddr, void* eaddr, uintptr_t tag);
bool verify_tag(void* saddr, void* eaddr, uintptr_t tag);

#endif
