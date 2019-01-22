// See LICENSE for license details.

#ifndef _USER_H
#define _section_H

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

extern volatile fault_t expected_fault;
extern int current_test;

#define TEST_SUITE(msg) do { \
  printm("Test suite %s...\n", msg); \
  current_test = 0; \
  } while(0)

#define TEST_START() do { \
    printm("[%d] running\n", current_test); \
  } while(0)

#define TEST_END() do { \
    printm("[%d] succeeded\n", current_test++); \
  } while(0)

#define TEST_FAIL() do { \
    printm("Test %d failed\n", current_test); \
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

#define ACCESS_LOAD_ASM     "ld   %0, (%3)             \n" /* read-access "data" */
#define EPC_LOAD_ASM(type)  "la   %0, access"#type"    \n" /* store pc of "access" label as potential epc */ 
#define ACCESS_STORE_ASM    "sd   %0, (%3)             \n" /* write-access "data" */
#define EPC_STORE_ASM(type) "la   %0, access"#type"    \n" /* store pc of "access" label as potential epc */ 
#define ACCESS_FETCH_ASM    "jalr %3                   \n" /* fetch "data". We assume "data" contains a "ret" instruction to return to us. */
#define EPC_FETCH_ASM(type) "mv   %0, %3               \n" /* store "data" as potential epc */
#define ACCESS_LTAG_ASM     "ltag %0, 0(%3)            \n" /* load tag from address "data" into "target" */
#define ACCESS_STAG_ASM     "stag x0, 0(%3)            \n" /* store tag 0 to address "data" */
#define EPC_TAG_ASM(type)   "la   %0, access"#type"    \n" /* store pc of "access" label as potential epc */ 

/**
 * type                is one of (load, store, fetch)
 * access_function_asm accesses variable "data" (register %3) either via read, write or fetch
 * epc_function_asm    stores potential faulting address of access_function_asm in register %0
 */
#define TEST_FAULT_ASM(type, access_function_asm, epc_function_asm) \
    register uintptr_t target; \
    register uintptr_t base; \
    asm volatile(epc_function_asm(type)         \
                 "la   %1, expected_fault   \n" /* in expected_fault.epc */ \
                 "sd   %0, (%1)             \n" \
                 "la   %0, fault"#type"     \n" /* store pc of "fault" handler in */ \
                 "addi %1, %1, %4           \n" /* resume_pc */ \
                 "sd   %0, (%1)             \n" \
                 "access"#type":            \n" \
                 access_function_asm            \
                 "j    end"#type"           \n" \
                 "fault"#type":             \n" /* fault handler increments "isfault" */ \
                 "addi %2, %2, 1            \n" \
                 "end"#type": nop           \n" \
               : "=r"(target), "=r"(base), "+r"(isfault), "+r"(data) \
               : "X" (sizeof(uintptr_t) * 2) \
               : "memory");

#define GEN_TEST_ACCESS_FAULT(type, access_func, epc_func) \
void test_fault_##type(uintptr_t data, uintptr_t ecause, size_t line) \
{\
  TEST_START(); \
  printm("Accessing %p\n", data); \
  uintptr_t isfault = 0; \
  expected_fault.cause = (ecause); \
  TEST_FAULT_ASM(type, access_func, epc_func); \
  ASSERT( ((ecause)==((uintptr_t)-1)) ^ isfault, "Wrong fault behavior at line %zu\n", line); \
  TEST_END(); \
}

void test_fault_load(uintptr_t data, uintptr_t ecause, size_t line);
void test_fault_store(uintptr_t data, uintptr_t ecause, size_t line);
void test_fault_fetch(uintptr_t data, uintptr_t ecause, size_t line);
void test_fault_ltag(uintptr_t data, uintptr_t ecause, size_t line);
void test_fault_stag(uintptr_t data, uintptr_t ecause, size_t line);
 
#define TEST_LOAD(buf)  test_fault_load(((uintptr_t)&buf), (uintptr_t)-1, __LINE__)
#define TEST_STORE(buf) test_fault_store(((uintptr_t)&buf), (uintptr_t)-1, __LINE__)
#define TEST_FETCH(buf) test_fault_fetch(((uintptr_t)&buf), (uintptr_t)-1, __LINE__)
#define TEST_LTAG(buf) test_fault_ltag(((uintptr_t)&buf), (uintptr_t)-1, __LINE__)
#define TEST_STAG(buf) test_fault_stag(((uintptr_t)&buf), (uintptr_t)-1, __LINE__)
#define TEST_LOAD_FAULT(buf)   test_fault_load(((uintptr_t)&(buf)), CAUSE_FAULT_LOAD, __LINE__)
#define TEST_STORE_FAULT(buf)  test_fault_store(((uintptr_t)&(buf)), CAUSE_FAULT_STORE, __LINE__)
#define TEST_FETCH_FAULT(buf)  test_fault_fetch(((uintptr_t)&(buf)), CAUSE_FAULT_FETCH, __LINE__)
#define TEST_LTAG_FAULT(buf) test_fault_ltag(((uintptr_t)&buf), CAUSE_ILLEGAL_INSTRUCTION, __LINE__)
#define TEST_STAG_FAULT(buf) test_fault_stag(((uintptr_t)&buf), CAUSE_ILLEGAL_INSTRUCTION, __LINE__)

#define TEST_VERIFY_TAG(start, end, tag) \
  do { \
    TEST_START(); \
    ASSERT(verify_tag(start, end, tag), "Wrong tag!\n"); \
    TEST_END(); \
  } while(0);

#endif // !__ASSEMBLER__

#endif
