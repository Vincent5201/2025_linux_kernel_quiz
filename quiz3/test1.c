#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* mpi: Multi-Precision Integers */
typedef struct {
    uint32_t *data;
    size_t capacity;
} mpi_t[1];

typedef size_t mp_bitcnt_t;

void mpi_init(mpi_t rop)
{
    rop->capacity = 0;
    rop->data = NULL;
}

void mpi_clear(mpi_t rop)
{
    free(rop->data);
}

void mpi_enlarge(mpi_t rop, size_t capacity)
{
    if (capacity > rop->capacity) {
        size_t min = rop->capacity;

        rop->capacity = capacity;

        rop->data = realloc(rop->data, capacity * 4);

        if (!rop->data && capacity != 0) {
            fprintf(stderr, "Out of memory (%zu words requested)\n", capacity);
            abort();
        }

        for (size_t n = min; n < capacity; ++n)
            rop->data[n] = 0;
    }
}

void mpi_compact(mpi_t rop)
{
    size_t capacity = rop->capacity;

    if (rop->capacity == 0)
        return;

    for (size_t i = rop->capacity - 1; i != (size_t) -1; --i) {
        if (rop->data[i])
            break;
        capacity--;
    }

    assert(capacity != (size_t) -1);

    rop->data = realloc(rop->data, capacity * 4);
    rop->capacity = capacity;

    if (!rop->data && capacity != 0) {
        fprintf(stderr, "Out of memory (%zu words requested)\n", capacity);
        abort();
    }
}

/* ceiling division without needing floating-point operations. */
static size_t ceil_div(size_t n, size_t d)
{
    return (n + d - 1) / d;
}

#define INTMAX 0x7fffffff

void mpi_set_u64(mpi_t rop, uint64_t op)
{
    size_t capacity = ceil_div(64, 31);

    mpi_enlarge(rop, capacity);

    for (size_t n = 0; n < capacity; ++n) {
        rop->data[n] = op & INTMAX;
        op >>= 31;
    }

    for (size_t n = capacity; n < rop->capacity; ++n)
        rop->data[n] = 0;
}

void mpi_set_u32(mpi_t rop, uint32_t op)
{
    size_t capacity = ceil_div(32, 31);

    mpi_enlarge(rop, capacity);

    for (size_t n = 0; n < capacity; ++n) {
        rop->data[n] = op & INTMAX;
        op >>= 31;
    }

    for (size_t n = capacity; n < rop->capacity; ++n)
        rop->data[n] = 0;
}

uint64_t mpi_get_u64(const mpi_t op)
{
    size_t capacity = op->capacity;

    if (capacity > ceil_div(64, 31))
        capacity = ceil_div(64, 31);

    uint64_t r = 0;

    for (size_t n = capacity - 1; n != (size_t) -1; --n) {
        r <<= 31;
        r |= op->data[n];
    }

    return r;
}

uint32_t mpi_get_u32(const mpi_t op)
{
    size_t capacity = op->capacity;

    if (capacity > ceil_div(32, 31))
        capacity = ceil_div(32, 31);

    uint32_t r = 0;

    for (size_t n = capacity - 1; n != (size_t) -1; --n) {
        r <<= 31;
        r |= op->data[n];
    }

    return r;
}

void mpi_add(mpi_t rop, const mpi_t op1, const mpi_t op2)
{
    size_t capacity =
        op1->capacity > op2->capacity ? op1->capacity : op2->capacity;

    mpi_enlarge(rop, capacity);

    uint32_t c = 0;

    /* op1 + op2 */
    for (size_t n = 0; n < rop->capacity; ++n) {
        uint32_t r1 = (n < op1->capacity) ? op1->data[n] : 0;
        uint32_t r2 = (n < op2->capacity) ? op2->data[n] : 0;
        rop->data[n] = r1 + r2 + c;
        c = rop->data[n] >> 31;
        rop->data[n] &= INTMAX;
    }

    if (c != 0) {
        mpi_enlarge(rop, capacity + 1);
        rop->data[capacity] = 0 + c;
    }

    mpi_compact(rop);
}

