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
#include "utility.h"
#include "c_kzg_util.h"

static uint32_t rev_bits_slow(uint32_t a) {
    uint32_t ret = 0;
    for (int i = 0; i < 32; i++) {
        ret <<= 1;
        ret |= a & 1;
        a >>= 1;
    }
    return ret;
}

void is_power_of_two_works(void) {
    // All actual powers of two
    for (int i = 0; i <= 63; i++) {
        TEST_CHECK(true == is_power_of_two((uint64_t)1 << i));
        TEST_MSG("Case %d", i);
    }

    // This is a bit weird
    TEST_CHECK(true == is_power_of_two(0));

    // Not powers of two
    TEST_CHECK(false == is_power_of_two(123));
    TEST_CHECK(false == is_power_of_two(1234567));
}

void test_log2_pow2(void) {
    int actual, expected;
    for (int i = 0; i < 32; i++) {
        expected = i;
        actual = log2_pow2((uint32_t)1 << i);
        TEST_CHECK(expected == actual);
    }
}

void test_next_power_of_two_powers(void) {
    for (int i = 0; i <= 63; i++) {
        uint64_t expected = (uint64_t)1 << i;
        uint64_t actual = next_power_of_two(expected);
        TEST_CHECK(expected == actual);
    }
}

void test_next_power_of_two_random(void) {
    for (int i = 0; i < 32768; i++) {
        uint64_t a = 1 + (rand_uint64() >> 1); // It's not expected to work for a > 2^63
        uint64_t higher = next_power_of_two(a);
        uint64_t lower = higher >> 1;
        if (!(TEST_CHECK(is_power_of_two(higher)) && TEST_CHECK(higher >= a) && TEST_CHECK(lower < a))) {
            TEST_MSG("Failed for %lu", a);
        }
    }
}

void test_reverse_bits_macros(void) {
    TEST_CHECK(128 == rev_byte(1));
    TEST_CHECK(128 == rev_byte(257));
    TEST_CHECK((uint32_t)1 << 31 == rev_4byte(1));
    TEST_CHECK(0x1e6a2c48 == rev_4byte(0x12345678));
    TEST_CHECK(0x00000000 == rev_4byte(0x00000000));
    TEST_CHECK(0xffffffff == rev_4byte(0xffffffff));
}

void test_reverse_bits_powers(void) {
    uint32_t actual, expected;
    for (int i = 0; i < 32; i++) {
        expected = (uint32_t)1 << (31 - i);
        actual = reverse_bits((uint32_t)1 << i);
        TEST_CHECK(expected == actual);
    }
}

void test_reverse_bits_random(void) {
    for (int i = 0; i < 32768; i++) {
        uint32_t a = (uint32_t)(rand_uint64() & 0xffffffffL);
        TEST_CHECK(rev_bits_slow(a) == reverse_bits(a));
        TEST_MSG("Failed for %08x. Expected %08x, got %08x.", a, rev_bits_slow(a), reverse_bits(a));
    }
}

void test_reverse_bit_order_g1(void) {
    int size = 10, n = 1 << size;
    g1_t a[n], b[n];
    fr_t tmp;

    for (int i = 0; i < n; i++) {
        fr_from_uint64(&tmp, i);
        g1_mul(&a[i], &g1_generator, &tmp);
        b[i] = a[i];
    }

    TEST_CHECK(C_KZG_OK == reverse_bit_order(a, sizeof(g1_t), n));
    for (int i = 0; i < n; i++) {
        TEST_CHECK(true == g1_equal(&b[reverse_bits(i) >> (32 - size)], &a[i]));
    }

    // Hand check a few select values
    TEST_CHECK(true == g1_equal(&b[0], &a[0]));
    TEST_CHECK(false == g1_equal(&b[1], &a[1]));
    TEST_CHECK(true == g1_equal(&b[n - 1], &a[n - 1]));
}

void test_reverse_bit_order_fr(void) {
    int size = 12, n = 1 << size;
    fr_t a[n], b[n];

    for (int i = 0; i < n; i++) {
        fr_from_uint64(&a[i], i);
        b[i] = a[i];
    }

    TEST_CHECK(C_KZG_OK == reverse_bit_order(a, sizeof(fr_t), n));
    for (int i = 0; i < n; i++) {
        TEST_CHECK(true == fr_equal(&b[reverse_bits(i) >> (32 - size)], &a[i]));
    }

    // Hand check a few select values
    TEST_CHECK(true == fr_equal(&b[0], &a[0]));
    TEST_CHECK(false == fr_equal(&b[1], &a[1]));
    TEST_CHECK(true == fr_equal(&b[n - 1], &a[n - 1]));
}

void test_reverse_bit_order_fr_large(void) {
    int size = 22, n = 1 << size;
    fr_t *a, *b;

    TEST_CHECK(C_KZG_OK == new_fr_array(&a, n));
    TEST_CHECK(C_KZG_OK == new_fr_array(&b, n));

    for (int i = 0; i < n; i++) {
        fr_from_uint64(&a[i], i);
        b[i] = a[i];
    }

    TEST_CHECK(C_KZG_OK == reverse_bit_order(a, sizeof(fr_t), n));
    for (int i = 0; i < n; i++) {
        TEST_CHECK(true == fr_equal(&b[reverse_bits(i) >> (32 - size)], &a[i]));
    }

    // Hand check a few select values
    TEST_CHECK(true == fr_equal(&b[0], &a[0]));
    TEST_CHECK(false == fr_equal(&b[1], &a[1]));
    TEST_CHECK(true == fr_equal(&b[n - 1], &a[n - 1]));

    free(a);
    free(b);
}

TEST_LIST = {
    {"UTILITY_TEST", title},
    {"is_power_of_two_works", is_power_of_two_works},
    {"test_log2_pow2", test_log2_pow2},
    {"test_next_power_of_two_powers", test_next_power_of_two_powers},
    {"test_next_power_of_two_random", test_next_power_of_two_random},
    {"test_reverse_bits_macros", test_reverse_bits_macros},
    {"test_reverse_bits_powers", test_reverse_bits_powers},
    {"test_reverse_bits_random", test_reverse_bits_random},
    {"test_reverse_bit_order_g1", test_reverse_bit_order_g1},
    {"test_reverse_bit_order_fr", test_reverse_bit_order_fr},
    {"test_reverse_bit_order_fr_large", test_reverse_bit_order_fr_large},
    {NULL, NULL} /* zero record marks the end of the list */
};
