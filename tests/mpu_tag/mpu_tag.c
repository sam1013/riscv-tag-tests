/* Copyright (c) Samuel Weiser */
/* See LICENSE for license details. */

#include "mpu_tag.h"
#include "tag.h"

#define BUFFER_SLOTS 8
__attribute__((section(".section.a"))) volatile uintptr_t buffer_a[BUFFER_SLOTS];
__attribute__((section(".section.b"))) volatile uintptr_t buffer_b[BUFFER_SLOTS];

__attribute__((section(".section.c"))) void buffer_x() {
  asm volatile("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n ret\n");
}

__attribute__((section(".section.d"))) volatile uintptr_t buffer_d[BUFFER_SLOTS];

#define NSTACK_SIZE 1024
uintptr_t nstack[NSTACK_SIZE];

#define PREPARE_NSTACK() \
  tagify_mem(&nstack, &nstack[NSTACK_SIZE-1], T_NORMAL); \
  volatile uintptr_t oldstack; \
  uintptr_t stack_top = (uintptr_t)&nstack[NSTACK_SIZE-1]; \
  asm volatile("mv %0, sp\n" : "=r"(oldstack)); \
  asm volatile("mv sp, %0\n" : : "r"(stack_top) : "sp");

#define RESTORE_SSTACK() \
  asm volatile("mv sp, %0\n" \
               :: "r"(oldstack) : "sp");

#define CALL_NORMAL(callee) do { \
    PREPARE_NSTACK(); \
    callee; \
    RESTORE_SSTACK(); \
  } while(0)

#define T_DEFAULT T_NORMAL

__attribute__((section(".section.trusted"),noinline))
void st_set_ststatus(unsigned int bits)
{
  set_csr(ststatus, bits);
}

__attribute__((section(".section.trusted"),noinline))
void st_clear_ststatus(unsigned int bits)
{
  clear_csr(ststatus, bits);
}

static void m_init_test()
{
  TEST_SUITE("init test");
  /* Enable policy checks and enclave as runnable */
  st_set_ststatus(TSTATUS_UE | TSTATUS_EN);
  ASSERT(read_csr(ststatus) & (TSTATUS_UE | TSTATUS_EN), "Protection not enabled\n!");
  st_clear_ststatus(TSTATUS_UE);
  ASSERT((read_csr(ststatus) & (TSTATUS_UE | TSTATUS_EN)) == TSTATUS_EN, "Protection not enabled\n!");
}

typedef enum {
  N = 1<<T_NORMAL,
  C = 1<<T_CALLABLE,
  UT = 1<<T_UTRUSTED,
  ST = 1<<T_STRUSTED,
} tag_mask_t;

static void generic_stag_test(int tag_mask) {

  if (tag_mask & N) {
    TEST_STAG(buffer_a, T_NORMAL);
  } else {
    TEST_STAG_FAULT(buffer_a, T_NORMAL);
  }
  if (tag_mask & C) {
    TEST_STAG(buffer_a, T_CALLABLE);
  } else {
    TEST_STAG_FAULT(buffer_a, T_CALLABLE);
  }
  if (tag_mask & UT) {
    TEST_STAG(buffer_a, T_UTRUSTED);
  } else {
    TEST_STAG_FAULT(buffer_a, T_UTRUSTED);
  }
  if (tag_mask & ST) {
    TEST_STAG(buffer_a, T_STRUSTED);
  } else {
    TEST_STAG_FAULT(buffer_a, T_STRUSTED);
  }
}

/**
 * Test ltag/stag instructions
 */
static void m_basic_lstag_test()
{
  TEST_SUITE("basic ltag/stag tests");

  CLEAR_PMP(m, 0);
  CLEAR_PMP(m, 1);
  CLEAR_PMP(m, 2);
  CLEAR_PMP(m, 3);

  /* Initially, all memory is tagged T_DEFAULT */
  TEST_VERIFY_TAG(&_global_start, &_global_end, T_DEFAULT);
  TEST_VERIFY_TAG(&_test_start, &_test_end, T_DEFAULT);
  TEST_VERIFY_TAG(&_section_start_a, &_section_end_a, T_DEFAULT);
  TEST_VERIFY_TAG(&_section_start_b, &_section_end_b, T_DEFAULT);
  TEST_VERIFY_TAG(&_section_start_c, &_section_end_c, T_DEFAULT);

  /* ST can overwrite any tag with any value */
  tagify_mem(&_section_start_a, &_section_end_a, T_NORMAL);
  generic_stag_test(N | C | UT | ST);
  tagify_mem(&_section_start_a, &_section_end_a, T_CALLABLE);
  generic_stag_test(N | C | UT | ST);
  tagify_mem(&_section_start_a, &_section_end_a, T_UTRUSTED);
  generic_stag_test(N | C | UT | ST);
  tagify_mem(&_section_start_a, &_section_end_a, T_STRUSTED);
  generic_stag_test(N | C | UT | ST);

  /* coarse-grained tag checks */
  tagify_mem(&_section_start_a, &_section_end_a, T_NORMAL);
  tagify_mem(&_section_start_b, &_section_end_b, T_STRUSTED);
  tagify_mem(&_section_start_c, &_section_end_c, T_CALLABLE);

  TEST_VERIFY_TAG(&_global_start, &_global_end, T_DEFAULT);
  TEST_VERIFY_TAG(&_test_start, &_test_end, T_DEFAULT);
  TEST_VERIFY_TAG(&_section_start_a, &_section_end_a, T_NORMAL);
  TEST_VERIFY_TAG(&_section_start_b, &_section_end_b, T_STRUSTED);
  TEST_VERIFY_TAG(&_section_start_c, &_section_end_c, T_CALLABLE);

  /* fine-grained tag checks */
  uintptr_t array[4];
  tagify_mem(&array[0], &array[1], T_NORMAL);
  tagify_mem(&array[1], &array[2], T_CALLABLE);
  tagify_mem(&array[2], &array[3], T_UTRUSTED);
  tagify_mem(&array[3], &array[4], T_STRUSTED);
  TEST_VERIFY_TAG(&array[0], &array[1], T_NORMAL);
  TEST_VERIFY_TAG(&array[1], &array[2], T_CALLABLE);
  TEST_VERIFY_TAG(&array[2], &array[3], T_UTRUSTED);
  TEST_VERIFY_TAG(&array[3], &array[4], T_STRUSTED);

  /* test possible tags */
  tagify_mem(&array[0], &array[1], 0xFFFF);
  TEST_VERIFY_TAG(&array[0], &array[1], 0x0003);

  /* reset stack variable */
  tagify_mem(&array[0], &array[4], T_UTRUSTED);
}

/*
 * Do read access test on asm 'access_func'.
 * Prepare *ptr with 'word' and compare if read value matches 'expected'
 */
#define DYN_TEST_RACCESS_OK(access_func, ptr, word, expected) { \
    uintptr_t* target = (uintptr_t*)&(ptr); \
    *target = (uintptr_t)word; \
    uintptr_t result = DYN_TEST(__LINE__, access_func, EPC_INSN, (uintptr_t)&(ptr), -1, TEST_SUCCESS); \
    ASSERT(result == expected, "Wrong value read: 0x%x instead of 0x%x, LINE %d\n", result, (expected), __LINE__); \
  }

