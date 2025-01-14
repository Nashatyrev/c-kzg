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

/**
 *  @file recover.c
 *
 * Recover polynomials from samples.
 */

#include "recover.h"
#include "c_kzg_util.h"
#include "fft_fr.h"
#include "utility.h"
#include "zero_poly.h"

/** 5 is a primitive element, but actually this can be pretty much anything not 0 or a low-degree root of unity */
#define SCALE_FACTOR 5

/**
 * Scale a polynomial in place.
 *
 * Multiplies each coefficient by `1 / scale_factor ^ i`. Equivalent to creating a polynomial that evaluates at `x * k`
 * rather than `x`.
 *
 * @param[out,in] p The polynomial coefficients to be scaled
 * @param[in] len_p Length of the polynomial coefficients
 */
void scale_poly(fr_t *p, uint64_t len_p) {
    fr_t scale_factor, factor_power, inv_factor;
    fr_from_uint64(&scale_factor, SCALE_FACTOR);
    fr_inv(&inv_factor, &scale_factor);
    factor_power = fr_one;

    for (uint64_t i = 1; i < len_p; i++) {
        fr_mul(&factor_power, &factor_power, &inv_factor);
        fr_mul(&p[i], &p[i], &factor_power);
    }
}

/**
 * Unscale a polynomial in place.
 *
 * Multiplies each coefficient by `scale_factor ^ i`. Equivalent to creating a polynomial that evaluates at `x / k`
 * rather than `x`.
 *
 * @param[out,in] p The polynomial coefficients to be unscaled
 * @param[in] len_p Length of the polynomial coefficients
 */
void unscale_poly(fr_t *p, uint64_t len_p) {
    fr_t scale_factor, factor_power;
    fr_from_uint64(&scale_factor, SCALE_FACTOR);
    factor_power = fr_one;

    for (uint64_t i = 1; i < len_p; i++) {
        fr_mul(&factor_power, &factor_power, &scale_factor);
        fr_mul(&p[i], &p[i], &factor_power);
    }
}

/**
 * Given a dataset with up to half the entries missing, return the reconstructed original.
 *
 * Assumes that the inverse FFT of the original data has the upper half of its values equal to zero.
 *
 * See https://ethresear.ch/t/reed-solomon-erasure-code-recovery-in-n-log-2-n-time-with-ffts/3039
 *
 * @param[out] reconstructed_data An attempted reconstruction of the original data
 * @param[in]  samples            The data to be reconstructed, with `fr_null` set for missing values
 * @param[in]  len_samples        The length of @p samples and @p reconstructed_data
 * @param[in]  fs                 The FFT settings previously initialised with #new_fft_settings
 * @retval C_CZK_OK      All is well
 * @retval C_CZK_BADARGS Invalid parameters were supplied
 * @retval C_CZK_ERROR   An internal error occurred
 * @retval C_CZK_MALLOC  Memory allocation failed
 */
C_KZG_RET recover_poly_from_samples(fr_t *reconstructed_data, fr_t *samples, uint64_t len_samples, FFTSettings *fs) {

    CHECK(is_power_of_two(len_samples));

    uint64_t *missing;
    TRY(new_uint64_array(&missing, len_samples));

    uint64_t len_missing = 0;
    for (uint64_t i = 0; i < len_samples; i++) {
        if (fr_is_null(&samples[i])) {
            missing[len_missing++] = i;
        }
    }

    // Make scratch areas, each of size len_samples. Cuts space required by 57%.
    fr_t *scratch;
    TRY(new_fr_array(&scratch, 3 * len_samples));
    fr_t *scratch0 = scratch;
    fr_t *scratch1 = scratch0 + len_samples;
    fr_t *scratch2 = scratch1 + len_samples;

    // Assign meaningful names to scratch spaces
    fr_t *zero_eval = scratch0;
    fr_t *poly_evaluations_with_zero = scratch2;
    fr_t *poly_with_zero = scratch0;
    fr_t *eval_scaled_poly_with_zero = scratch2;
    fr_t *eval_scaled_zero_poly = scratch0;
    fr_t *scaled_reconstructed_poly = scratch1;

    poly zero_poly;
    zero_poly.length = len_samples;
    zero_poly.coeffs = scratch1;

    // Calculate `Z_r,I`
    TRY(zero_polynomial_via_multiplication(zero_eval, &zero_poly, len_samples, missing, len_missing, fs));

    // Check all is well
    for (uint64_t i = 0; i < len_samples; i++) {
        TRY(fr_is_null(&samples[i]) == fr_is_zero(&zero_eval[i]) ? C_KZG_OK : C_KZG_ERROR);
    }

    // Construct E * Z_r,I: the loop makes the evaluation polynomial
    for (uint64_t i = 0; i < len_samples; i++) {
        if (fr_is_null(&samples[i])) {
            poly_evaluations_with_zero[i] = fr_zero;
        } else {
            fr_mul(&poly_evaluations_with_zero[i], &samples[i], &zero_eval[i]);
        }
    }
    // Now inverse FFT so that poly_with_zero is (E * Z_r,I)(x) = (D * Z_r,I)(x)
    TRY(fft_fr(poly_with_zero, poly_evaluations_with_zero, true, len_samples, fs));

    // x -> k * x
    scale_poly(poly_with_zero, len_samples);
    scale_poly(zero_poly.coeffs, zero_poly.length);

    // Q1 = (D * Z_r,I)(k * x)
    fr_t *scaled_poly_with_zero = poly_with_zero; // Renaming
    // Q2 = Z_r,I(k * x)
    fr_t *scaled_zero_poly = zero_poly.coeffs; // Renaming

    // Polynomial division by convolution: Q3 = Q1 / Q2
    TRY(fft_fr(eval_scaled_poly_with_zero, scaled_poly_with_zero, false, len_samples, fs));
    TRY(fft_fr(eval_scaled_zero_poly, scaled_zero_poly, false, len_samples, fs));

    fr_t *eval_scaled_reconstructed_poly = eval_scaled_poly_with_zero;
    for (uint64_t i = 0; i < len_samples; i++) {
        fr_div(&eval_scaled_reconstructed_poly[i], &eval_scaled_poly_with_zero[i], &eval_scaled_zero_poly[i]);
    }

    // The result of the division is D(k * x):
    TRY(fft_fr(scaled_reconstructed_poly, eval_scaled_reconstructed_poly, true, len_samples, fs));

    // k * x -> x
    unscale_poly(scaled_reconstructed_poly, len_samples);

    // Finally we have D(x) which evaluates to our original data at the powers of roots of unity
    fr_t *reconstructed_poly = scaled_reconstructed_poly; // Renaming

    // The evaluation polynomial for D(x) is the reconstructed data:
    TRY(fft_fr(reconstructed_data, reconstructed_poly, false, len_samples, fs));

    // Check all is well
    for (uint64_t i = 0; i < len_samples; i++) {
        TRY(fr_is_null(&samples[i]) || fr_equal(&reconstructed_data[i], &samples[i]) ? C_KZG_OK : C_KZG_ERROR);
    }

    free(scratch);
    free(missing);

    return C_KZG_OK;
}