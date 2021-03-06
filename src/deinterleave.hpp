#ifndef DEINTERLEAVE_HPP
#define DEINTERLEAVE_HPP
#include "immintrin.h"
#include "complex_dotproduct.hpp"
#include <iostream>
static const int16_t temp[32] = {0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,   // real part
                                 1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,}; // imag part
const __m512i seperateSwap = _mm512_loadu_si512((const void*)temp);

// Takes in Complex_int16 matrix src and store its deinterleaved component matrices in real and imag
static inline void deinterleaveMatrix(const Complex_int16* src, int size, int16_t* real_res, int16_t* imag_res) {
    
    // Take two 512 vectors of 16 complex numbers and get the 32 real and 32 complex numbers in two resulting 512 bit vectors
    for(int i = 0; i < size; i+=32) {
        // Step 1: Get 2 vectors of 16 complex numbers each
        __m512i complex1 = _mm512_load_si512((const __m512i*)&src[i]);
        __m512i complex2 = _mm512_load_si512((const __m512i*)&src[i+16]);

        // Step 2: Permute both vectors so that first half is the
        // real component and the second half is imaginary (seperate real and complex)
        complex1 = _mm512_permutexvar_epi16(seperateSwap, complex1);
        complex2 = _mm512_permutexvar_epi16(seperateSwap, complex2);

        // Step 3: Combine the first halves and the second halves to get the
        // resulting 32 real and 32 imaginary numbers in their own vectors
        __m256i real2 = _mm512_castsi512_si256(complex2); // cast has zero overhead
        __m512i real = _mm512_inserti32x8(complex1, real2, 1);
        __m256i imag1 = _mm512_extracti64x4_epi64(complex1, 1);
        __m512i imag = _mm512_inserti32x8(complex2, imag1, 0);

        // print_m512i(real);
        // print_m512i(imag);
        // Step 4: Store the resulting vectors into the real and imag parameters
        _mm512_store_si512((void*)(&real_res[i]), real);
        _mm512_store_si512((void*)(&imag_res[i]), imag);
    }
}

#endif