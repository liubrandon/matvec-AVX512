#include "complex_dotproduct.hpp"
#include "immintrin.h"
#include <iostream>
#include <iomanip>
#include <stdio.h>

// Functions to print Intel vector types to help with debugging 

// Takes the 512 bit vector of integers v (__m512i ) and
// prints the 32 short ints (16 bits each) stored inside
void print_m512i(__m512i v) {
    int16_t* val = (int16_t*)&v;
    std::cout << "__m512i: ";
    for(int i = 0; i < 32; i+=2) {
        std::cout << "(" << std::setw(2) << val[i] << "," << std::setw(2) << val[i+1] << "), ";
    }
    std::cout << std::endl;
}

// Takes the 256 bit vector of integers v (__m256i ) and
// prints the 16 short ints (16 bits each) stored inside
void print_m256i(__m256i v) {
    int16_t* val = (int16_t*)&v;
    std::cout << "__m256i: ";
    for(int i = 0; i < 16; i+=2) {
        std::cout << "(" << std::setw(2) << val[i] << "," << std::setw(2) << val[i+1] << "), ";
    }
    std::cout << std::endl;
}

// Takes the 128 bit vector of integers v (__m128i ) and
// prints the 8 short ints (16 bits each) stored inside
void print_m128i(__m128i v) {
    int16_t* val = (int16_t*)&v;
    std::cout << "__m128i: ";
    for(int i = 0; i < 8; i+=2) {
        std::cout << "(" << std::setw(2) << val[i] << "," << std::setw(2) << val[i+1] << "), ";
    }
    std::cout << std::endl;
}

// dotProduct32x16() helper functions below
// Adapted Peter Cordes' 2/20/2020 horizontal sum but rewrote for Complex int16 numbers
// https://stackoverflow.com/questions/60108658/fastest-method-to-calculate-sum-of-all-packed-32-bit-integers-using-avx512-or-av

// Sums the 4 Complex numbers packed into v
Complex_int16 hsum4x32(__m128i v) {
    //    (c1  c2  c3 c4) is v 
    // +  (c3  c4   0  0) is _mm_permutexvar_epi16(_mm_setr_epi16(4,5,6,7,0,0,0,0))
    //  ------------------
    //    (c5  c6  c3 c4) c5 and c6 are the resulting complex numbers (last two values here are ignored)
    __m128i r1 = _mm_add_epi16(v, _mm_permutexvar_epi16(_mm_setr_epi16(4,5,6,7,0,0,0,0), v));
    // now, do c5 + c6 = res
    // c5 and c6 are complex so we can parallelize the two additions
    // c5 is the first two elements of __m128i r1
    // the below statement moves c6 to be the first two elements of __m128i r2
    __m128i r2 = _mm_setr_epi16(_mm_extract_epi16(r1, 2),_mm_extract_epi16(r1, 3),0,0,0,0,0,0);
    // Now we can add the real and imaginary compenents in parallel
    __m128i res = _mm_add_epi16(r1, r2);
    int16_t real = _mm_cvtsi128_si32(res); // extract first e
    int16_t imag = _mm_extract_epi16(res, 1);
    Complex_int16 ret = {real, imag};
    return ret;
}

// Sums the low half with the high half of v to reduce into __m128i
Complex_int16 hsum8x32(__m256i v) {
    __m128i sum128 = _mm_add_epi16( 
        _mm256_castsi256_si128(v), // low half
        _mm256_extracti128_si256(v, 1)); // high half
    return hsum4x32(sum128);
}

// Sums the low half with the high half of v to reduce into __m256i
// Unused for now
Complex_int16 hsum16x32(__m512i v) {
    __m256i sum256 = _mm256_add_epi16( 
        _mm512_castsi512_si256(v),  // low half
        _mm512_extracti64x4_epi64(v, 1)); // high half (function for 64 bit ints but still work fine for copying purpose)
    return hsum8x32(sum256);
}

