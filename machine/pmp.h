/* Copyright (c) Samuel Weiser */
/* See LICENSE for license details. */

#ifndef _PMP_H
#define _PMP_H

#include "encoding.h"
#include <stdint.h>

#define PMP_ACCESS_R (1<<0)
#define PMP_ACCESS_W (1<<1)
#define PMP_ACCESS_X (1<<2)
#define PMP_ACCESS_T (1<<3)
#define PMP_ACCESS_A (1<<4)
#define PMP_ACCESS_ST (1<<5)

#define PMP_ACCESS_RWX (PMP_ACCESS_R | PMP_ACCESS_W | PMP_ACCESS_X)
#define PMP_ACCESS_TA (PMP_ACCESS_T | PMP_ACCESS_A)
#define PMP_ACCESS_F (PMP_ACCESS_RWX | PMP_ACCESS_TA)

typedef struct {
  uintptr_t base;
  uintptr_t bound;
  uintptr_t flags;
} pmp_entry_t;

/**
 * m ... privilege mode (m or s)
 * x ... PMP index to clear (0 ... 3)
 */
#define SET_PMP(m, x, base, bound, flags) \
  do { \
    write_csr(m##pmpbase##x,  (base)); \
    write_csr(m##pmpbound##x, (bound)); \
    write_csr(m##pmpflags##x, (flags)); \
  } while(0)

/**
 * m ... privilege mode (m or s)
 * x ... PMP index to clear (0 ... 3)
 */
#define CLEAR_PMP(m,x) \
  do { \
    write_csr(m##pmpbase##x,  0); \
    write_csr(m##pmpbound##x, 0); \
    write_csr(m##pmpflags##x, 0); \
  } while(0)

/**
 * m ... privilege mode (m or s)
 * x ... PMP index to read (0 ... 3)
 * entry ... to store the result. Must be of type pmp_entry_t*
 */
#define READ_PMP(m, x, entry) \
  do { \
    ((pmp_entry_t*)entry)->base = read_csr(m##pmpbase##x); \
    ((pmp_entry_t*)entry)->bound = read_csr(m##pmpbound##x); \
    ((pmp_entry_t*)entry)->flags = read_csr(m##pmpflags##x); \
  } while(0)

/**
 * m ... privilege mode (m or s)
 * x ... PMP index to load (0 ... 3)
 * symbol ... symbol to load
 * flags ... flags to write
 */
#define LOAD_PMP(m, x, symbol, flags) \
  do { \
    write_csr(m##pmpbase##x,  &_##symbol##_start); \
    write_csr(m##pmpbound##x, &_##symbol##_end); \
    write_csr(m##pmpflags##x, flags); \
  } while(0)

/**
 * m ... privilege mode (m or s)
 */
#define DUMP_PMP(m) { \
  printm("pmp:\n"); \
  printm("%016x - %016x [%016x]\n", read_csr(m##pmpbase0), read_csr(m##pmpbound0), read_csr(m##pmpflags0)); \
  printm("%016x - %016x [%016x]\n", read_csr(m##pmpbase1), read_csr(m##pmpbound1), read_csr(m##pmpflags1)); \
  printm("%016x - %016x [%016x]\n", read_csr(m##pmpbase2), read_csr(m##pmpbound2), read_csr(m##pmpflags2)); \
  printm("%016x - %016x [%016x]\n", read_csr(m##pmpbase3), read_csr(m##pmpbound3), read_csr(m##pmpflags3)); \
  printm("%016x - %016x [%016x]\n", read_csr(m##pmpbase4), read_csr(m##pmpbound4), read_csr(m##pmpflags4)); \
  printm("%016x - %016x [%016x]\n", read_csr(m##pmpbase5), read_csr(m##pmpbound5), read_csr(m##pmpflags5)); \
  printm("%016x - %016x [%016x]\n", read_csr(m##pmpbase6), read_csr(m##pmpbound6), read_csr(m##pmpflags6)); \
  printm("%016x - %016x [%016x]\n", read_csr(m##pmpbase7), read_csr(m##pmpbound7), read_csr(m##pmpflags7)); \
}

#endif
