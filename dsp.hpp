#ifndef DSP_HPP
#define DSP_HPP

#include <complex>
#include <cmath>
#include <cstdint>

// int16_t iq
typedef struct {
    int16_t I;
    int16_t Q;
} iq16_t;

#define pow2(x) (uint32_t)(0x1 << x)

// =============================================================================
// Fast Furier Transform impl
// =============================================================================
static unsigned int bitReverse(unsigned int x, int log2n) {
    int n = 0;
    int mask = 0x1;
    for (int i=0; i < log2n; i++) {
        n <<= 1;
        n |= (x & 1);
        x >>= 1;
    }
    return n;
}

/**
 * @brief Функция расчёта FFT для вектора комплексных чисел
 * @param a Начальный итератор вектора комплексных отсчётов сигнала
 * @param b Начальный итератор вектора результата вычисления комплексного FFT
 * @param log2n 2^log2n порядок FFT
 */
template<class Iter_T>
void stdComplexFFT(Iter_T a, Iter_T b, int log2n)
{
    typedef typename std::iterator_traits<Iter_T>::value_type complex;
    const complex J(0, 1);
    int n = 1 << log2n;
    for (unsigned int i=0; i < n; ++i) {
        b[bitReverse(i, log2n)] = a[i];
    }
    for (int s = 1; s <= log2n; ++s) {
        int m = 1 << s;
        int m2 = m >> 1;
        complex w(1, 0);
        complex wm = exp(-J * ((float)M_PI / m2));
        for (int j=0; j < m2; ++j) {
            for (int k=j; k < n; k += m) {
                complex t = w * b[k + m2];
                complex u = b[k];
                b[k] = u + t;
                b[k + m2] = u - t;
            }
            w *= wm;
        }
    }
}

// =============================================================================

#endif // DSP_HPP
