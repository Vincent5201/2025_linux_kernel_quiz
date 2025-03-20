#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define clz32(x) clz2(x, 0)

static const int mask[] = {0, 8, 12, 14};
static const int magic[] = {1, 1, 0, 0};

unsigned clz2(uint32_t x, int c)
{
    if (!x && !c)
        return 32;
    uint32_t upper = (x >> (16 >> c));
    uint32_t lower = (x & (0xFFFF >> mask[c]));
    if (c == 3)
        return upper ? magic[upper] : 2 + magic[lower];
    return upper ? clz2(upper, c + 1) : (16 >> (c)) + clz2(lower, c + 1);
}

static inline int clz64(uint64_t x)
{
    /* If the high 32 bits are nonzero, count within them.
     * Otherwise, count in the low 32 bits and add 32.
     */
    return (x >> 32) ? clz32((uint32_t) (x >> 32)) : clz32((uint32_t) x) + 32;
}

uint64_t sqrti(uint64_t x)
{
    uint64_t m, y = 0;
    if (x <= 1)
        return x;

    int total_bits = 64;

    /* clz64(x) returns the count of leading zeros in x.
     * (total_bits - 1 - clz64(x)) gives the index of the highest set bit.
     * Rounding that index down to an even number ensures our starting m is a
     * power of 4.
     */
    int shift = (total_bits - 1 - clz64(x)) & 0xFFFE;
    m = 1ULL << shift;

    while (m) {
        uint64_t b = y + m;
        y >>= 1;
        if (x >= b) {
            x -= b;
            y += m;
        }
        m >>= 2;
    }
    return y;
}

uint64_t sqrtiup(uint64_t x)
{
    uint64_t m, y = 0;
    if (x <= 1)
        return x;

    int total_bits = 64;

    /* clz64(x) returns the count of leading zeros in x.
     * (total_bits - 1 - clz64(x)) gives the index of the highest set bit.
     * Rounding that index down to an even number ensures our starting m is a
     * power of 4.
     */
    int shift = (total_bits - 1 - clz64(x)) & 0xFFFE;
    m = 1ULL << shift;

    while (m) {
        uint64_t b = y + m;
        y >>= 1;
        if (x >= b) {
            x -= b;
            y += m;
        }
        m >>= 2;
    }
    y += (x > 0);
    return y;
}

uint32_t mysqrtf(uint32_t a0)
{
    uint32_t a1, a2, a3, a4;
    static const uint8_t rsqrt_lut[12] = {
        0xf1, 0xda, 0xc9, 0xbb, 0xb0, 0xa6, 0x9e, 0x97, 0x91, 0x8b, 0x86, 0x82
    };

    if (a0 < 0) {
        a1 = a0 << 1;
        a1 >>= 24;
        if (a1) {
            a0 >>= 31;
            if (a0 & 0x00000001)
                a0 |= 0xFFFFFFFE;
            a0 <<= 23;
        } else {
            a0 >>= 31;
            a0 <<= 31;
        }
    } else {
        a1 = a0 << 8;
        a1 |= 0x80000000;
        a1 >>= 8;
        a2 = a0 >> 23;
        if (a2) {
            if (a2 == 255) {
                a0 >>= 23;
                a0 <<= 23;
            } else {
                a2 += 125;
                a4 = a2 << 31;
                a2 >>= 1;
                if (a2 & 0x40000000)
                    a2 |= 0x80000000;
                if (a4)
                    a1 <<= 1;
                a3 = a1 >> 21;
                a4 = rsqrt_lut[a3];
                
                a0 = a1 >> 7;
                a0 *= a4;
                a0 *= a4;
                a0 >>= 12;
                if (a0 & 0x00080000)
                    a0 |= 0xFFF00000;
                
                a0 *= a4;
                a0 >>= 13;
                if (a0 & 0x00040000)
                    a0 |= 0xFFF80000;
                a4 <<= 8;
                a4 -= a0;
                a4 += 170;
                
                a0 = a4;
                a0 *= a0;
                a0 >>= 15;
                a3 = a1 >> 8;
                
                a0 *= a3;
                a0 >>= 12;
                if (a0 & 0x00080000)
                    a0 |= 0xFFF00000;
                a0 *= a4;
                a0 >>= 21;
                if (a0 & 0x00000400)
                    a0 |= 0xFFFFF800;
                a4 -= a0;
                
                a3 *= a4;
                a3 >>= 15;

                a0 = a3;
                a0 *= a0;
                a1 <<= 9;
                a0 = a1 - a0;
                a0 >>= 5;
                if (a0 & 0x04000000)
                    a0 |= 0xF8000000;

                a4 *= a0;
                a3 <<= 7;
                a0 = a4 >> 15;
                if (a0 & 0x00010000)
                    a0 |= 0xFFFE0000;
                a0 += 16;
                a0 >>= 6;
                if (a0 & 0x02000000)
                    a0 |= 0xFC000000;

                if (a3 < a0) {
                    a4 = 1;
                } else {
                    a4 = 0;
                }
                if (a4) {
                    a4 += a3;
                    a4 *= a4;
                    a1 <<= 16;
                    a1 -= a4;
                    if (a1 >= 0)
                        a3++;
                }
                a2 <<=  23;
                a0 = a2 + a3;
            }
        } else {
            a0 >>= 31;
            a0 <<= 31;
        }
    }
    return a0;
}

int main(void)
{
    for (float i = 2.0f; i <= 10 ;i += 0.5f){
        uint32_t a = *(uint32_t*)&i;
        uint32_t result32 = mysqrtf(a);
        float resultf = *(float*)&result32;
        printf("%f %f\n", i, resultf);
    }
    
    return 0;
}