void mpi_sub(mpi_t rop, const mpi_t op1, const mpi_t op2)
{
    size_t capacity =
        op1->capacity > op2->capacity ? op1->capacity : op2->capacity;

    mpi_enlarge(rop, capacity);

    uint32_t c = 0;

    /* op1 - op2 */
    for (size_t n = 0; n < rop->capacity; ++n) {
        uint32_t r1 = (n < op1->capacity) ? op1->data[n] : 0;
        uint32_t r2 = (n < op2->capacity) ? op2->data[n] : 0;
        rop->data[n] = r1 - r2 - c;
        c = rop->data[n] >> 31;
        rop->data[n] &= INTMAX;
    }

    if (c != 0) {
        fprintf(stderr, "Negative numbers not supported\n");
        abort();
    }

    mpi_compact(rop);
}

void mpi_add_u64(mpi_t rop, const mpi_t op1, uint64_t op2)
{
    size_t capacity =
        op1->capacity > ceil_div(64, 31) ? op1->capacity : ceil_div(64, 31);

    mpi_enlarge(rop, capacity);

    uint32_t c = 0;

    /* op1 + op2 */
    for (size_t n = 0; n < rop->capacity; ++n) {
        uint32_t r1 = (n < op1->capacity) ? op1->data[n] : 0;
        uint32_t r2 = op2 & INTMAX;
        op2 >>= 31;
        rop->data[n] = r1 + r2 + c;
        c = rop->data[n] >> 31;
        rop->data[n] &= INTMAX;
    }

    if (c != 0) {
        mpi_enlarge(rop, capacity + 1);
        rop->data[capacity] = 0 + c;
    }
}

void mpi_add_u32(mpi_t rop, const mpi_t op1, uint32_t op2)
{
    size_t capacity =
        op1->capacity > ceil_div(32, 31) ? op1->capacity : ceil_div(32, 31);

    mpi_enlarge(rop, capacity);

    uint32_t c = 0;

    /* op1 + op2 */
    for (size_t n = 0; n < rop->capacity; ++n) {
        uint32_t r1 = (n < op1->capacity) ? op1->data[n] : 0;
        uint32_t r2 = op2 & INTMAX;
        op2 >>= 31;
        rop->data[n] = r1 + r2 + c;
        c = rop->data[n] >> 31;
        rop->data[n] &= INTMAX;
    }

    if (c != 0) {
        mpi_enlarge(rop, capacity + 1);
        rop->data[capacity] = 0 + c;
    }
}

void mpi_sub_u32(mpi_t rop, const mpi_t op1, uint32_t op2)
{
    size_t capacity =
        op1->capacity > ceil_div(32, 31) ? op1->capacity : ceil_div(32, 31);

    mpi_enlarge(rop, capacity);

    uint32_t c = 0;

    /* op1 + op2 */
    for (size_t n = 0; n < rop->capacity; ++n) {
        uint32_t r1 = (n < op1->capacity) ? op1->data[n] : 0;
        uint32_t r2 = op2 & INTMAX;
        op2 >>= 31;
        rop->data[n] = r1 - r2 - c;
        c = rop->data[n] >> 31;
        rop->data[n] &= INTMAX;
    }

    if (c != 0) {
        fprintf(stderr, "Negative numbers not supported\n");
        abort();
    }
}

void mpi_mul_u32(mpi_t rop, const mpi_t op1, uint32_t op2)
{
    size_t capacity = op1->capacity + 1;

    mpi_enlarge(rop, capacity);

    uint32_t c = 0;

    /* op1 * op2 */
    for (size_t n = 0; n < op1->capacity; ++n) {
        assert(op1->data[n] <= (UINT64_MAX - c) / op2);
        uint64_t r = (uint64_t) op1->data[n] * op2 + c;
        rop->data[n] = r & INTMAX;
        c = r >> 31;
    }

    while (c != 0) {
        mpi_enlarge(rop, capacity + 1);
        rop->data[capacity] = c & INTMAX;
        c >>= 31;
    }
}

int mpi_set_str(mpi_t rop, const char *str, int base)
{
    assert(base == 10); /* only decimal integers */

    size_t len = strlen(str);

    mpi_set_u32(rop, 0U);

    for (size_t i = 0; i < len; ++i) {
        mpi_mul_u32(rop, rop, 10U);
        assert(str[i] >= '0' && str[i] <= '9');
        mpi_add_u32(rop, rop, (uint32_t) (str[i] - '0'));
    }

    return 0;
}

