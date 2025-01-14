/*
 * Copyright 2021 Benjamin Edgington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../inc/acutest.h"
#include "test_util.h"
#include "poly.h"

void poly_div_0(void) {
    fr_t a[3], b[2], expected[2];
    poly dividend, divisor, actual;

    // Calculate (x^2 - 1) / (x + 1) = x - 1

    // Dividend
    fr_from_uint64(&a[0], 1);
    fr_negate(&a[0], &a[0]);
    fr_from_uint64(&a[1], 0);
    fr_from_uint64(&a[2], 1);
    dividend.length = 3;
    dividend.coeffs = a;

    // Divisor
    fr_from_uint64(&b[0], 1);
    fr_from_uint64(&b[1], 1);
    divisor.length = 2;
    divisor.coeffs = b;

    // Known result
    fr_from_uint64(&expected[0], 1);
    fr_negate(&expected[0], &expected[0]);
    fr_from_uint64(&expected[1], 1);

    TEST_CHECK(C_KZG_OK == new_poly_long_div(&actual, &dividend, &divisor));
    TEST_CHECK(fr_equal(&expected[0], &actual.coeffs[0]));
    TEST_CHECK(fr_equal(&expected[1], &actual.coeffs[1]));

    free_poly(&actual);
}

void poly_div_1(void) {
    fr_t a[4], b[2], expected[3];
    poly dividend, divisor, actual;

    // Calculate (12x^3 - 11x^2 + 9x + 18) / (4x + 3) = 3x^2 - 5x + 6

    // Dividend
    fr_from_uint64(&a[0], 18);
    fr_from_uint64(&a[1], 9);
    fr_from_uint64(&a[2], 11);
    fr_negate(&a[2], &a[2]);
    fr_from_uint64(&a[3], 12);
    dividend.length = 4;
    dividend.coeffs = a;

    // Divisor
    fr_from_uint64(&b[0], 3);
    fr_from_uint64(&b[1], 4);
    divisor.length = 2;
    divisor.coeffs = b;

    // Known result
    fr_from_uint64(&expected[0], 6);
    fr_from_uint64(&expected[1], 5);
    fr_negate(&expected[1], &expected[1]);
    fr_from_uint64(&expected[2], 3);

    TEST_CHECK(C_KZG_OK == new_poly_long_div(&actual, &dividend, &divisor));
    TEST_CHECK(fr_equal(&expected[0], &actual.coeffs[0]));
    TEST_CHECK(fr_equal(&expected[1], &actual.coeffs[1]));
    TEST_CHECK(fr_equal(&expected[2], &actual.coeffs[2]));

    free_poly(&actual);
}

void poly_div_2(void) {
    fr_t a[2], b[3];
    poly dividend, divisor, actual;

    // Calculate (x + 1) / (x^2 - 1) = nil

    // Dividend
    fr_from_uint64(&a[0], 1);
    fr_from_uint64(&a[1], 1);
    dividend.length = 2;
    dividend.coeffs = a;

    // Divisor
    fr_from_uint64(&b[0], 1);
    fr_negate(&b[0], &b[0]);
    fr_from_uint64(&b[1], 0);
    fr_from_uint64(&b[2], 1);
    divisor.length = 3;
    divisor.coeffs = b;

    TEST_CHECK(C_KZG_OK == new_poly_long_div(&actual, &dividend, &divisor));
    TEST_CHECK(NULL == actual.coeffs);

    free_poly(&actual);
}

void poly_div_by_zero(void) {
    fr_t a[2];
    poly dividend, divisor, dummy;

    // Calculate (x + 1) / 0 = FAIL

    // Dividend
    fr_from_uint64(&a[0], 1);
    fr_from_uint64(&a[1], 1);
    dividend.length = 2;
    dividend.coeffs = a;

    // Divisor
    new_poly(&divisor, 0);

    TEST_CHECK(C_KZG_BADARGS == new_poly_long_div(&dummy, &dividend, &divisor));
}

void poly_eval_check(void) {
    uint64_t n = 10;
    fr_t actual, expected;
    poly p;
    new_poly(&p, n);
    for (uint64_t i = 0; i < n; i++) {
        fr_from_uint64(&p.coeffs[i], i + 1);
    }
    fr_from_uint64(&expected, n * (n + 1) / 2);

    eval_poly(&actual, &p, &fr_one);

    TEST_CHECK(fr_equal(&expected, &actual));

    free_poly(&p);
}

void poly_eval_0_check(void) {
    uint64_t n = 7, a = 597;
    fr_t actual, expected;
    poly p;
    new_poly(&p, n);
    for (uint64_t i = 0; i < n; i++) {
        fr_from_uint64(&p.coeffs[i], i + a);
    }
    fr_from_uint64(&expected, a);

    eval_poly(&actual, &p, &fr_zero);

    TEST_CHECK(fr_equal(&expected, &actual));

    free_poly(&p);
}

void poly_eval_nil_check(void) {
    uint64_t n = 0;
    fr_t actual;
    poly p;
    new_poly(&p, n);

    eval_poly(&actual, &p, &fr_one);

    TEST_CHECK(fr_equal(&fr_zero, &actual));

    free_poly(&p);
}

TEST_LIST = {
    {"POLY_TEST", title},
    {"poly_div_0", poly_div_0},
    {"poly_div_1", poly_div_1},
    {"poly_div_2", poly_div_2},
    {"poly_div_by_zero", poly_div_by_zero},
    {"poly_eval_check", poly_eval_check},
    {"poly_eval_0_check", poly_eval_0_check},
    {"poly_eval_nil_check", poly_eval_nil_check},
    {NULL, NULL} /* zero record marks the end of the list */
};
