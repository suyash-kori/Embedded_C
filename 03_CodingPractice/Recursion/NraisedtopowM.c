#include <stdio.h>
unsigned long long power(int base, int exponent) {
    if (exponent == 0) {
        return 1; // Base case: any number to the power of 0 is 1
    }
    return base * power(base, exponent - 1); // Recursive case
}
int main() {
    int base = 2; // You can change this value to test with different bases
    int exponent = 10; // You can change this value to test with different exponents
    unsigned long long result = power(base, exponent);
    printf("%d raised to the power of %d is %llu\n", base, exponent, result);
    return 0;
}