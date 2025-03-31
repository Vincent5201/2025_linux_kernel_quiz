#include <stdint.h>
#include <stdio.h>

typedef int32_t fix16_t;
#define FIX16_ONE 0x00010000  /* 1.0 in fixed-point representation */

static inline float fix16_to_float(fix16_t a)
{
    return (float) a / FIX16_ONE;
}

static inline fix16_t float_to_fix16(float a)
{
    /* Round to nearest fixed-point value */
    return (fix16_t)(a * FIX16_ONE + (a >= 0 ? 0.5f : -0.5f));
}

static inline fix16_t int_to_fix16(int a)
{
    return a * FIX16_ONE;
}

static inline fix16_t fix16_mul(fix16_t x, fix16_t y)
{
    int64_t res = (int64_t) x * y;
    return (fix16_t) (res >> 16);
}

static inline fix16_t fix16_div(fix16_t a, fix16_t b)
{
    if (b == 0) /* Avoid division by zero */
        return 0;
    fix16_t result = (((int64_t) a) << 16) / ((int64_t) b);
    return result;
}

fix16_t fix16_exp(fix16_t in)
{
    if (in == 0)
        return FIX16_ONE;
    if (in == FIX16_ONE)
        return 178145 /* precomputed exp(1) in fixed-point */;
    if (in >= 681391)
        return 0x7FFFFFFF;
    if (in <= -772243)
        return 0;

    int neg = (in < 0);
    if (neg)
        in = -in;
    fix16_t result = in + FIX16_ONE;
    fix16_t term = in;
    for (uint_fast8_t i = 2; i < 30; i++) {
        term = fix16_mul(term, fix16_div(in, int_to_fix16(i)));
        result += term;
        /* Break early if the term is sufficiently small */
        if ((term < 500) && ((i > 15) || (term < 20)))
            break;
    }
    if (neg)
        result = fix16_div(FIX16_ONE, result);
    return result;
}

/* tanh(x) is defined as (exp(x) - exp(-x)) / (exp(x) + exp(-x)). */
fix16_t fix16_tanh(fix16_t in)
{
    fix16_t e_x = fix16_exp(in);
    fix16_t m_e_x = fix16_exp(-in);
    return fix16_div(e_x - m_e_x, e_x + m_e_x);
}

int main(void)
{
    printf("%f\n", fix16_to_float(fix16_tanh(float_to_fix16(0.5))));
}
