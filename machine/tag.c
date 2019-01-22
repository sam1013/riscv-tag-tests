/* Copyright (c) Samuel Weiser */
/* See LICENSE for license details. */

#include "tag.h"
#include "encoding.h"
#include "mtrap.h"

#define IS_ALIGNED(addr, bytes) !((uintptr_t)(addr) % (bytes))

__attribute__((section(".section.trusted")))
bool tagify_mem(void* saddr, void* eaddr, uintptr_t tag)
{
  if (!IS_ALIGNED(saddr, sizeof(uintptr_t)) ||
      !IS_ALIGNED(eaddr, sizeof(uintptr_t))) {
    printm("Misaligned address %p-%p\n", saddr, eaddr);
    return false;
  }
  printm("tagify %p-%p with %d\n", saddr, eaddr, tag);
  for(uintptr_t* paddr = (uintptr_t*)saddr; paddr < (uintptr_t*)eaddr; paddr++) {
    store_tag(paddr, tag);
  }
  return true;
}

bool verify_tag(void* saddr, void* eaddr, uintptr_t tag)
{
  if (!IS_ALIGNED(saddr, sizeof(uintptr_t)) ||
      !IS_ALIGNED(eaddr, sizeof(uintptr_t))) {
    printm("Misaligned address %p-%p\n", saddr, eaddr);
    return false;
  }
  for(uintptr_t* paddr = (uintptr_t*)saddr; paddr < (uintptr_t*)eaddr; paddr++) {
    uintptr_t t;
    if ((t = load_tag(paddr)) != tag) {
      printm("Expected tag 0x%x, found 0x%x\n", tag, t);
      return false;
    }
  }
  return true;
}
