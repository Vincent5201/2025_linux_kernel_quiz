#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* This function calculates log_base(@num) through a naive, recursive approach.
 * @acc denotes the recursion depth, which effectively controls precision.
 * e.g.,
 * - If @num >= @base, the result is 1 plus the log of (@num / @base).
 * - If @num < @base, the result is (1 / @base) times the log of (@num^@base).
 *
 * This process recurses @acc times, partitioning the logarithm into
 * integer and fractional parts step by step.
 *
 * NOTE: This work method is mainly for educational purposes, yielding
 * performance.
 */
double logarithm(double base, double num, int acc)
{
    /* Stop recursion once we have reached the desired depth */
    if (acc <= 0)
        return 0.0;

    /* If num is large enough to "remove" one whole unit of log at once */
    if (num >= base)
        return 1.0 + logarithm(base, num / base, acc - 1);

    /* Otherwise, multiply 'num' by itself 'base' times
     * (which is effectively num^base).
     */
    double tmp = 1.0;
    for (int i = 0; i < (int) base; i++)
        tmp *= num;
    return (1.0 / base) * logarithm(base, tmp, acc - 1);
}

int main(void)
{
    double result = logarithm(2, 4024, 100);
    printf("%.18lf\n", result);

    return 0;
}