/*
 * Do load-test-tag on asm 'access_func'.
 * Compare if tag test result matches 'expected'
 */
#define DYN_TEST_TACCESS_OK(access_func, ptr, expected) { \
    uintptr_t result = DYN_TEST(__LINE__, access_func, EPC_INSN, (uintptr_t)&(ptr), -1, TEST_SUCCESS); \
    ASSERT(result == expected, "Wrong tag check: 0x%x instead of 0x%x, LINE %d\n", result, (expected), __LINE__); \
  }

/*
 * Do read access test on asm 'access_func' and expect a TAG_MISMATCH
 */
#define DYN_TEST_RACCESS_NO(access_func, ptr) { \
    uintptr_t result = DYN_TEST(__LINE__, access_func, EPC_INSN, (uintptr_t)&(ptr), -1, CAUSE_TAG_MISMATCH); \
  }

__attribute__((noinline))
static void m_basic_lc_test()
{
  TEST_SUITE("basic lc tests");

  // For checking if asm + disas works
  //~ int result = 0;
  //~ uintptr_t addr = (uintptr_t)&result;
  //~ asm volatile("lb %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("lh %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("lw %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("ld %0, (%1)\n" : "+r"(result) : "r"(addr));

  //~ asm volatile("lbct c, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("lhct n, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("lwct st, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("ldct n, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("ldct c, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("ldct ut, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("ldct st, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("lbuct st, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("lhuct st, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("lwuct st, %0, (%1)\n" : "+r"(result) : "r"(addr));

  tagify_mem(&_section_start_a, &_section_end_a, T_NORMAL);
  tagify_mem(&_section_start_b, &_section_end_b, T_CALLABLE);
  tagify_mem(&_section_start_c, &_section_end_c, T_UTRUSTED);
  tagify_mem(&_section_start_d, &_section_end_d, T_STRUSTED);

  uintptr_t* pbuffer_x = ((uintptr_t*)&buffer_x);
  uintptr_t buffer_x_orig = pbuffer_x[0];
  DYN_TEST_RACCESS_OK(LBC_ASM(n),  buffer_a, 0x7F7F7F7F, 0x0000007F);
  DYN_TEST_RACCESS_OK(LBC_ASM(n),  buffer_a, 0x000000FF, -1);
  DYN_TEST_RACCESS_NO(LBC_ASM(c),  buffer_a);
  DYN_TEST_RACCESS_NO(LBC_ASM(ut), buffer_a);
  DYN_TEST_RACCESS_NO(LBC_ASM(st), buffer_a);
  DYN_TEST_RACCESS_NO(LBC_ASM(n),  buffer_b);
  DYN_TEST_RACCESS_OK(LBC_ASM(c),  buffer_b, 0x7F7F7F7F, 0x0000007F);
  DYN_TEST_RACCESS_OK(LBC_ASM(c),  buffer_b, 0x000000FF, -1);
  DYN_TEST_RACCESS_NO(LBC_ASM(ut), buffer_b);
  DYN_TEST_RACCESS_NO(LBC_ASM(st), buffer_b);
  DYN_TEST_RACCESS_NO(LBC_ASM(n),  buffer_x);
  DYN_TEST_RACCESS_NO(LBC_ASM(c),  buffer_x);
  DYN_TEST_RACCESS_OK(LBC_ASM(ut), buffer_x, 0x7F7F7F7F, 0x0000007F);
  DYN_TEST_RACCESS_OK(LBC_ASM(ut), buffer_x, 0x000000FF, -1);
  DYN_TEST_RACCESS_NO(LBC_ASM(st), buffer_x);
  DYN_TEST_RACCESS_NO(LBC_ASM(n),  buffer_d);
  DYN_TEST_RACCESS_NO(LBC_ASM(c),  buffer_d);
  DYN_TEST_RACCESS_NO(LBC_ASM(ut), buffer_d);
  DYN_TEST_RACCESS_OK(LBC_ASM(st), buffer_d, 0x7F7F7F7F, 0x0000007F);
  DYN_TEST_RACCESS_OK(LBC_ASM(st), buffer_d, 0x000000FF, -1);
  pbuffer_x[0] = buffer_x_orig; /* restore original value, which is later needed for fetch tests */

  DYN_TEST_RACCESS_OK(LBUC_ASM(n),  buffer_a, 0x7F7F7F7F, 0x0000007F);
  DYN_TEST_RACCESS_OK(LBUC_ASM(n),  buffer_a, 0x000000FF, 0xFF);
  DYN_TEST_RACCESS_NO(LBUC_ASM(c),  buffer_a);
  DYN_TEST_RACCESS_NO(LBUC_ASM(ut), buffer_a);
  DYN_TEST_RACCESS_NO(LBUC_ASM(st), buffer_a);
  DYN_TEST_RACCESS_NO(LBUC_ASM(n),  buffer_b);
  DYN_TEST_RACCESS_NO(LBUC_ASM(ut), buffer_b);
  DYN_TEST_RACCESS_NO(LBUC_ASM(st), buffer_b);
  DYN_TEST_RACCESS_NO(LBUC_ASM(n),  buffer_x);
  DYN_TEST_RACCESS_NO(LBUC_ASM(c),  buffer_x);
  DYN_TEST_RACCESS_NO(LBUC_ASM(st), buffer_x);
  DYN_TEST_RACCESS_NO(LBUC_ASM(n),  buffer_d);
  DYN_TEST_RACCESS_NO(LBUC_ASM(c),  buffer_d);
  DYN_TEST_RACCESS_NO(LBUC_ASM(ut), buffer_d);

  DYN_TEST_RACCESS_OK(LHC_ASM(n),  buffer_a, 0x7F7F7F7F, 0x00007F7F);
  DYN_TEST_RACCESS_OK(LHC_ASM(n),  buffer_a, 0x0000FFFF, -1);
  DYN_TEST_RACCESS_NO(LHC_ASM(c),  buffer_a);
  DYN_TEST_RACCESS_NO(LHC_ASM(ut), buffer_a);
  DYN_TEST_RACCESS_NO(LHC_ASM(st), buffer_a);
  DYN_TEST_RACCESS_NO(LHC_ASM(n),  buffer_b);
  DYN_TEST_RACCESS_NO(LHC_ASM(ut), buffer_b);
  DYN_TEST_RACCESS_NO(LHC_ASM(st), buffer_b);
  DYN_TEST_RACCESS_NO(LHC_ASM(n),  buffer_x);
  DYN_TEST_RACCESS_NO(LHC_ASM(c),  buffer_x);
  DYN_TEST_RACCESS_NO(LHC_ASM(st), buffer_x);
  DYN_TEST_RACCESS_NO(LHC_ASM(n),  buffer_d);
  DYN_TEST_RACCESS_NO(LHC_ASM(c),  buffer_d);
  DYN_TEST_RACCESS_NO(LHC_ASM(ut), buffer_d);

  DYN_TEST_RACCESS_OK(LHUC_ASM(n),  buffer_a, 0x7F7F7F7F, 0x00007F7F);
  DYN_TEST_RACCESS_OK(LHUC_ASM(n),  buffer_a, 0x0000FFFF, 0xFFFF);
  DYN_TEST_RACCESS_NO(LHUC_ASM(c),  buffer_a);
  DYN_TEST_RACCESS_NO(LHUC_ASM(ut), buffer_a);
  DYN_TEST_RACCESS_NO(LHUC_ASM(st), buffer_a);
  DYN_TEST_RACCESS_NO(LHUC_ASM(n),  buffer_b);
  DYN_TEST_RACCESS_NO(LHUC_ASM(ut), buffer_b);
  DYN_TEST_RACCESS_NO(LHUC_ASM(st), buffer_b);
  DYN_TEST_RACCESS_NO(LHUC_ASM(n),  buffer_x);
  DYN_TEST_RACCESS_NO(LHUC_ASM(c),  buffer_x);
  DYN_TEST_RACCESS_NO(LHUC_ASM(st), buffer_x);
  DYN_TEST_RACCESS_NO(LHUC_ASM(n),  buffer_d);
  DYN_TEST_RACCESS_NO(LHUC_ASM(c),  buffer_d);
  DYN_TEST_RACCESS_NO(LHUC_ASM(ut), buffer_d);

  DYN_TEST_RACCESS_OK(LWC_ASM(n),  buffer_a, 0x7F7F7F7F, 0x7F7F7F7F); // INVALID-INSTRUCTION_FAULT!!
  DYN_TEST_RACCESS_OK(LWC_ASM(n),  buffer_a, 0xFFFFFFFF, -1);
  DYN_TEST_RACCESS_NO(LWC_ASM(c),  buffer_a);
  DYN_TEST_RACCESS_NO(LWC_ASM(ut), buffer_a);
  DYN_TEST_RACCESS_NO(LWC_ASM(st), buffer_a);
  DYN_TEST_RACCESS_NO(LWC_ASM(n),  buffer_b);
  DYN_TEST_RACCESS_NO(LWC_ASM(ut), buffer_b);
  DYN_TEST_RACCESS_NO(LWC_ASM(st), buffer_b);
  DYN_TEST_RACCESS_NO(LWC_ASM(n),  buffer_x);
  DYN_TEST_RACCESS_NO(LWC_ASM(c),  buffer_x);
  DYN_TEST_RACCESS_NO(LWC_ASM(st), buffer_x);
  DYN_TEST_RACCESS_NO(LWC_ASM(n),  buffer_d);
  DYN_TEST_RACCESS_NO(LWC_ASM(c),  buffer_d);
  DYN_TEST_RACCESS_NO(LWC_ASM(ut), buffer_d);

#if __riscv_xlen == 64
  DYN_TEST_RACCESS_OK(LWC_ASM(n),  buffer_a, 0x7F7F7F7F7F7F7F7F, 0x7F7F7F7F); // INVALID-INSTRUCTION_FAULT!!
  DYN_TEST_RACCESS_OK(LWC_ASM(n),  buffer_a, 0xFFFFFFFFFFFFFFFF, -1);

  DYN_TEST_RACCESS_OK(LWUC_ASM(n),  buffer_a, 0x7F7F7F7F7F7F7F7F, 0x7F7F7F7F);
  DYN_TEST_RACCESS_OK(LWUC_ASM(n),  buffer_a, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFF);
  DYN_TEST_RACCESS_NO(LWUC_ASM(c),  buffer_a);
  DYN_TEST_RACCESS_NO(LWUC_ASM(ut), buffer_a);
  DYN_TEST_RACCESS_NO(LWUC_ASM(st), buffer_a);
  DYN_TEST_RACCESS_NO(LWUC_ASM(n),  buffer_b);
  DYN_TEST_RACCESS_NO(LWUC_ASM(ut), buffer_b);
  DYN_TEST_RACCESS_NO(LWUC_ASM(st), buffer_b);
  DYN_TEST_RACCESS_NO(LWUC_ASM(n),  buffer_x);
  DYN_TEST_RACCESS_NO(LWUC_ASM(c),  buffer_x);
  DYN_TEST_RACCESS_NO(LWUC_ASM(st), buffer_x);
  DYN_TEST_RACCESS_NO(LWUC_ASM(n),  buffer_d);
  DYN_TEST_RACCESS_NO(LWUC_ASM(c),  buffer_d);
  DYN_TEST_RACCESS_NO(LWUC_ASM(ut), buffer_d);

  DYN_TEST_RACCESS_OK(LDC_ASM(n),  buffer_a, 0x7F7F7F7F7F7F7F7F, 0x7F7F7F7F7F7F7F7F);
  DYN_TEST_RACCESS_OK(LDC_ASM(n),  buffer_a, 0xFFFFFFFFFFFFFFFF, -1);
  DYN_TEST_RACCESS_NO(LDC_ASM(c),  buffer_a);
  DYN_TEST_RACCESS_NO(LDC_ASM(ut), buffer_a);
  DYN_TEST_RACCESS_NO(LDC_ASM(st), buffer_a);
  DYN_TEST_RACCESS_NO(LDC_ASM(n),  buffer_b);
  DYN_TEST_RACCESS_NO(LDC_ASM(ut), buffer_b);
  DYN_TEST_RACCESS_NO(LDC_ASM(st), buffer_b);
  DYN_TEST_RACCESS_NO(LDC_ASM(n),  buffer_x);
  DYN_TEST_RACCESS_NO(LDC_ASM(c),  buffer_x);
  DYN_TEST_RACCESS_NO(LDC_ASM(st), buffer_x);
  DYN_TEST_RACCESS_NO(LDC_ASM(n),  buffer_d);
  DYN_TEST_RACCESS_NO(LDC_ASM(c),  buffer_d);
  DYN_TEST_RACCESS_NO(LDC_ASM(ut), buffer_d);
#endif
}

