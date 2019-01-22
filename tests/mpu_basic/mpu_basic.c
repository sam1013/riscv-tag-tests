/* Copyright (c) Samuel Weiser */
/* See LICENSE for license details. */

#include "mpu_basic.h"
#include "tag.h"

#define BUFFER_SLOTS 8
__attribute__((section(".section.a"))) volatile uintptr_t buffer_r[BUFFER_SLOTS];
__attribute__((section(".section.b"))) volatile uintptr_t buffer_w[BUFFER_SLOTS];

__attribute__((section(".section.c"),noinline)) void buffer_x() {
  asm volatile("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n ret\n");
}

uintptr_t unstack[USTACK_SLOTS];

static void m_init_test()
{
  TEST_SUITE("init test");
  set_csr(mtstatus, TSTATUS_EN);

  //TODO: extend test
}

static void m_access_test()
{
  TEST_SUITE("machine access test");
  CLEAR_PMP(m, 0);
  CLEAR_PMP(m, 1);
  CLEAR_PMP(m, 2);
  CLEAR_PMP(m, 3);
  CLEAR_PMP(m, 4);
  CLEAR_PMP(m, 5);
  CLEAR_PMP(m, 6);
  CLEAR_PMP(m, 7);
  DUMP_PMP(m);

  /* Machine mode bypasses all PMP checks */
  TEST_LOAD(buffer_r);
  TEST_LOAD(buffer_r[BUFFER_SLOTS-1]);
  TEST_STORE(buffer_w);
  TEST_STORE(buffer_w[BUFFER_SLOTS-1]);
  TEST_FETCH(buffer_x);

  /* Machine mode bypasses all PMP checks */
  SET_PMP(m, 0, &_section_start_a, &_section_end_a, 0);
  SET_PMP(m, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_R);
  SET_PMP(m, 2, &_section_start_c, &_section_end_c, PMP_ACCESS_W);
  SET_PMP(m, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_X);

  TEST_LOAD(buffer_r);
  TEST_LOAD(buffer_r[BUFFER_SLOTS-1]);
  TEST_STORE(buffer_w);
  TEST_STORE(buffer_w[BUFFER_SLOTS-1]);
  TEST_FETCH(buffer_x);

  /* Machine mode bypasses all PMP checks */
  SET_PMP(m, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_A);
  TEST_FETCH(buffer_x);
  SET_PMP(m, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_T);
  TEST_FETCH(buffer_x);
  SET_PMP(m, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_F);
  TEST_FETCH(buffer_x);
}

static void m_pmp_test()
{
  TEST_SUITE("machine pmp test");
  /* M can write all flags */
  SET_PMP(m, 0, &_section_start_a, &_section_end_a, 0);
  SET_PMP(m, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_R);
  SET_PMP(m, 2, &_section_start_c, &_section_end_c, PMP_ACCESS_W);
  SET_PMP(m, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_X);

  TEST_VERIFY_PMP(m, 0, &_section_start_a, &_section_end_a, 0);
  TEST_VERIFY_PMP(m, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_R);
  TEST_VERIFY_PMP(m, 2, &_section_start_c, &_section_end_c, PMP_ACCESS_W);
  TEST_VERIFY_PMP(m, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_X);

  SET_PMP(m, 0, &_section_start_a, &_section_end_a, PMP_ACCESS_A);
  SET_PMP(m, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_T);
  SET_PMP(m, 2, &_section_start_c, &_section_end_c, PMP_ACCESS_F);
  SET_PMP(m, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_ST);

  TEST_VERIFY_PMP(m, 0, &_section_start_a, &_section_end_a, PMP_ACCESS_A);
  TEST_VERIFY_PMP(m, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_T);
  TEST_VERIFY_PMP(m, 2, &_section_start_c, &_section_end_c, PMP_ACCESS_F);
  TEST_VERIFY_PMP(m, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_ST);

  CLEAR_PMP(m, 0);
  CLEAR_PMP(m, 1);
  CLEAR_PMP(m, 2);
  CLEAR_PMP(m, 3);
}

