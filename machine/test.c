/* Copyright (c) Samuel Weiser */
/* See LICENSE for license details. */

#include "test.h"
#include "mtrap.h"
#include "frontend.h"
#include <stdbool.h>

int current_test = 0;
int current_usertest = 0;

__attribute__((section(".test"))) 
GEN_TEST_ACCESS_FAULT(load, ACCESS_LOAD_ASM, EPC_LOAD_ASM)
__attribute__((section(".test"))) 
GEN_TEST_ACCESS_FAULT(store, ACCESS_STORE_ASM, EPC_STORE_ASM)
__attribute__((section(".test"))) 
GEN_TEST_ACCESS_FAULT(fetch, ACCESS_FETCH_ASM, EPC_FETCH_ASM)
__attribute__((section(".test"))) 
GEN_TEST_ACCESS_FAULT(ltag, ACCESS_LTAG_ASM, EPC_TAG_ASM)
__attribute__((section(".test"))) 
GEN_TEST_ACCESS_FAULT(stag, ACCESS_STAG_ASM, EPC_TAG_ASM)
__attribute__((section(".test"))) 
GEN_TEST_ACCESS_FAULT(mttcb, ACCESS_MTTCB_ASM, EPC_TTCB_ASM)
__attribute__((section(".test"))) 
GEN_TEST_ACCESS_FAULT(sttcb, ACCESS_STTCB_ASM, EPC_TTCB_ASM)
