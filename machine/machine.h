/* See LICENSE for license details. */

#ifndef _MACHINE_H
#define _MACHINE_H

/* Main entry function of machine mode */
void m_test_main();

/* Userspace re-entry function when calling SBI_USERTEST */
void m_usertest_main(int);

/* Generic mode change to mode 'prv' using stack 'stack'.
 * Does not return
 */
void enter_mode(uintptr_t prv, void (*fn)(uintptr_t), uintptr_t stack)
  __attribute__((noreturn));

#define ENTER_S(entry) do { enter_mode(PRV_S, entry, (uintptr_t)&sstack[SSTACK_SLOTS-1]); } while(0)

#define SSTACK_SLOTS 1024
#define USTACK_SLOTS 1024

extern uintptr_t sstack[SSTACK_SLOTS];
extern uintptr_t ustack[USTACK_SLOTS];

#if __riscv_xlen != 64
#error "riscv-tag-tests only work in 64-bit mode"
#endif

#endif