static void s_pmp_test()
{
  TEST_SUITE("supervisor pmp test");
  /* ST can write all flags */
  SET_PMP(s, 0, &_section_start_a, &_section_end_a, 0);
  SET_PMP(s, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_R);
  SET_PMP(s, 2, &_section_start_c, &_section_end_c, PMP_ACCESS_W);
  SET_PMP(s, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_X);

  TEST_VERIFY_PMP(s, 0, &_section_start_a, &_section_end_a, 0);
  TEST_VERIFY_PMP(s, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_R);
  TEST_VERIFY_PMP(s, 2, &_section_start_c, &_section_end_c, PMP_ACCESS_W);
  TEST_VERIFY_PMP(s, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_X);

  SET_PMP(s, 0, &_section_start_a, &_section_end_a, PMP_ACCESS_A);
  SET_PMP(s, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_T);
  SET_PMP(s, 2, &_section_start_c, &_section_end_c, PMP_ACCESS_F);
  SET_PMP(s, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_ST);

  TEST_VERIFY_PMP(s, 0, &_section_start_a, &_section_end_a, PMP_ACCESS_A);
  TEST_VERIFY_PMP(s, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_T);
  TEST_VERIFY_PMP(s, 2, &_section_start_c, &_section_end_c, PMP_ACCESS_F);
  TEST_VERIFY_PMP(s, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_ST);

  CLEAR_PMP(s, 0);
  CLEAR_PMP(s, 1);
  CLEAR_PMP(s, 2);
  CLEAR_PMP(s, 3);
}

__attribute__((section(".section.trusted"),noinline))
void switch_usermode(void (*fn)(uintptr_t))
{
  /* To be called from S-mode
   * This gives user access to kernel test wrappers and printm.
   * The .section.trusted must be tagged as callable and must not be PMP mapped,
   * otherwise this function cannot be executed from S-mode
   */

  /* Make whole kernel accessible from UN */
  tagify_mem(&_global_start, &_global_end, T_NORMAL);
  /* Map kernel for UT (except this function).
   * Note! kernel becomes inaccessible to S-mode.
   * The next thing we need to do is either change to usermode (what we do here)
   * or unmap kernel again
   */
  SET_PMP(s, 6, &_global_start, &_global_end, PMP_ACCESS_F);

  /* our user stack is placed in the kernel, so it is already available */
  uintptr_t sstatus = read_csr(sstatus);
  sstatus = INSERT_FIELD(sstatus, SSTATUS_SPP, PRV_U);
  sstatus = INSERT_FIELD(sstatus, MSTATUS_SPIE, 0);
  write_csr(sstatus, sstatus);
  write_csr(sscratch, sstack);
  write_csr(sepc, fn);
  asm volatile ("mv a0, %0; mv sp, %0; sret" : : "r" ((uintptr_t)&unstack[USTACK_SLOTS-1]));
  __builtin_unreachable();
}

void u_user_test()
{
  printm("Hello from usermode\n");
  /* LOAD is only possible from buffer_r */
  TEST_LOAD(buffer_r);
  TEST_LOAD(buffer_r[BUFFER_SLOTS-1]);
  TEST_LOAD_FAULT(buffer_w);
  TEST_LOAD_FAULT(buffer_w[BUFFER_SLOTS-1]);
  TEST_LOAD_FAULT(buffer_x);

  /* STORE is only possible from buffer_w */
  TEST_STORE(buffer_w);
  TEST_STORE(buffer_w[BUFFER_SLOTS-1]);
  TEST_STORE_FAULT(buffer_r);
  TEST_STORE_FAULT(buffer_r[BUFFER_SLOTS-1]);
  TEST_STORE_FAULT(buffer_x);

  /* FETCH is only possible from buffer_x */
  TEST_FETCH(buffer_x);
  TEST_FETCH_FAULT(buffer_r);
  TEST_FETCH_FAULT(buffer_r[BUFFER_SLOTS-1]);
  TEST_FETCH_FAULT(buffer_w);
  TEST_FETCH_FAULT(buffer_w[BUFFER_SLOTS-1]);

  SUCCESS();
}

static void s_user_test()
{
  TEST_SUITE("user test");
  SET_PMP(s, 0, &_section_user_start, &_section_user_end, PMP_ACCESS_F); /* This section is currently unused because UT lives in global section */
  SET_PMP(s, 1, &_section_start_a, &_section_end_a, PMP_ACCESS_R | PMP_ACCESS_TA);
  SET_PMP(s, 2, &_section_start_b, &_section_end_b, PMP_ACCESS_W | PMP_ACCESS_TA);
  SET_PMP(s, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_X | PMP_ACCESS_TA);
  DUMP_PMP(s);

  buffer_r[0] = 0xFFFFFFFF; // invalid opcode
  buffer_w[0] = 0xFFFFFFFF; // invalid opcode

  tagify_mem(&_section_start_a, &_section_end_a, T_NORMAL);
  tagify_mem(&_section_start_b, &_section_end_b, T_NORMAL);
  tagify_mem(&_section_start_c, &_section_end_c, T_NORMAL);
  tagify_mem(&unstack[0], &unstack[USTACK_SLOTS], T_NORMAL);
  tagify_mem(&_section_start_trusted, &_section_end_trusted, T_CALLABLE);
  switch_usermode(u_user_test);
}

