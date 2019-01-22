/* Copyright (c) Samuel Weiser */
/* See LICENSE for license details. */

#ifndef _TEST_H
#define _TEST_H

#ifndef __ASSEMBLER__

#include "encoding.h"
#include "mtrap.h"
#include "frontend.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

extern char _global_start;
extern char _global_end;
extern char _data_start;
extern char _data_end;
extern char _section_start_a;
extern char _section_end_a;
extern char _section_start_b;
extern char _section_end_b;
extern char _section_start_c;
extern char _section_end_c;
extern char _section_start_d;
extern char _section_end_d;
extern char _section_user_start;
extern char _section_user_end;
extern char _test_start;
extern char _test_end;
extern char _section_start_trusted;
extern char _section_end_trusted;
extern char _section_start_normal;
extern char _section_end_normal;
extern char _mstack_start;
extern char _mstack_end;

extern volatile fault_t expected_fault;
extern int current_test;
extern int current_usertest;

#define TEST_SUITE(msg) do { \
  printm("Test suite %s...\n", msg); \
  current_test = 0; \
  } while(0)

#define TEST_START() do { \
    if(current_usertest) printm("[%d]", current_usertest); \
    printm("[%d] running\n", current_test); \
  } while(0)

#define TEST_END() do { \
    if(current_usertest) printm("[%d]", current_usertest); \
    printm("[%d] succeeded\n", current_test++); \
  } while(0)

#define TEST_FAIL() do { \
    if(current_usertest) printm("[%d]", current_usertest); \
    printm("[%d] failed\n", current_test); \
    shutdown(-1); \
  } while(0)

#define ASSERT(cond,msg,...) do { \
  if (!(cond)) { \
       printm(msg, ##__VA_ARGS__); \
      TEST_FAIL(); \
  }} while(0)

#define SUCCESS() do { \
    printm("SUCCESS\n"); \
    shutdown(0); \
  } while(0)

#define USERTEST_EXIT() do { \
  usertest_exit(++current_usertest); \
  } while(0)

typedef enum {
  R = 1<<0,
  W = 1<<1,
  X = 1<<2,
} access_t;


#define EPC_INSN(type)      "la   %0, access"#type"    \n" /* assume instruction of "ACCESS_xxx_ASM" faults directly. Store pc of "access" label as potential epc */

/**
 */
#if __riscv_xlen == 64
#define ASMLOAD  "ld"
#define ASMSTORE "sd"
#define ACCESS_LOAD_ASM     "ld   %0, (%3)             \n" /* read-access "ptr" */
#define ACCESS_STORE_ASM    "sd   %0, (%3)             \n" /* write-access "ptr" */
#else
#define ASMLOAD  "lw"
#define ASMSTORE "sw"
#define ACCESS_LOAD_ASM     "lw   %0, (%3)             \n" /* read-access "ptr" */
#define ACCESS_STORE_ASM    "sw   %0, (%3)             \n" /* write-access "ptr" */
#endif
#define EPC_LOAD_ASM(type)  EPC_INSN(type)
#define EPC_STORE_ASM(type) EPC_INSN(type)
#define ACCESS_FETCH_ASM    "jalr %3                   \n" /* fetch "ptr". We assume "ptr" contains a "ret" instruction to return to us. */
#define EPC_FETCH_ASM(type) "mv   %0, %3               \n" /* jump will not fault directly but at target (value). Store "ptr" as potential epc */
#define ACCESS_LTAG_ASM     "ltag %0, 0(%3)            \n" /* load tag from address "ptr" into "value" */
#define ACCESS_STAG_ASM     "stag %0, 0(%3)            \n" /* store tag "value" to address "ptr" */
#define EPC_TAG_ASM(type)   EPC_INSN(type)
#define ACCESS_MTTCB_ASM    "csrw mttcb, %3            \n" /* load "ptr" into ttcb */
#define ACCESS_STTCB_ASM    "csrw sttcb, %3            \n" /* load "ptr" into ttcb */
#define EPC_TTCB_ASM(type)  EPC_INSN(type)
#define LBC_ASM(etag)        "lbct  "#etag", %0, (%3)    \n" /* read-access "ptr" with expected etag */
#define LBUC_ASM(etag)       "lbuct "#etag", %0, (%3)    \n" /* read-access "ptr" with expected etag */
#define LHC_ASM(etag)        "lhct  "#etag", %0, (%3)    \n" /* read-access "ptr" with expected etag */
#define LHUC_ASM(etag)       "lhuct "#etag", %0, (%3)    \n" /* read-access "ptr" with expected etag */
#define LWC_ASM(etag)        "lwct  "#etag", %0, (%3)    \n" /* read-access "ptr" with expected etag */
#define LWUC_ASM(etag)       "lwuct "#etag", %0, (%3)    \n" /* read-access "ptr" with expected etag */
#define LDC_ASM(etag)        "ldct  "#etag", %0, (%3)    \n" /* read-access "ptr" with expected etag */
#define LTT_ASM(etag)        "ltt   "#etag", %0, (%3)    \n" /* load and test tag of "ptr" against expected etag */
#define SBCT_ASM(etag, ntag) "sbct  "#etag","#ntag", %0, (%3)  \n" /* write-access "ptr", new "ntag" with expected etag */
#define SHCT_ASM(etag, ntag) "shct  "#etag","#ntag", %0, (%3)  \n" /* write-access "ptr", new "ntag" with expected etag */
#define SWCT_ASM(etag, ntag) "swct  "#etag","#ntag", %0, (%3)  \n" /* write-access "ptr", new "ntag" with expected etag */
#define SDCT_ASM(etag, ntag) "sdct  "#etag","#ntag", %0, (%3)  \n" /* write-access "ptr", new "ntag" with expected etag */