/*
 * Do read access test on asm 'access_func'.
 * Prepare *ptr with 'word' and compare if read value matches 'expected'
 */
#define DYN_TEST_WACCESS_OK(access_func, ptr, word, expected) { \
    uintptr_t* target = (uintptr_t*)&(ptr); \
    *target = 0; \
    DYN_TEST(__LINE__, access_func, EPC_INSN, (uintptr_t)&(ptr), (word), TEST_SUCCESS); \
    ASSERT(*target == (expected), "Wrong value read: 0x%x instead of 0x%x, LINE %d\n", *target, (expected), __LINE__); \
  }

/*
 * Do read access test on asm 'access_func' and expect a TAG_MISMATCH
 */
#define DYN_TEST_WACCESS_NO(access_func, ptr, word) { \
    uintptr_t* target = (uintptr_t*)&(ptr); \
    *target = 0; \
    uintptr_t result = DYN_TEST(__LINE__, access_func, EPC_INSN, (uintptr_t)&(ptr), (word), CAUSE_TAG_MISMATCH); \
    ASSERT(*target == 0, "Wrong value read: 0x%x instead of 0x%x, LINE %d\n", *target, 0, __LINE__); \
  }

__attribute__((noinline))
static void m_basic_sct_test()
{
  TEST_SUITE("basic sct tests");

  // For checking if asm + disas works
  //~ int result = 0;
  //~ uintptr_t addr = (uintptr_t)&result;
  //~ asm volatile("sbct n, n, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("shct c, n, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("swct ut, n, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("sdct st, n, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("sdct n, c, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("sdct n, ut, %0, (%1)\n" : "+r"(result) : "r"(addr));
  //~ asm volatile("sdct n, st, %0, (%1)\n" : "+r"(result) : "r"(addr));

  tagify_mem(&_section_start_a, &_section_end_a, T_NORMAL);

  /* Check if store instructions preserve original semantics */
  DYN_TEST_WACCESS_OK(SBCT_ASM(n, n), buffer_a, 0x7F7F7F7F, 0x0000007F);
  DYN_TEST_WACCESS_OK(SHCT_ASM(n, n), buffer_a, 0x7F7F7F7F, 0x00007F7F);
  DYN_TEST_WACCESS_OK(SWCT_ASM(n, n), buffer_a, 0x7F7F7F7F, 0x7F7F7F7F);
#if __riscv_xlen == 64
  DYN_TEST_WACCESS_OK(SWCT_ASM(n, n), buffer_a, 0x7F7F7F7F7F7F7F7F, 0x000000007F7F7F7F);
  DYN_TEST_WACCESS_OK(SDCT_ASM(n, n), buffer_a, 0x7F7F7F7F7F7F7F7F, 0x7F7F7F7F7F7F7F7F);
#endif

  /* Check that expecting a wrong tag causes a fault and nothing is written */
  DYN_TEST_WACCESS_NO(SBCT_ASM(c, n),  buffer_a, 0x7F7F7F7F);
  DYN_TEST_WACCESS_NO(SHCT_ASM(c, n),  buffer_a, 0x7F7F7F7F);
  DYN_TEST_WACCESS_NO(SWCT_ASM(c, n),  buffer_a, 0x7F7F7F7F);
#if __riscv_xlen == 64
  DYN_TEST_WACCESS_NO(SDCT_ASM(c, n),  buffer_a, 0x7F7F7F7F7F7F7F7F);
#endif
  DYN_TEST_WACCESS_NO(SBCT_ASM(ut, n), buffer_a, 0x7F7F7F7F);
  DYN_TEST_WACCESS_NO(SHCT_ASM(ut, n), buffer_a, 0x7F7F7F7F);
  DYN_TEST_WACCESS_NO(SWCT_ASM(ut, n), buffer_a, 0x7F7F7F7F);
#if __riscv_xlen == 64
  DYN_TEST_WACCESS_NO(SDCT_ASM(ut, n), buffer_a, 0x7F7F7F7F7F7F7F7F);
#endif
  DYN_TEST_WACCESS_NO(SBCT_ASM(st, n), buffer_a, 0x7F7F7F7F);
  DYN_TEST_WACCESS_NO(SHCT_ASM(st, n), buffer_a, 0x7F7F7F7F);
  DYN_TEST_WACCESS_NO(SWCT_ASM(st, n), buffer_a, 0x7F7F7F7F);
#if __riscv_xlen == 64
  DYN_TEST_WACCESS_NO(SDCT_ASM(st, n), buffer_a, 0x7F7F7F7F7F7F7F7F);
#endif

  /* Check whether store sets tags correctly */
  DYN_TEST_WACCESS_OK(SBCT_ASM(n, c), buffer_a, 0x7F7F7F7F, 0x0000007F);
  DYN_TEST_RACCESS_OK(LBC_ASM(c),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SBCT_ASM(c, ut), buffer_a, 0x7F7F7F7F, 0x0000007F);
  DYN_TEST_RACCESS_OK(LBC_ASM(ut),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SBCT_ASM(ut, st), buffer_a, 0x7F7F7F7F, 0x0000007F);
  DYN_TEST_RACCESS_OK(LBC_ASM(st),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SBCT_ASM(st, n), buffer_a, 0x7F7F7F7F, 0x0000007F);
  DYN_TEST_RACCESS_OK(LBC_ASM(n),  buffer_a, 0, 0);

  DYN_TEST_WACCESS_OK(SHCT_ASM(n, c), buffer_a, 0x7F7F7F7F, 0x00007F7F);
  DYN_TEST_RACCESS_OK(LHC_ASM(c),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SHCT_ASM(c, ut), buffer_a, 0x7F7F7F7F, 0x00007F7F);
  DYN_TEST_RACCESS_OK(LHC_ASM(ut),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SHCT_ASM(ut, st), buffer_a, 0x7F7F7F7F, 0x00007F7F);
  DYN_TEST_RACCESS_OK(LHC_ASM(st),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SHCT_ASM(st, n), buffer_a, 0x7F7F7F7F, 0x00007F7F);
  DYN_TEST_RACCESS_OK(LHC_ASM(n),  buffer_a, 0, 0);

  DYN_TEST_WACCESS_OK(SWCT_ASM(n, c), buffer_a, 0x7F7F7F7F, 0x7F7F7F7F);
  DYN_TEST_RACCESS_OK(LWC_ASM(c),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SWCT_ASM(c, ut), buffer_a, 0x7F7F7F7F, 0x7F7F7F7F);
  DYN_TEST_RACCESS_OK(LWC_ASM(ut),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SWCT_ASM(ut, st), buffer_a, 0x7F7F7F7F, 0x7F7F7F7F);
  DYN_TEST_RACCESS_OK(LWC_ASM(st),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SWCT_ASM(st, n), buffer_a, 0x7F7F7F7F, 0x7F7F7F7F);
  DYN_TEST_RACCESS_OK(LWC_ASM(n),  buffer_a, 0, 0);

#if __riscv_xlen == 64
  DYN_TEST_WACCESS_OK(SWCT_ASM(n, c), buffer_a, 0x7F7F7F7F, 0x000000007F7F7F7F);
  DYN_TEST_RACCESS_OK(LWC_ASM(c),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SWCT_ASM(c, ut), buffer_a, 0x7F7F7F7F, 0x000000007F7F7F7F);
  DYN_TEST_RACCESS_OK(LWC_ASM(ut),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SWCT_ASM(ut, st), buffer_a, 0x7F7F7F7F, 0x000000007F7F7F7F);
  DYN_TEST_RACCESS_OK(LWC_ASM(st),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SWCT_ASM(st, n), buffer_a, 0x7F7F7F7F, 0x000000007F7F7F7F);
  DYN_TEST_RACCESS_OK(LWC_ASM(n),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SDCT_ASM(n, c), buffer_a, 0x7F7F7F7F7F7F7F7F, 0x7F7F7F7F7F7F7F7F);
  DYN_TEST_RACCESS_OK(LDC_ASM(c),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SDCT_ASM(c, ut), buffer_a, 0x7F7F7F7F7F7F7F7F, 0x7F7F7F7F7F7F7F7F);
  DYN_TEST_RACCESS_OK(LDC_ASM(ut),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SDCT_ASM(ut, st), buffer_a, 0x7F7F7F7F7F7F7F7F, 0x7F7F7F7F7F7F7F7F);
  DYN_TEST_RACCESS_OK(LDC_ASM(st),  buffer_a, 0, 0);
  DYN_TEST_WACCESS_OK(SDCT_ASM(st, n), buffer_a, 0x7F7F7F7F7F7F7F7F, 0x7F7F7F7F7F7F7F7F);
  DYN_TEST_RACCESS_OK(LDC_ASM(n),  buffer_a, 0, 0);
#endif

  /* load-test-tag instruction tests */
  /* ltt subtracts expected tag from real tag */
  DYN_TEST_TACCESS_OK(LTT_ASM(n),  buffer_a, 0);
  DYN_TEST_TACCESS_OK(LTT_ASM(c),  buffer_a, 1);
  DYN_TEST_TACCESS_OK(LTT_ASM(ut), buffer_a, 2);
  DYN_TEST_TACCESS_OK(LTT_ASM(st), buffer_a, 3);
  DYN_TEST_WACCESS_OK(SWCT_ASM(n, c), buffer_a, 0x7F7F7F7F, 0x7F7F7F7F);
  DYN_TEST_TACCESS_OK(LTT_ASM(n),  buffer_a, -1);
  DYN_TEST_TACCESS_OK(LTT_ASM(c),  buffer_a, 0);
  DYN_TEST_TACCESS_OK(LTT_ASM(ut), buffer_a, 1);
  DYN_TEST_TACCESS_OK(LTT_ASM(st), buffer_a, 2);
  DYN_TEST_WACCESS_OK(SWCT_ASM(c, ut), buffer_a, 0x7F7F7F7F, 0x7F7F7F7F);
  DYN_TEST_TACCESS_OK(LTT_ASM(n),  buffer_a, -2);
  DYN_TEST_TACCESS_OK(LTT_ASM(c),  buffer_a, -1);
  DYN_TEST_TACCESS_OK(LTT_ASM(ut), buffer_a, 0);
  DYN_TEST_TACCESS_OK(LTT_ASM(st), buffer_a, 1);
  DYN_TEST_WACCESS_OK(SWCT_ASM(ut, st), buffer_a, 0x7F7F7F7F, 0x7F7F7F7F);
  DYN_TEST_TACCESS_OK(LTT_ASM(n),  buffer_a, -3);
  DYN_TEST_TACCESS_OK(LTT_ASM(c),  buffer_a, -2);
  DYN_TEST_TACCESS_OK(LTT_ASM(ut), buffer_a, -1);
  DYN_TEST_TACCESS_OK(LTT_ASM(st), buffer_a, 0);
  DYN_TEST_WACCESS_OK(SWCT_ASM(st, n), buffer_a, 0x7F7F7F7F, 0x7F7F7F7F);
}