void mpi_set(mpi_t rop, const mpi_t op)
{
    mpi_enlarge(rop, op->capacity);

    for (size_t n = 0; n < op->capacity; ++n)
        rop->data[n] = op->data[n];

    for (size_t n = op->capacity; n < rop->capacity; ++n)
        rop->data[n] = 0;
}

/* Naive multiplication */
static void mpi_mul_naive(mpi_t rop, const mpi_t op1, const mpi_t op2)
{
    size_t capacity = op1->capacity + op2->capacity;

    mpi_t tmp;
    mpi_init(tmp);

    mpi_enlarge(tmp, capacity);

    for (size_t n = 0; n < tmp->capacity; ++n)
        tmp->data[n] = 0;

    for (size_t n = 0; n < op1->capacity; ++n) {
        for (size_t m = 0; m < op2->capacity; ++m) {
            uint64_t r = (uint64_t) op1->data[n] * op2->data[m];
            uint64_t c = 0;
            for (size_t k = m + n; c || r; ++k) {
                if (k >= tmp->capacity)
                    mpi_enlarge(tmp, tmp->capacity + 1);
                tmp->data[k] += (r & INTMAX) + c;
                r >>= 31;
                c = tmp->data[k] >> 31;
                tmp->data[k] &= INTMAX;
            }
        }
    }

    mpi_set(rop, tmp);

    mpi_compact(rop);

    mpi_clear(tmp);
}

void mpi_fdiv_r_2exp(mpi_t r, const mpi_t n, mp_bitcnt_t b);
void mpi_fdiv_q_2exp(mpi_t q, const mpi_t n, mp_bitcnt_t b);
void mpi_mul_2exp(mpi_t rop, const mpi_t op1, mp_bitcnt_t op2);

/* Karatsuba algorithm */
static void mpi_mul_karatsuba(mpi_t rop, const mpi_t op1, const mpi_t op2)
{
    /* end recursion */
    if (op1->capacity < 32 || op2->capacity < 32) {
        mpi_mul_naive(rop, op1, op2);
        return;
    }

    size_t capacity =
        op1->capacity > op2->capacity ? op1->capacity : op2->capacity;

    size_t m = capacity / 2;

    mpi_t x0, x1, y0, y1;

    mpi_init(x0);
    mpi_init(x1);
    mpi_init(y0);
    mpi_init(y1);

    /* x = op1 */
    /* y = op2 */
    mpi_fdiv_r_2exp(x0, op1, 31 * m);
    mpi_fdiv_q_2exp(x1, op1, 31 * m);
    mpi_fdiv_r_2exp(y0, op2, 31 * m);
    mpi_fdiv_q_2exp(y1, op2, 31 * m);

    mpi_t z0, z1, z2;

    mpi_init(z0);
    mpi_init(z1);
    mpi_init(z2);

    mpi_mul_karatsuba(z2, x1, y1);
    mpi_mul_karatsuba(z0, x0, y0);

    mpi_t w0, w1;

    mpi_init(w0);
    mpi_init(w1);

    mpi_add(w0, x0, x1);
    mpi_add(w1, y0, y1);

    mpi_mul_karatsuba(z1, w0, w1);
    mpi_sub(z1, z1, z2);
    mpi_sub(z1, z1, z0);

    mpi_mul_2exp(z2, z2, 31 * 2 * m);
    mpi_mul_2exp(z1, z1, 31 * m);

    mpi_add(rop, z0, z1);
    mpi_add(rop, rop, z2);

    mpi_clear(w0);
    mpi_clear(w1);

    mpi_clear(z0);
    mpi_clear(z1);
    mpi_clear(z2);

    mpi_clear(x0);
    mpi_clear(x1);
    mpi_clear(y0);
    mpi_clear(y1);

    mpi_compact(rop);
}

void mpi_mul(mpi_t rop, const mpi_t op1, const mpi_t op2)
{
    mpi_mul_karatsuba(rop, op1, op2);
}