/**
 * Used in void test_fault_##type(uintptr_t ptr, uintptr_t value, uintptr_t ecause, size_t line)
 *
 * Parameters:
 * type                must be a unique identifier for each invocation, e.g. (load, store, fetch, ltag, stag...)
 *                     It is used to create unique goto labels.
 * access_function_asm The core functionality which is tested.
 *                     This is an assembly sequence which can make use of
 *                     - parameter 'value' in register %0 for passing values and storing unused results
 *                     - temporary 'base' in register %1 for storing unused results
 *                     - parameter 'ptr' in register %3 for passing and returning values
 * epc_function_asm    Assembly sequence for storing (potential) faulting address of access_function_asm in register %0
 */
#define TEST_FAULT_ASM(type, access_function_asm, epc_function_asm, ptr, value) \
    register uintptr_t target=(value); \
    register uintptr_t base; \
    uintptr_t ptrcopy = ptr; \
    asm volatile("addi  sp, sp, -8           \n" /* push target onto stack */ \
                 ASMSTORE" %0, 0(sp)            \n" /* push target onto stack */ \
                 epc_function_asm(type)          \
                 "la    %1, expected_fault   \n" /* in expected_fault.epc */ \
                 ASMSTORE" %0, (%1)             \n" \
                 "la    %0, fault"#type"     \n" /* store pc of "fault" handler in */ \
                 "addi  %1, %1, %4           \n" /* resume_pc */ \
                 ASMSTORE" %0, (%1)             \n" \
                 ASMLOAD"  %0, 0(sp)            \n" /* pop target from stack */ \
                 "addi  sp, sp, +8           \n" /* pop target from stack */ \
                 "access"#type":             \n" \
                 access_function_asm             \
                 "j     end"#type"           \n" \
                 "fault"#type":              \n" /* fault handler increments "isfault" */ \
                 "addi  %2, %2, 1            \n" \
                 "end"#type": nop            \n" \
               : "+r"(target), "=r"(base), "+r"(isfault), "+r"(ptrcopy) \
               : "X" (sizeof(uintptr_t) * 2) \
               : "memory");

#define GEN_TEST_ACCESS_FAULT(type, access_func, epc_func) \
void test_fault_##type(uintptr_t ptr, uintptr_t value, uintptr_t ecause, size_t line) \
{\
  TEST_START(); \
  printm("Accessing %p\n", ptr); \
  uintptr_t isfault = 0; \
  expected_fault.cause = (ecause); \
  TEST_FAULT_ASM(type, access_func, epc_func, ptr, value); \
  ASSERT( ((ecause)==((uintptr_t)-1)) ^ isfault, "Wrong fault behavior at line %d\n", line); \
  TEST_END(); \
}

#define DYN_TEST(type, access_func, epc_func, ptr, value, ecause) ({ \
  TEST_START(); \
  printm("Accessing %p\n", ptr); \
  uintptr_t isfault = 0; \
  expected_fault.cause = (ecause); \
  TEST_FAULT_ASM(type, access_func, epc_func, ptr, value); \
  ASSERT( ((ecause)==((uintptr_t)-1)) ^ isfault, "Wrong fault behavior at line %d\n", type); \
  TEST_END(); \
  target; \
})

void test_fault_load(uintptr_t ptr, uintptr_t value, uintptr_t ecause, size_t line);
void test_fault_store(uintptr_t ptr, uintptr_t value, uintptr_t ecause, size_t line);
void test_fault_fetch(uintptr_t ptr, uintptr_t value, uintptr_t ecause, size_t line);
void test_fault_ltag(uintptr_t ptr, uintptr_t value, uintptr_t ecause, size_t line);
void test_fault_stag(uintptr_t ptr, uintptr_t value, uintptr_t ecause, size_t line);
void test_fault_mttcb(uintptr_t ptr, uintptr_t value, uintptr_t ecause, size_t line);
void test_fault_sttcb(uintptr_t ptr, uintptr_t value, uintptr_t ecause, size_t line);
void test_fault_lbc(uintptr_t ptr, uintptr_t value, uintptr_t ecause, size_t line);