__attribute__((noinline)) static void st_modeswitch_sn()
{
  TEST_SUITE("modeswitch tests");
  /* test wrappers are set to ST mode */
  /* Since normal-mode can only return to callable, we have special test wrapper
   * that is set to callable */
  tagify_mem(&_test_start, &_test_end, T_CALLABLE);

  /* ST can call T_NORMAL */
  tagify_mem(&_section_start_c, &_section_end_c, T_NORMAL);
  TEST_FETCH(buffer_x);

  /* ST can call T_CALLABLE */
  tagify_mem(&_section_start_c, &_section_end_c, T_CALLABLE);
  TEST_FETCH(buffer_x);

  /* ST cannot call T_UTRUSTED */
  tagify_mem(&_section_start_c, &_section_end_c, T_UTRUSTED);
  TEST_FETCH_TAGFAULT(buffer_x);

  /* ST can call T_STRUSTED */
  tagify_mem(&_section_start_c, &_section_end_c, T_STRUSTED);
  TEST_FETCH(buffer_x);

  /* test wrappers are set to SN mode */
  tagify_mem(&_test_start, &_test_end, T_NORMAL);
  tagify_mem(&_data_start, &_data_end, T_NORMAL);

  /* SN can call T_NORMAL */
  tagify_mem(&_section_start_c, &_section_end_c, T_NORMAL);
  CALL_NORMAL(TEST_FETCH(buffer_x));

  /* SN can call T_CALLABLE */
  tagify_mem(&_section_start_c, &_section_end_c, T_CALLABLE);
  CALL_NORMAL(TEST_FETCH(buffer_x));

  /* SN cannot call T_UTRUSTED */
  tagify_mem(&_section_start_c, &_section_end_c, T_UTRUSTED);
  CALL_NORMAL(TEST_FETCH_TAGFAULT(buffer_x));

  /* SN cannot call T_STRUSTED */
  tagify_mem(&_section_start_c, &_section_end_c, T_STRUSTED);
  CALL_NORMAL(TEST_FETCH_TAGFAULT(buffer_x));
}

