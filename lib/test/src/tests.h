/*
 * Copyright (c) 2015, Dan Collins
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _WS_TESTS_H
#define _WS_TESTS_H


extern int test_cnt, pass_cnt;
extern bool res;

/**
 * Add tests to this list and they will get automatically added to the test
 * execution path.
 */
#define TESTS\
    X(list_add_elements)\
    X(list_add_elements_sorted)\
    X(list_remove_elements) \
    X(list_count_elements) \
    X(list_test_empty) \
    X(list_test_first) \
    X(list_test_last)


/* Prototypes */
#define X(name)\
    extern bool\
    name(void);
TESTS;
#undef X


/* Test macros */
#define RUN_TEST(test_name)\
    do\
    {\
        res = test_name();\
        printf(#test_name ": %s\n", res == true ? "PASS" : "FAIL");\
        if (res)\
            pass_cnt++;\
        test_cnt++;\
    } while(0)

#define X(name) RUN_TEST(name);
#define RUN_TESTS()\
    do\
    {\
        TESTS;\
    } while (0)




#endif /* _WS_TESTS_H */