static void s_access_test() {
  TEST_SUITE("supervisor access test");

  /* Switch all test wrappers to normal mode */
  tagify_mem(&_test_start, &_test_end, T_NORMAL);
  tagify_mem(&_data_start, &_data_end, T_NORMAL);

  /* SN supervisor mode can read/write but not fetch configured PMP range */
  SET_PMP(s, 0, &_section_start_a, &_section_end_a, 0);
  SET_PMP(s, 1, &_section_start_b, &_section_end_b, PMP_ACCESS_R);
  SET_PMP(s, 2, &_section_start_c, &_section_end_c, PMP_ACCESS_W);
  SET_PMP(s, 3, &_section_start_c, &_section_end_c, PMP_ACCESS_X);

  /* SN bypasses configured PMP ranges */
  TEST_LOAD(buffer_r);
  TEST_LOAD(buffer_r[BUFFER_SLOTS-1]);
  TEST_STORE(buffer_w);
  TEST_STORE(buffer_w[BUFFER_SLOTS-1]);
  TEST_FETCH(buffer_x);

  /* Switch all test wrappers to trusted mode */
  tagify_mem(&_test_start, &_test_end, T_CALLABLE);

  /* ST has full access */
  tagify_mem(&_section_start_c, &_section_end_c, T_CALLABLE);
  TEST_LOAD(buffer_r);
  TEST_LOAD(buffer_r[BUFFER_SLOTS-1]);
  TEST_STORE(buffer_w);
  TEST_STORE(buffer_w[BUFFER_SLOTS-1]);
  TEST_FETCH(buffer_x);

  /* Remove ST fetch access to buffer_x */
  CLEAR_PMP(s, 7);

  TEST_LOAD(buffer_r);
  TEST_LOAD(buffer_r[BUFFER_SLOTS-1]);
  TEST_STORE(buffer_w);
  TEST_STORE(buffer_w[BUFFER_SLOTS-1]);
  TEST_FETCH_FAULT(buffer_x);

  /* Allow ST full access again */
  SET_PMP(s, 7, (void*)0, (void*)((uintptr_t)(-1)), PMP_ACCESS_X | PMP_ACCESS_ST);
  CLEAR_PMP(s, 6); /* we don't need it */
}

static void s_supervisor_test() {
  s_pmp_test();
  s_access_test();
  s_user_test(); // calls SUCCESS, does not return
}

static void m_supervisor_test()
{
  CLEAR_PMP(m, 0);
  CLEAR_PMP(m, 1);
  CLEAR_PMP(m, 2);
  CLEAR_PMP(m, 3);
  CLEAR_PMP(m, 4);
  CLEAR_PMP(m, 5);
  /* Slot 6 allows ST restricted access */
  SET_PMP(m, 6, &_global_start, &_global_end, PMP_ACCESS_X | PMP_ACCESS_ST);
  /* Slot 7 allows ST full access */
  SET_PMP(m, 7, (void*)0, (void*)((uintptr_t)(-1)), PMP_ACCESS_X | PMP_ACCESS_ST);
  DUMP_PMP(m);

  ENTER_S(s_supervisor_test); //calls user test, does not return
}

static void prepare_tags()
{
  tagify_mem(&_global_start, &_global_end, T_CALLABLE);
  /* Make stacks accessible from normal and trusted mode */
  tagify_mem(ustack, &ustack[USTACK_SLOTS], T_NORMAL);
  tagify_mem(sstack, &sstack[SSTACK_SLOTS], T_NORMAL);
  tagify_mem(&_section_start_trusted, &_section_end_trusted, T_CALLABLE);
}

void m_test_main()
{
  prepare_tags();
  m_init_test();
  m_access_test();
  m_pmp_test();

  m_supervisor_test();
}

void m_usertest_main(int test)
{
}
