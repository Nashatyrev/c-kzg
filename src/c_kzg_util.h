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

/** @file c_kzg_util.h */

#include <stdlib.h> // free()
#include "c_kzg.h"
#include "poly.h"

#define min_u64(a, b)                                                                                                  \
    ({                                                                                                                 \
        uint64_t _a = (a);                                                                                             \
        uint64_t _b = (b);                                                                                             \
        _a < _b ? _a : _b;                                                                                             \
    })

C_KZG_RET c_kzg_malloc(void **p, size_t n);
C_KZG_RET new_uint64_array(uint64_t **x, size_t n);
C_KZG_RET new_fr_array(fr_t **x, size_t n);
C_KZG_RET new_fr_array_2(fr_t ***x, size_t n);
C_KZG_RET new_g1_array(g1_t **x, size_t n);
C_KZG_RET new_g1_array_2(g1_t ***x, size_t n);
C_KZG_RET new_g2_array(g2_t **x, size_t n);
C_KZG_RET new_poly_array(poly **x, size_t n);