// returns vec1 * vec2, where each vector contains 8 Complex numbers (int16 real + int16 imag = 32 bits each)
// Adapted Matt Scarpino's approach but for int16 instead of float
// https://www.codeproject.com/Articles/874396/Crunching-Numbers-with-AVX-and-AVX
__m256i _mm256_myComplexMult_epi16(__m256i vec1, __m256i vec2) {
    /* Step 1: Multiply vec1 and vec2 */
    __m256i vec3 = _mm256_mullo_epi16(vec1, vec2);
    /* Step 2: Switch the real and imaginary elements of vec2 */
    __m256i index2 = _mm256_setr_epi16(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14); // These numbers correspond to the permuted indexes of vec2
    vec2 = _mm256_permutexvar_epi16(index2, vec2);
    /* Step 3: Negate the imaginary elements of vec2 */
    __m256i neg = _mm256_setr_epi16(1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1);
    vec2 = _mm256_mullo_epi16(vec2, neg);
    /* Step 4: Multiply vec1 and the modified vec2 */
    __m256i vec4 = _mm256_mullo_epi16(vec1, vec2);
    /* Step 5: Horizontally subtract the elements in vec3 and vec4 */
    vec1 = _mm256_hsub_epi16(vec3, vec4);
    /* Step 6: Swap into correct spots */
    __m256i index6 = _mm256_setr_epi16(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15);
    vec1 = _mm256_permutexvar_epi16(index6, vec1);
    return vec1;
}

void print_m512(__m512 v) {
    float* val = (float*)&v;
    std::cout << "__m512: ";
    for(int i = 0; i < 8; i+=2) {
        std::cout << "(" << std::setw(2) << val[i] << "," << std::setw(2) << val[i+1] << "), ";
    }
    std::cout << std::endl;
}

__m512 _mm512_myComplexMult_ps(__m512 a, __m512 b) {
    __m512 b_flip = _mm512_shuffle_ps(b,b,0xB1);   // Swap b.re and b.im
    __m512 a_im   = _mm512_shuffle_ps(a,a,0xF5);   // Imag part of a in both
    __m512 a_re   = _mm512_shuffle_ps(a,a,0xA0);   // Real part of a in both
    __m512 aib    = _mm512_mul_ps(a_im, b_flip);   // (a.im*b.im, a.im*b.re)
    print_m512(a_re);
    print_m512(b);
    print_m512(aib);
    return _mm512_fmaddsub_ps(a_re, b, aib);   // a_re * b +/- aib
}

__m512i _mm512_myComplexMult_epi16(__m512i a, __m512i b) {
    // Not sure why _mm512_set_epi16 throws an error but the below array to vector conversion should be equivalent
    // idx0 corresponds to indices to swap real and imag, idx1 sets both component to imag, idx2 sets both components to real
    const int16_t temp0[32] = {1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 17, 16, 19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31, 30};
    const int16_t temp1[32] = {1, 1, 3, 3, 5, 5, 7, 7, 9, 9, 11, 11, 13, 13, 15, 15, 17, 17, 19, 19, 21, 21, 23, 23, 25, 25, 27, 27, 29, 29, 31, 31};
    const int16_t temp2[32] = {0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14, 16, 16, 18, 18, 20, 20, 22, 22, 24, 24, 26, 26, 28, 28, 30, 30};
    const int16_t temp3[32] = {-1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1};
    const __m512i idx0 = *(__m512i*)temp0;
    const __m512i idx1 = *(__m512i*)temp1;
    const __m512i idx2 = *(__m512i*)temp2;
    const __m512i addsub = *(__m512i*)temp3;
    __m512i b_flip = _mm512_permutexvar_epi16(idx0, b); // Swap b.re and b.im
    __m512i a_im = _mm512_permutexvar_epi16(idx1, a); // Imag part of a in both
    __m512i a_re = _mm512_permutexvar_epi16(idx2, a); // Real part of a in both
    __m512i aib = _mm512_mullo_epi16(a_im, b_flip);   // (a.im*b.im, a.im*b.re)
    __m512i areb = _mm512_mullo_epi16(a_re, b);   // a_re * b
    __m512i aib_addsub = _mm512_mullo_epi16(aib, addsub); // flips sign of even index values
    return _mm512_add_epi16(areb, aib_addsub);   // areb +/- aib
}

Complex_int16 dotProduct16x32(__m512i a, __m512i b) {
    return _mm512_reduce_add_epi16(_mm512_myComplexMult_epi16(a, b));
}


// a dot b, where a and b are vectors with 16 elements, each a 32 bit complex number {int16 real, int16 imag}
Complex_int16 old_dotProduct16x32(__m512i a, __m512i b) {
    // Split a and b into front and back halves
    __m256i aFront = _mm512_castsi512_si256(a);
    __m256i aBack = _mm512_extracti64x4_epi64(a, 1);
    __m256i bFront = _mm512_castsi512_si256(b);
    __m256i bBack = _mm512_extracti64x4_epi64(b, 1);
    // (aFront dot bFront) + (aBack dot bBack) is the same as a dot b
    __m256i frontMul = _mm256_myComplexMult_epi16(aFront, bFront);
    __m256i backMul = _mm256_myComplexMult_epi16(aBack, bBack);
    return (hsum8x32(frontMul) + hsum8x32(backMul));
}