static void st_access_test_trusted()
{
  TEST_SUITE("st access tests secure");
  /* Switch all test wrappers to secure mode */
  tagify_mem(&_test_start, &_test_end, T_CALLABLE); /* is executed in secure mode */

  /* Secure machine-mode can access anything */
  tagify_mem(&_section_start_a, &_section_end_a, T_NORMAL);
  TEST_LOAD(buffer_a);
  TEST_LOAD(buffer_a[BUFFER_SLOTS-1]);
  TEST_STORE(buffer_a);
  TEST_STORE(buffer_a[BUFFER_SLOTS-1]);

  tagify_mem(&_section_start_a, &_section_end_a, T_CALLABLE);
  TEST_LOAD(buffer_a);
  TEST_LOAD(buffer_a[BUFFER_SLOTS-1]);
  TEST_STORE(buffer_a);
  TEST_STORE(buffer_a[BUFFER_SLOTS-1]);

  tagify_mem(&_section_start_a, &_section_end_a, T_UTRUSTED);
  TEST_LOAD(buffer_a);
  TEST_LOAD(buffer_a[BUFFER_SLOTS-1]);
  TEST_STORE(buffer_a);
  TEST_STORE(buffer_a[BUFFER_SLOTS-1]);

  tagify_mem(&_section_start_a, &_section_end_a, T_STRUSTED);
  TEST_LOAD(buffer_a);
  TEST_LOAD(buffer_a[BUFFER_SLOTS-1]);
  TEST_STORE(buffer_a);
  TEST_STORE(buffer_a[BUFFER_SLOTS-1]);
}