#define TEST_LOAD(buf)  test_fault_load(((uintptr_t)&(buf)), -1, (uintptr_t)-1, __LINE__)
#define TEST_STORE(buf) test_fault_store(((uintptr_t)&(buf)), -1, (uintptr_t)-1, __LINE__)
#define TEST_FETCH(buf) test_fault_fetch(((uintptr_t)&(buf)), -1, (uintptr_t)-1, __LINE__)
#define TEST_LTAG(buf) test_fault_ltag(((uintptr_t)&(buf)), -1, (uintptr_t)-1, __LINE__)
#define TEST_STAG(buf, tag) test_fault_stag(((uintptr_t)&(buf)), (tag), (uintptr_t)-1, __LINE__)
#define TEST_MTTCB(buf) test_fault_mttcb(((uintptr_t)&(buf)), -1, (uintptr_t)-1, __LINE__)
#define TEST_STTCB(buf) test_fault_sttcb(((uintptr_t)&(buf)), -1, (uintptr_t)-1, __LINE__)
#define TEST_LOAD_FAULT(buf)   test_fault_load(((uintptr_t)&(buf)), -1, CAUSE_LOAD_ACCESS, __LINE__)
#define TEST_LOAD_TAGFAULT(buf)   test_fault_load(((uintptr_t)&(buf)), -1, CAUSE_TAG_MISMATCH, __LINE__)
#define TEST_STORE_FAULT(buf)  test_fault_store(((uintptr_t)&(buf)), -1, CAUSE_STORE_ACCESS, __LINE__)
#define TEST_STORE_TAGFAULT(buf)  test_fault_store(((uintptr_t)&(buf)), -1, CAUSE_TAG_MISMATCH, __LINE__)
#define TEST_FETCH_FAULT(buf)  test_fault_fetch(((uintptr_t)&(buf)), -1, CAUSE_FETCH_ACCESS, __LINE__)
#define TEST_FETCH_TAGFAULT(buf)  test_fault_fetch(((uintptr_t)&(buf)), -1, CAUSE_TAG_MISMATCH, __LINE__)
#define TEST_LTAG_FAULT(buf) test_fault_ltag(((uintptr_t)&(buf)), -1, CAUSE_ILLEGAL_INSTRUCTION, __LINE__)
#define TEST_STAG_FAULT(buf, tag) test_fault_stag(((uintptr_t)&(buf)), (tag), CAUSE_ILLEGAL_INSTRUCTION, __LINE__)
#define TEST_MTTCB_FAULT(buf) test_fault_mttcb(((uintptr_t)&(buf)), -1, CAUSE_ILLEGAL_INSTRUCTION, __LINE__)
#define TEST_STTCB_FAULT(buf) test_fault_sttcb(((uintptr_t)&(buf)), -1, CAUSE_ILLEGAL_INSTRUCTION, __LINE__)

/* Wrappers which allow to dynamically specify expected fault
 * Use TEST_SUCCESS if you don't expect a fault
 */
#define TEST_SUCCESS -1
#define TEST_LOAD_RES(res,buf)  test_fault_load(((uintptr_t)&(buf)), -1, (uintptr_t)(res), __LINE__)
#define TEST_STORE_RES(res,buf) test_fault_store(((uintptr_t)&(buf)), -1, (uintptr_t)(res), __LINE__)
#define TEST_FETCH_RES(res,buf) test_fault_fetch(((uintptr_t)&(buf)), -1, (uintptr_t)(res), __LINE__)
#define TEST_LTAG_RES(res,buf) test_fault_ltag(((uintptr_t)&(buf)), -1, (uintptr_t)(res), __LINE__)
#define TEST_STAG_RES(res,buf, tag) test_fault_stag(((uintptr_t)&(buf)), (tag), (uintptr_t)(res), __LINE__)
#define TEST_MTTCB_RES(res,buf) test_fault_mttcb(((uintptr_t)&(buf)), -1, (uintptr_t)(res), __LINE__)
#define TEST_STTCB_RES(res,buf) test_fault_sttcb(((uintptr_t)&(buf)), -1, (uintptr_t)(res), __LINE__)

#define TEST_VERIFY_TAG(start, end, tag) \
  do { \
    TEST_START(); \
    ASSERT(verify_tag(start, end, tag), "Wrong tag!\n"); \
    TEST_END(); \
  } while(0)

#define TEST_VERIFY_PMP(m, idx, ebase, ebound, eflags) \
  do { \
    TEST_START(); \
    pmp_entry_t curr; \
    READ_PMP(m, idx, &curr); \
    ASSERT((curr.base == ((uintptr_t)ebase)), "Wrong PMP base %x, expected %x\n", curr.base, ((uintptr_t)ebase)); \
    ASSERT((curr.bound == ((uintptr_t)ebound)), "Wrong PMP bound %x, expected %x\n", curr.bound, ((uintptr_t)ebound)); \
    ASSERT((curr.flags == ((uintptr_t)eflags)), "Wrong PMP flags %x, expected %x\n", curr.flags, ((uintptr_t)eflags)); \
    TEST_END(); \
  } while(0)

#endif // !__ASSEMBLER__

#endif