int mpi_cmp(const mpi_t op1, const mpi_t op2)
{
    size_t capacity =
        op1->capacity > op2->capacity ? op1->capacity : op2->capacity;

    if (capacity == 0)
        return 0;

    for (size_t n = capacity - 1; n != (size_t) -1; --n) {
        uint32_t r1 = (n < op1->capacity) ? op1->data[n] : 0;
        uint32_t r2 = (n < op2->capacity) ? op2->data[n] : 0;

        if (r1 < r2)
            return -1;

        if (r1 > r2)
            return +1;
    }

    return 0;
}

int mpi_cmp_u32(const mpi_t op1, uint32_t op2)
{
    size_t capacity = op1->capacity;

    if (capacity < ceil_div(32, 31))
        capacity = ceil_div(32, 31);

    for (size_t n = capacity - 1; n != (size_t) -1; --n) {
        uint32_t r1 = (n < op1->capacity) ? op1->data[n] : 0;
        uint32_t r2 = 0;

        switch (n) {
        case 0:
            r2 = (op2) & INTMAX;
            break;
        case 1:
            r2 = (op2 >> 31) & INTMAX;
            break;
        }

        if (r1 < r2)
            return -1;

        if (r1 > r2)
            return +1;
    }

    return 0;
}

uint64_t mpi_get_word_u64(const mpi_t op, size_t n)
{
    uint64_t r = 0;

    if (n + 0 < op->capacity)
        r |= (uint64_t) op->data[n + 0];

    if (n + 1 < op->capacity)
        r |= (uint64_t) op->data[n + 1] << 31;

    if (n + 2 < op->capacity)
        r |= (uint64_t) op->data[n + 2] << 62;

    return r;
}

/* Compute q = floor(n / 2^b).
 *
 * Extracts the "quotient" part when dividing a multi-precision integer n by
 * 2^b. Another way to think about this: it effectively performs a right-shift
 * of n by b bits (dropping the lower b bits).
 */
void mpi_fdiv_q_2exp(mpi_t q, const mpi_t n, mp_bitcnt_t b)
{
    size_t words = b / 31; /* shift by whole words/limbs */
    size_t bits = b % 31;  /* and shift by bits */

    size_t capacity = n->capacity >= words ? n->capacity - words : 0;

    mpi_t tmp;

    mpi_init(tmp);

    mpi_enlarge(tmp, capacity);

    if (bits == 0) {
        memcpy(tmp->data, n->data + words, capacity * 4);
    } else {
        for (size_t i = 0; i < tmp->capacity; ++i) {
            uint32_t r = (uint32_t) ((mpi_get_word_u64(n, i + words) >> bits) &
                                     INTMAX);

            tmp->data[i] = r;
        }
    }

    mpi_set(q, tmp);

    mpi_clear(tmp);
}

/* Compute r = n mod 2^b.
 *
 * Extracts the "remainder" part when dividing a multi-precision integer n by
 * 2^b. Another way to think about this: it keeps only the lower b bits of n,
 * discarding (zeroing) anything above that.
 */
void mpi_fdiv_r_2exp(mpi_t r, const mpi_t n, mp_bitcnt_t b)
{
    size_t words = b / 31; /* shift by whole words/limbs */
    size_t bits = b % 31;  /* and shift by bits */

    size_t capacity = words + 1;

    mpi_t tmp;

    mpi_init(tmp);

    mpi_enlarge(tmp, capacity);

    if (bits == 0) {
        size_t min = words < n->capacity ? words : n->capacity;
        memcpy(tmp->data, n->data, 4 * min);
        memset(tmp->data + min, 0, 4 * (tmp->capacity - min));
    } else {
        for (size_t i = 0; i < words; ++i) {
            tmp->data[i] = i < n->capacity ? n->data[i] : 0;
        }

        tmp->data[words] = words < n->capacity
                               ? n->data[words] & ((UINT32_C(1) << bits) - 1)
                               : 0;
    }

    mpi_set(r, tmp);

    mpi_clear(tmp);

    mpi_compact(r);
}