__attribute__((section(".section.normal"),noinline))
static void sn_access_test_normal()
{
  printm("sn access test normal\n");
  /* Switch all test wrappers and data to normal mode */
  tagify_mem(&_test_start, &_test_end, T_NORMAL); /* is executed in secure mode */
  tagify_mem(&_data_start, &_data_end, T_NORMAL);

  /* Normal machine-mode can access T_NORMAL only */
  tagify_mem(&_section_start_a, &_section_end_a, T_NORMAL); /* is executed in secure mode */
  TEST_LOAD(buffer_a);
  TEST_LOAD(buffer_a[BUFFER_SLOTS-1]);
  TEST_STORE(buffer_a);
  TEST_STORE(buffer_a[BUFFER_SLOTS-1]);

  TEST_LTAG(buffer_a);     /* ltag should be possible */
  generic_stag_test(N);    /* stag can change to T_NORMAL */

  tagify_mem(&_section_start_a, &_section_end_a, T_CALLABLE); /* is executed in secure mode */
  TEST_LOAD_TAGFAULT(buffer_a);
  TEST_LOAD_TAGFAULT(buffer_a[BUFFER_SLOTS-1]);
  TEST_STORE_TAGFAULT(buffer_a);
  TEST_STORE_TAGFAULT(buffer_a[BUFFER_SLOTS-1]);

  TEST_LTAG(buffer_a); /* ltag should be possible */
  generic_stag_test(0);    /* stag must fail */

  tagify_mem(&_section_start_a, &_section_end_a, T_UTRUSTED); /* is executed in secure mode */
  TEST_LOAD_TAGFAULT(buffer_a);
  TEST_LOAD_TAGFAULT(buffer_a[BUFFER_SLOTS-1]);
  TEST_STORE_TAGFAULT(buffer_a);
  TEST_STORE_TAGFAULT(buffer_a[BUFFER_SLOTS-1]);

  TEST_LTAG(buffer_a); /* ltag should be possible */
  generic_stag_test(0);    /* stag must fail */

  tagify_mem(&_section_start_a, &_section_end_a, T_STRUSTED); /* is executed in secure mode */
  TEST_LOAD_TAGFAULT(buffer_a);
  TEST_LOAD_TAGFAULT(buffer_a[BUFFER_SLOTS-1]);
  TEST_STORE_TAGFAULT(buffer_a);
  TEST_STORE_TAGFAULT(buffer_a[BUFFER_SLOTS-1]);

  TEST_LTAG(buffer_a); /* ltag should be possible */
  generic_stag_test(0);    /* stag must fail */
}

static void st_access_test_normal()
{
  TEST_SUITE("access tests from normal machine code");

  CALL_NORMAL(sn_access_test_normal());
}

__attribute__((section(".section.normal"),noinline))
static void sn_pmp_update_invalidate()
{
  printm("sn pmp update invalidate\n");
  TEST_VERIFY_PMP(s, 0, &_section_start_a, &_section_end_a, PMP_ACCESS_F);
  TEST_VERIFY_PMP(s, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_F);
  TEST_VERIFY_PMP(s, 2, &_section_start_a, &_section_end_a, PMP_ACCESS_F);
  TEST_VERIFY_PMP(s, 3, &_section_start_b, &_section_end_b, PMP_ACCESS_F);

  /* Any write to PMP from SN mode shall invalidate T-flag */
  SET_PMP(s, 0, &_section_start_a, &_section_end_a, PMP_ACCESS_F);
  SET_PMP(s, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_R | PMP_ACCESS_TA);
  SET_PMP(s, 2, &_section_start_a, &_section_end_a, 0);
  TEST_VERIFY_PMP(s, 0, &_section_start_a, &_section_end_a, PMP_ACCESS_F & ~PMP_ACCESS_A);
  TEST_VERIFY_PMP(s, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_R | PMP_ACCESS_T);
  TEST_VERIFY_PMP(s, 2, &_section_start_a, &_section_end_a, 0);
  TEST_VERIFY_PMP(s, 3, &_section_start_b, &_section_end_b, PMP_ACCESS_F);
}

#define TTCB_SIZE 128
__attribute__((section(".section.d"),aligned(0x1000))) uintptr_t ttcb[TTCB_SIZE];

__attribute__((section(".section.normal"),noinline))
static void sn_ttcb_update()
{
  TEST_STTCB_FAULT(ttcb);
}

static void st_pmp_test_ttcb()
{
  TEST_SUITE("st pmp ttcb test");
  /* Switch all test wrappers to secure mode */
  tagify_mem(&_test_start, &_test_end, T_CALLABLE);

  /* Tag ttcb as strusted, should not make any difference */
  tagify_mem(&ttcb, &ttcb[TTCB_SIZE-1], T_STRUSTED);

  /* Setting TTCB from secure mode shall succeed */
  TEST_STTCB(ttcb);

  /* Switch all test wrappers to normal mode */
  tagify_mem(&_test_start, &_test_end, T_NORMAL);
  tagify_mem(&_data_start, &_data_end, T_NORMAL);

  /* Setting TTCB from normal mode shall fail */
  CALL_NORMAL(sn_ttcb_update());
}

static void st_pmp_test_ack()
{
  TEST_SUITE("st pmp ack test");

  SET_PMP(s, 0, &_section_start_a, &_section_end_a, PMP_ACCESS_F);
  SET_PMP(s, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_F);
  SET_PMP(s, 2, &_section_start_a, &_section_end_a, PMP_ACCESS_F);
  SET_PMP(s, 3, &_section_start_b, &_section_end_b, PMP_ACCESS_F);
  DUMP_PMP(s);

  CALL_NORMAL(sn_pmp_update_invalidate());
}

//###########################################
// USER TESTS
//###########################################

int tag_mask = 0;

__attribute__((section(".test"),noinline))
static void u_stag_test() {
  generic_stag_test(tag_mask);
  USERTEST_EXIT();
}

int access_mask = 0;
int expect_tagfault = 0;

/*
 * Requires globals to be set:
 * access_mask ... bitmask of R|W|X specifying which tests shall pass
 * expect_tagfault ... shall be != zero if a tag fault is expected instead
 * of, e.g. a load fault
 */
__attribute__((section(".test"),noinline))
static void u_access_test_generic()
{
  uintptr_t res;
  if (access_mask & R) {
    res = TEST_SUCCESS;
  } else {
    res = expect_tagfault ? CAUSE_TAG_MISMATCH : CAUSE_LOAD_ACCESS;
  }
  TEST_LOAD_RES(res, buffer_a);
  TEST_LOAD_RES(res, buffer_a[BUFFER_SLOTS-1]);
  TEST_LOAD_RES(res, buffer_b);
  TEST_LOAD_RES(res, buffer_b[BUFFER_SLOTS-1]);

  if (access_mask & W) {
    res = TEST_SUCCESS;
  } else {
    res = expect_tagfault ? CAUSE_TAG_MISMATCH : CAUSE_STORE_ACCESS;
  }
  TEST_STORE_RES(res, buffer_a);
  TEST_STORE_RES(res, buffer_a[BUFFER_SLOTS-1]);
  TEST_STORE_RES(res, buffer_b);
  TEST_STORE_RES(res, buffer_b[BUFFER_SLOTS-1]);

  if (access_mask & X) {
    res = TEST_SUCCESS;
  } else {
    res = expect_tagfault ? CAUSE_TAG_MISMATCH : CAUSE_FETCH_ACCESS;
  }
  TEST_FETCH_RES(res, buffer_x);

  USERTEST_EXIT();
}

