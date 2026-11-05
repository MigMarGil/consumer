#include <stdio.h>

int main(void) {
    double pi = 0.0;
    double sign = 1.0;
    long i = 1;
    long n = 10000000;

    while (i < n * 2) {
        pi += sign * (4.0 / (double)i);
        sign *= -1.0;
        i += 2;
    }

    printf("%g\n", pi);
    return 0;
}