/* Retrieve a 32-bit word from the MPI, shifted by lshift bits. */
uint32_t mpi_get_word_lshift_u32(const mpi_t op, size_t n, size_t lshift)
{
    uint32_t r = 0;

    assert(lshift < 31);

    if (n < op->capacity + 0)
        r |= (op->data[n + 0] << lshift) & INTMAX;

    if (n < op->capacity + 1 && n > 0) {
        r |= op->data[n - 1] >> (31 - lshift);
    }

    return r;
}

/* Left-shift (multiply) a multi-precision integer by 2^op2 */
void mpi_mul_2exp(mpi_t rop, const mpi_t op1, mp_bitcnt_t op2)
{
    size_t words = ceil_div(op2, 31);
    size_t word_shift = op2 / 31;
    size_t bit_shift = op2 % 31;

    size_t capacity = op1->capacity + words;

    mpi_t tmp;

    mpi_init(tmp);

    mpi_enlarge(tmp, capacity);

    for (size_t i = 0; i < tmp->capacity; ++i) {
        tmp->data[i] = i >= word_shift ? mpi_get_word_lshift_u32(
                                             op1, i - word_shift, bit_shift)
                                       : 0;
    }

    mpi_set(rop, tmp);

    mpi_clear(tmp);

    mpi_compact(rop);
}

int mpi_testbit(const mpi_t op, mp_bitcnt_t bit_index)
{
    size_t word = bit_index / 31;
    size_t bit = bit_index % 31;

    uint32_t r = word < op->capacity ? op->data[word] : 0;

    return (r >> bit) & 1;
}

void mpi_setbit(mpi_t rop, mp_bitcnt_t bit_index)
{
    size_t word = bit_index / 31;
    size_t bit = bit_index % 31;

    mpi_enlarge(rop, word + 1);

    uint32_t mask = 1U << bit;
    rop->data[word] |= mask;
}

/* calculates how many bits are required to represent the MPI in base 2. */
size_t mpi_sizeinbase(const mpi_t op, int base)
{
    assert(base == 2); /* Only binary */

    /* find right-most non-zero word */
    for (size_t i = op->capacity - 1; i != (size_t) -1; --i) {
        if (op->data[i] != 0) {
            /* find right-most non-zero bit */
            for (int b = 30; b >= 0; --b) {
                if ((op->data[i] & (1U << b)) != 0)
                    return 31 * i + b + 1;
            }
        }
    }

    return 0;
}

/* Computes the quotient (q) and remainder (r) of n / d. */
void mpi_fdiv_qr(mpi_t q, mpi_t r, const mpi_t n, const mpi_t d)
{
    mpi_t n0, d0;
    mpi_init(n0);
    mpi_init(d0);
    mpi_set(n0, n);
    mpi_set(d0, d);

    if (mpi_cmp_u32(d0, 0) == 0) {
        fprintf(stderr, "Division by zero\n");
        abort();
    }

    mpi_set_u32(q, 0);
    mpi_set_u32(r, 0);

    size_t start = mpi_sizeinbase(n0, 2) - 1;

    for (size_t i = start; i != (size_t) -1; --i) {
        mpi_mul_2exp(r, r, 1);
        if (mpi_testbit(n0, i) != 0)
            mpi_setbit(r, 0);
        if (mpi_cmp(r, d0) >= 0) {
            mpi_sub(r, r, d0);
            mpi_setbit(q, i);
        }
    }

    mpi_clear(n0);
    mpi_clear(d0);
}

void mpi_gcd(mpi_t rop, const mpi_t op1, const mpi_t op2)
{
    if (mpi_cmp_u32(op2, 0) == 0) {
        mpi_set(rop, op1);
        return;
    }

    mpi_t q, r;
    mpi_init(q);
    mpi_init(r);

    mpi_fdiv_qr(q, r, op1, op2);

    mpi_gcd(rop, op2, r);

    mpi_clear(q);
    mpi_clear(r);
}