__attribute__((section(".section.trusted"),noinline))
void switch_usermode(void (*fn)(uintptr_t))
{
  /* To be called from S-mode
   * This gives user access to kernel test wrappers and printm.
   * The .section.trusted must be tagged as callable and must not be PMP mapped,
   * otherwise this function cannot be executed from S-mode
   */

  /* Map kernel for UT (except this function).
   * Note! kernel becomes inaccessible to S-mode.
   * The next thing we need to do is either change to usermode (what we do here)
   * or unmap kernel again
   */
  SET_PMP(s, 0, &_global_start, &_global_end, PMP_ACCESS_F);

  /* our user stack is placed in the kernel, so it is already available */
  uintptr_t sstatus = read_csr(sstatus);
  sstatus = INSERT_FIELD(sstatus, SSTATUS_SPP, PRV_U);
  sstatus = INSERT_FIELD(sstatus, MSTATUS_SPIE, 0);
  write_csr(sstatus, sstatus);
  write_csr(sscratch, sstack);
  write_csr(sepc, fn);
  asm volatile ("mv a0, %0; mv sp, %0; sret" : : "r" ((uintptr_t)&ustack[USTACK_SLOTS-1]));
  __builtin_unreachable();
}

void s_usertest_main(int test)
{
  printm("usertest main, executing test %d\n", test);
  switch(test) {
    case 0: //UT
    {
      TEST_SUITE("usertest UT->invalid ack");
      /* Allow access to testing framework */
      /* Normal mode can call secure machine-code (T_CALLABLE) */
      /* and access all machine data except stack */
      tagify_mem(&_global_start, &_global_end, T_CALLABLE);
      tagify_mem(&_data_start, &_data_end, T_UTRUSTED);

      /* Switch all test wrappers to secure mode */
      tagify_mem(&_test_start, &_test_end, T_CALLABLE);

      /* .section.a-c are not fetchable since either !T or !ACK */
      /* However, they can be read/written, which does not require T|ACK */
      SET_PMP(s, 1, &_section_start_a, &_section_end_a, PMP_ACCESS_RWX | PMP_ACCESS_T);
      SET_PMP(s, 2, &_section_start_b, &_section_end_b, PMP_ACCESS_RWX | PMP_ACCESS_A);
      SET_PMP(s, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_RWX | PMP_ACCESS_T);

      tagify_mem(&_section_start_a, &_section_end_a, T_UTRUSTED);
      tagify_mem(&_section_start_b, &_section_end_b, T_UTRUSTED);
      tagify_mem(&_section_start_c, &_section_end_c, T_UTRUSTED);

      access_mask = R|W;
      expect_tagfault = 0;
      switch_usermode(u_access_test_generic);
      break;
    }
    case 1: //UT, builds upon 0
    {
      TEST_SUITE("usertest UT->UTRUSTED");
      /* .section.a-c are accessible due to T and ACK */
      SET_PMP(s, 1, &_section_start_a, &_section_end_a, PMP_ACCESS_RWX | PMP_ACCESS_TA);
      SET_PMP(s, 2, &_section_start_b, &_section_end_b, PMP_ACCESS_RWX | PMP_ACCESS_TA);
      SET_PMP(s, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_RWX | PMP_ACCESS_TA);

      access_mask = R|W|X;
      expect_tagfault = 1;
      switch_usermode(u_access_test_generic);
      break;
    }
    case 2: //UT, builds upon 1
    {
      TEST_SUITE("usertest UT->MTRUSTED");
      /* .section.a-c not accessible due to T_STRUSTED */
      tagify_mem(&_section_start_a, &_section_end_a, T_STRUSTED);
      tagify_mem(&_section_start_b, &_section_end_b, T_STRUSTED);
      tagify_mem(&_section_start_c, &_section_end_c, T_STRUSTED);

      access_mask = 0;
      expect_tagfault = 1;
      switch_usermode(u_access_test_generic);
      break;
    }
    case 3: //UT, builds upon 1
    {
      TEST_SUITE("usertest UT->NORMAL");
      /* .section.a-c accessible due to T_NORMAL */
      tagify_mem(&_section_start_a, &_section_end_a, T_NORMAL);
      tagify_mem(&_section_start_b, &_section_end_b, T_NORMAL);
      tagify_mem(&_section_start_c, &_section_end_c, T_NORMAL);

      st_set_ststatus(TSTATUS_UE); /* Enable UT to run */
      ASSERT(read_csr(ststatus) & (TSTATUS_UE | TSTATUS_EN), "TSTATUS configuration error\n!");

      access_mask = R|W|X;
      expect_tagfault = 1;
      switch_usermode(u_access_test_generic);
      break;
    }
    case 4: //UT, builds upon 1
    {
      TEST_SUITE("usertest UT->CALLABLE");
      /* .section.a-c only rx accessible due to T_CALLABLE */
      tagify_mem(&_section_start_a, &_section_end_a, T_CALLABLE);
      tagify_mem(&_section_start_b, &_section_end_b, T_CALLABLE);
      tagify_mem(&_section_start_c, &_section_end_c, T_CALLABLE);

      access_mask = R|X;
      expect_tagfault = 1;
      switch_usermode(u_access_test_generic);
      break;
    }
    case 5: //UT
    {
      TEST_SUITE("usertest UT->lstag on NORMAL");
      tagify_mem(&_section_start_a, &_section_end_a, T_NORMAL);
      tag_mask = N | UT;
      switch_usermode(u_stag_test);
      break;
    }
    case 6: //UT
    {
      TEST_SUITE("usertest UT->lstag on CALLABLE");
      tagify_mem(&_section_start_a, &_section_end_a, T_CALLABLE);
      tag_mask = 0;
      switch_usermode(u_stag_test);
      break;
    }
    case 7: //UT
    {
      TEST_SUITE("usertest UT->lstag on UTRUSTED");
      tagify_mem(&_section_start_a, &_section_end_a, T_UTRUSTED);
      tag_mask = N | UT;
      switch_usermode(u_stag_test);
      break;
    }
    case 8: //UT
    {
      TEST_SUITE("usertest UT->lstag on MTRUSTED");
      tagify_mem(&_section_start_a, &_section_end_a, T_STRUSTED);
      tag_mask = 0;
      switch_usermode(u_stag_test);
      break;
    }
    case 9: //UN
    {
      TEST_SUITE("usertest UN->NORMAL");
      /* Switch all test wrappers and data to normal mode */
      tagify_mem(&_test_start, &_test_end, T_NORMAL);
      tagify_mem(&_data_start, &_data_end, T_NORMAL);

      /* UN does not care about T or ACK */
      SET_PMP(s, 1, &_section_start_a, &_section_end_a, PMP_ACCESS_RWX | PMP_ACCESS_T);
      SET_PMP(s, 2, &_section_start_b, &_section_end_b, PMP_ACCESS_RWX | PMP_ACCESS_A);
      SET_PMP(s, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_RWX | PMP_ACCESS_T);

       /* .section.a-c accessible due to T_NORMAL */
      tagify_mem(&_section_start_a, &_section_end_a, T_NORMAL);
      tagify_mem(&_section_start_b, &_section_end_b, T_NORMAL);
      tagify_mem(&_section_start_c, &_section_end_c, T_NORMAL);

      access_mask = R|W|X;
      expect_tagfault = 1;
      switch_usermode(u_access_test_generic);
      break;
    }
    case 10: //UN
    {
      TEST_SUITE("usertest UN->UTRUSTED");
      /* TRUSTED and CALLABLE need T and ACK */
      SET_PMP(s, 1, &_section_start_a, &_section_end_a, PMP_ACCESS_RWX | PMP_ACCESS_TA);
      SET_PMP(s, 2, &_section_start_b, &_section_end_b, PMP_ACCESS_RWX | PMP_ACCESS_TA);
      SET_PMP(s, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_RWX | PMP_ACCESS_TA);

       /* .section.a-c not accessible due to T_UTRUSTED */
      tagify_mem(&_section_start_a, &_section_end_a, T_UTRUSTED);
      tagify_mem(&_section_start_b, &_section_end_b, T_UTRUSTED);
      tagify_mem(&_section_start_c, &_section_end_c, T_UTRUSTED);

      access_mask = 0;
      expect_tagfault = 1;
      switch_usermode(u_access_test_generic);
      break;
    }
    case 11: //UN, builds upon 10
    {
      TEST_SUITE("usertest UN->MTRUSTED");
       /* .section.a-c blocked due to T_STRUSTED */
      tagify_mem(&_section_start_a, &_section_end_a, T_STRUSTED);
      tagify_mem(&_section_start_b, &_section_end_b, T_STRUSTED);
      tagify_mem(&_section_start_c, &_section_end_c, T_STRUSTED);

      access_mask = 0;
      expect_tagfault = 1;
      switch_usermode(u_access_test_generic);
      break;
    }
    case 12: //UN, builds upon 10
    {
      TEST_SUITE("usertest UN->CALLABLE");
       /* .section.a-c only x accessible due to T_CALLABLE */
      tagify_mem(&_section_start_a, &_section_end_a, T_CALLABLE);
      tagify_mem(&_section_start_b, &_section_end_b, T_CALLABLE);
      tagify_mem(&_section_start_c, &_section_end_c, T_CALLABLE);

      access_mask = X;
      expect_tagfault = 1;
      switch_usermode(u_access_test_generic);
      break;
    }
    case 13: //UN
    {
      TEST_SUITE("usertest UN->lstag on NORMAL");
      tagify_mem(&_section_start_a, &_section_end_a, T_NORMAL);
      tag_mask = N;
      switch_usermode(u_stag_test);
      break;
    }
    case 14: //UN
    {
      TEST_SUITE("usertest UN->lstag on CALLABLE");
      tagify_mem(&_section_start_a, &_section_end_a, T_CALLABLE);
      tag_mask = 0;
      switch_usermode(u_stag_test);
      break;
    }
    case 15: //UN
    {
      TEST_SUITE("usertest UN->lstag on UTRUSTED");
      tagify_mem(&_section_start_a, &_section_end_a, T_UTRUSTED);
      tag_mask = 0;
      switch_usermode(u_stag_test);
      break;
    }
    case 16: //UN
    {
      TEST_SUITE("usertest UN->lstag on MTRUSTED");
      tagify_mem(&_section_start_a, &_section_end_a, T_STRUSTED);
      tag_mask = 0;
      switch_usermode(u_stag_test);
      break;
    }
    case 17: //UN
    {
      TEST_SUITE("usertest UN->CALLABLE interrupted");

      SET_PMP(s, 1, &_section_start_a, &_section_end_a, PMP_ACCESS_RWX | PMP_ACCESS_TA);
      SET_PMP(s, 2, &_section_start_b, &_section_end_b, PMP_ACCESS_RWX | PMP_ACCESS_TA);
      SET_PMP(s, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_RWX | PMP_ACCESS_TA);

      /* Change whole kernel to normal --> we proceed in SN.
       * Since we mark UT as interrupted, it cannot execute kernel in UT mode and would crash
       */
      tagify_mem(&_global_start, &_global_end, T_NORMAL);
      tagify_mem(&_section_start_a, &_section_end_a, T_NORMAL);
      tagify_mem(&_section_start_b, &_section_end_b, T_NORMAL);
      tagify_mem(&_section_start_c, &_section_end_c, T_CALLABLE);

      st_set_ststatus(TSTATUS_UE | TSTATUS_UI); /* Mark UT as interrupted. It shall not execute */
      ASSERT(read_csr(ststatus) & (TSTATUS_UE | TSTATUS_UI | TSTATUS_EN), "TSTATUS configuration error\n!");

      access_mask = R|W;
      expect_tagfault = 1;
      switch_usermode(u_access_test_generic);
      break;
    }
    case 18: //UN, builds upon 17
    {
      TEST_SUITE("usertest UN->CALLABLE disabled");

      st_clear_ststatus(TSTATUS_UE | TSTATUS_UI); /* Mark UT as disabled. It shall not execute */
      ASSERT((read_csr(ststatus) & (TSTATUS_UE | TSTATUS_UI | TSTATUS_EN)) == TSTATUS_EN, "TSTATUS configuration error\n!");

      access_mask = R|W;
      expect_tagfault = 1;
      switch_usermode(u_access_test_generic);
      break;
    }
    default:
      SUCCESS();
  }
  ASSERT(false, "Should not reach here!");
}