int main()
{
    printf("mpi_init, mpi_clear\n");
    {
        mpi_t r;
        mpi_init(r);
        mpi_clear(r);
    }

    printf("mpi_set_u32, mpi_get_u32\n");
    {
        mpi_t r;
        mpi_init(r);
        mpi_set_u32(r, UINT32_C(4294967295));
        assert(mpi_get_u32(r) == UINT32_C(4294967295));
        mpi_clear(r);
    }

    printf("mpi_set_u64, mpi_get_u64\n");
    {
        mpi_t r;
        mpi_init(r);
        mpi_set_u64(r, UINT64_C(0xFFFFFFFFFFFFFFFF));
        assert(mpi_get_u64(r) == UINT64_C(0xFFFFFFFFFFFFFFFF));
        mpi_clear(r);
    }

    printf("mpi_set_str\n");
    {
        mpi_t s;
        mpi_init(s);

        mpi_set_str(s, "1234567890", 10);
        assert(UINT64_C(1234567890) == mpi_get_u64(s));

        mpi_set_str(s, "18446744073709551615", 10);
        assert(UINT64_C(18446744073709551615) == mpi_get_u64(s));

        mpi_set_str(s, "0", 10);
        assert(UINT64_C(0) == mpi_get_u64(s));

        mpi_clear(s);
    }

    printf("mpi_cmp\n");
    {
        mpi_t r, s;
        mpi_init(r);
        mpi_init(s);

        mpi_set_str(r, "3433683820292512484657849089280", 10);
        mpi_set_str(s, "3433683820292512484657849089279", 10);

        assert(mpi_cmp(r, r) == 0);
        assert(mpi_cmp(r, s) > 0);
        assert(mpi_cmp(s, r) < 0);

        mpi_clear(r);
        mpi_clear(s);
    }

    printf("mpi_cmp_u32\n");
    {
        mpi_t r;
        mpi_init(r);

        mpi_set_str(r, "123456", 10);
        assert(mpi_cmp_u32(r, UINT32_C(123456)) == 0);
        assert(mpi_cmp_u32(r, UINT32_C(123455)) > 0);
        assert(mpi_cmp_u32(r, UINT32_C(123457)) < 0);

        mpi_clear(r);
    }

    printf("mpi_add_u32\n");
    {
        mpi_t r, s;
        mpi_init(r);
        mpi_init(s);

        mpi_set_str(r, "3433683820292512484657849089280", 10);
        mpi_add_u32(r, r, UINT32_C(2172748161));
        mpi_set_str(s, "3433683820292512484660021837441", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_clear(r);
        mpi_clear(s);
    }

    printf("mpi_add_u64\n");
    {
        mpi_t r, s;
        mpi_init(r);
        mpi_init(s);

        mpi_set_str(r, "3433683820292512484657849089280", 10);
        mpi_add_u64(r, r, UINT64_C(142393223512449));
        mpi_set_str(s, "3433683820292512627051072601729", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_clear(r);
        mpi_clear(s);
    }

    printf("mpi_add\n");
    {
        mpi_t r, s, t;
        mpi_init(r);
        mpi_init(s);
        mpi_init(t);

        mpi_set_str(r, "3433683820292512484657849089280", 10);
        mpi_set_str(t, "1144561273430837494885949696424", 10);
        mpi_add(r, r, t);
        mpi_set_str(s, "4578245093723349979543798785704", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_set_str(r, "3433683820292512484657849089280", 10);
        mpi_set_str(t, "42391158275216203514294433201", 10);
        mpi_add(r, r, t);
        mpi_set_str(s, "3476074978567728688172143522481", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_clear(r);
        mpi_clear(s);
        mpi_clear(t);
    }

    printf("mpi_sub_u32\n");
    {
        mpi_t r, s;
        mpi_init(r);
        mpi_init(s);

        mpi_set_str(r, "3433683820292512484657849089280", 10);
        mpi_sub_u32(r, r, 2);
        mpi_set_str(s, "3433683820292512484657849089278", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_set_str(r, "18446744073709551616", 10);
        mpi_sub_u32(r, r, 2);
        mpi_set_str(s, "18446744073709551614", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_clear(r);
        mpi_clear(s);
    }

    printf("mpi_sub\n");
    {
        mpi_t r, s, t;
        mpi_init(r);
        mpi_init(s);
        mpi_init(t);

        mpi_set_str(r, "3433683820292512484657849089280", 10);
        mpi_set_str(t, "1144561273430837494885949696424", 10);
        mpi_sub(r, r, t);
        mpi_set_str(s, "2289122546861674989771899392856", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_set_str(r, "423911582752162035142944332014", 10);
        mpi_set_str(t, "11445612734308374948859496924", 10);
        mpi_sub(r, r, t);
        mpi_set_str(s, "412465970017853660194084835090", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_clear(r);
        mpi_clear(s);
        mpi_clear(t);
    }

    printf("mpi_mul\n");
    {
        mpi_t r, s, t;

        mpi_init(s);
        mpi_init(r);
        mpi_init(t);

        mpi_set_str(s, "1853020188851841", 10);
        mpi_set_str(r, "22876792454961", 10);
        mpi_set_str(t, "42391158275216203514294433201", 10);
        mpi_mul(r, r, s);
        assert(mpi_cmp(r, t) == 0);

        mpi_set_str(
            s, "1797010299914431210413179829509605039731475627537851106400",
            10);
        mpi_set_str(r, "42391158275216203514294433201", 10);
        mpi_set_str(t,
                    "7617734804586639233928972772061556175042480140239519672395"
                    "9174586681921139518743586400",
                    10);
        mpi_mul(r, r, s);
        assert(mpi_cmp(r, t) == 0);

        mpi_set_str(s, "2147483648", 10);
        mpi_mul(s, s, s);
        mpi_set_str(t, "4611686018427387904", 10);
        assert(mpi_cmp(s, t) == 0);

        mpi_clear(s);
        mpi_clear(r);
        mpi_clear(t);
    }

    printf("mpi_fdiv_q_2exp\n");
    {
        mpi_t r, s;
        mpi_init(s);
        mpi_init(r);

        mpi_set_str(s, "42391158275216203514294433201", 10);
        mpi_fdiv_q_2exp(s, s, 23);
        mpi_set_str(r, "5053419861223245085989", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_set_str(s, "42391158275216203514294433201", 10);
        mpi_fdiv_q_2exp(s, s, 31);
        mpi_set_str(r, "19739921332903301117", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_set_str(s, "42391158275216203514294433201", 10);
        mpi_fdiv_q_2exp(s, s, 35);
        mpi_set_str(r, "1233745083306456319", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_set_str(
            s, "1797010299914431210413179829509605039731475627537851106400",
            10);
        mpi_fdiv_q_2exp(s, s, 31);
        mpi_set_str(r, "836798129563420643291054214122521243864426215895", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_set_str(s, "4611686018427387903", 10);
        mpi_fdiv_q_2exp(s, s, 31);
        mpi_set_str(r, "2147483647", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_set_str(s, "9223372036854775807", 10);
        mpi_fdiv_q_2exp(s, s, 31);
        mpi_set_str(r, "4294967295", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_set_str(s, "1144561273430837494885949696425", 10);
        mpi_fdiv_q_2exp(s, s, 31);
        mpi_set_str(r, "532977875988389130162", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_set_str(s, "1144561273430837494885949696425", 10);
        mpi_fdiv_q_2exp(s, s, 100);
        mpi_set_str(r, "0", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_set_str(s, "1144561273430837494885949696425", 10);
        mpi_fdiv_q_2exp(s, s, 200);
        mpi_set_str(r, "0", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_clear(s);
        mpi_clear(r);
    }

    printf("mpi_fdiv_r_2exp\n");
    {
        mpi_t r, s;
        mpi_init(s);
        mpi_init(r);

        mpi_set_str(s, "42391158275216203514294433201", 10);
        mpi_fdiv_r_2exp(s, s, 23);
        mpi_set_str(r, "6419889", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_set_str(s, "42391158275216203514294433201", 10);
        mpi_fdiv_r_2exp(s, s, 31);
        mpi_set_str(r, "316798385", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_set_str(s, "42391158275216203514294433201", 10);
        mpi_fdiv_r_2exp(s, s, 35);
        mpi_set_str(r, "28234085809", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_set_str(
            s, "1797010299914431210413179829509605039731475627537851106400",
            10);
        mpi_fdiv_r_2exp(s, s, 31);
        mpi_set_str(r, "820921440", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_set_str(s, "1144561273430837494885949696425", 10);
        mpi_fdiv_r_2exp(s, s, 31);
        mpi_set_str(r, "2111105449", 10);
        assert(mpi_cmp(s, r) == 0);

        mpi_clear(s);
        mpi_clear(r);
    }

    printf("mpi_mul_2exp\n");
    {
        mpi_t r, s;
        mpi_init(r);
        mpi_init(s);

        mpi_set_u32(r, 123456);
        mpi_mul_2exp(r, r, 89);
        mpi_set_str(s, "76415562745007953608973140099072", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_set_str(r, "532977875988389130162", 10);
        mpi_mul_2exp(r, r, 31);
        mpi_set_str(s, "1144561273430837494883838590976", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_set_str(r, "3", 10);
        mpi_mul_2exp(r, r, 1);
        mpi_set_str(s, "6", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_set_str(r, "3", 10);
        mpi_mul_2exp(r, r, 32);
        mpi_set_str(s, "12884901888", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_set_str(r, "2147483647", 10);
        mpi_mul_2exp(r, r, 1);
        mpi_set_str(s, "4294967294", 10);
        assert(mpi_cmp(r, s) == 0);

        mpi_clear(r);
        mpi_clear(s);
    }

    printf("mpi_fdiv_q_2exp, mpi_fdiv_r_2exp, mpi_mul_2exp\n");
    {
        mpi_t s, q, r;
        mpi_init(s);
        mpi_init(q);
        mpi_init(r);

        mpi_set_str(s, "1144561273430837494885949696425", 10);
        mpi_fdiv_q_2exp(q, s, 31);
        mpi_fdiv_r_2exp(r, s, 31);
        mpi_mul_2exp(q, q, 31);
        mpi_add(q, q, r);
        assert(mpi_cmp(q, s) == 0);

        mpi_clear(s);
        mpi_clear(q);
        mpi_clear(r);
    }

    printf("mpi_testbit\n");
    {
        mpi_t s;
        mpi_init(s);

        mpi_set_str(s, "4886718345", 10);
        assert(mpi_testbit(s, 0) == 1);
        assert(mpi_testbit(s, 10) == 1);
        assert(mpi_testbit(s, 31) == 0);
        assert(mpi_testbit(s, 32) == 1);
        assert(mpi_testbit(s, 33) == 0);
        assert(mpi_testbit(s, 100) == 0);

        mpi_clear(s);
    }

    printf("mpi_setbit\n");
    {
        mpi_t s;
        mpi_init(s);

        mpi_set_str(s, "0", 10);

        mpi_setbit(s, 1);
        assert(mpi_cmp_u32(s, 2) == 0);
        mpi_setbit(s, 0);
        assert(mpi_cmp_u32(s, 3) == 0);
        mpi_setbit(s, 31);
        assert(mpi_cmp_u32(s, UINT32_C(2147483651)) == 0);

        mpi_clear(s);
    }

    printf("mpi_sizeinbase\n");
    {
        mpi_t s;
        mpi_init(s);

        mpi_set_str(s, "49152", 10);
        assert(mpi_sizeinbase(s, 2) == 16);
        mpi_set_str(s, "4295016448", 10);
        assert(mpi_sizeinbase(s, 2) == 33);

        mpi_clear(s);
    }

    printf("mpi_fdiv_qr\n");
    {
        mpi_t n, d, q, r;
        mpi_init(n);
        mpi_init(d);
        mpi_init(q);
        mpi_init(r);

        mpi_set_str(n, "549755813889", 10);
        mpi_set_str(d, "1234", 10);
        mpi_fdiv_qr(q, r, n, d);
        assert(mpi_cmp_u32(q, 445507142) == 0);
        assert(mpi_cmp_u32(r, 661) == 0);

        mpi_clear(n);
        mpi_clear(d);
        mpi_clear(q);
        mpi_clear(r);
    }

    printf("GCD test\n");
    {
        mpi_t a, b, r;
        mpi_init(a);
        mpi_init(b);
        mpi_init(r);

        mpi_set_str(a, "2310", 10);
        mpi_set_str(b, "46189", 10);

        mpi_gcd(r, a, b);
        assert(mpi_cmp_u32(r, 11) == 0);

        mpi_clear(a);
        mpi_clear(b);
        mpi_clear(r);
    }

    return 0;
}