int test_tmp;
void s_usertest_trampoline() {
  s_usertest_main(test_tmp);
}

/**
 * Re-entry point for finished user tests
 * @param test contains increasing number
 */
__attribute__((section(".section.normal"),noinline))
void m_usertest_main(int test)
{
  /* must be always in normal mode */
  /* Clear pmp[0] to unmap kernel from UT
   * This allows ST to use kernel code again
   */
  CLEAR_PMP(s, 0);
  test_tmp = test;
  ENTER_S(s_usertest_trampoline);
}

void s_supervisor_main()
{
  st_modeswitch_sn();
  st_pmp_test_ttcb();
  st_pmp_test_ack();

  st_access_test_trusted();
  st_access_test_normal();

  s_usertest_main(0); // calls SUCCESS();
}

void m_supervisor_main()
{
  CLEAR_PMP(m, 0);
  CLEAR_PMP(m, 1);
  CLEAR_PMP(m, 2);
  CLEAR_PMP(m, 3);
  CLEAR_PMP(m, 4);
  CLEAR_PMP(m, 5);
  CLEAR_PMP(m, 6);
  CLEAR_PMP(m, 7);
  /* Allow ST full fetch access */
  SET_PMP(m, 7, (void*)0, (void*)((uintptr_t)(-1)), PMP_ACCESS_X | PMP_ACCESS_ST);
  m_init_test(); /* This one is required to set up TSTATUS */
  ENTER_S(s_supervisor_main);
}

static void m_prepare_tags()
{
  tagify_mem(&_global_start, &_global_end, T_CALLABLE);
  tagify_mem(&_section_start_trusted, &_section_end_trusted, T_CALLABLE);
  tagify_mem(&_section_start_normal, &_section_end_normal, T_NORMAL);
  /* Make stacks & co accessible from normal and trusted mode */
  tagify_mem(ustack, &ustack[USTACK_SLOTS], T_NORMAL);
  tagify_mem(sstack, &sstack[SSTACK_SLOTS], T_NORMAL);
  tagify_mem(&_test_start, &_test_end, T_NORMAL);
  tagify_mem(&_data_start, &_data_end, T_NORMAL);
}

void m_test_main() {
  /* Run machine-mode tests */
  m_basic_lstag_test(); /* should be first since it checks for default tags */
  m_basic_lc_test();
  m_basic_sct_test();
  m_prepare_tags();
  m_supervisor_main();
}
