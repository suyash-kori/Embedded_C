#include <stdio.h>

unsigned long long factorial(int n) {
    if (n == 0 || n == 1) {
        return 1; // Base case: factorial of 0 or 1 is 1
    }
    return n * factorial(n - 1); // Recursive case
}
int main() {
    int number = 5; // You can change this value to test with different numbers
    unsigned long long result = factorial(number);
    printf("Factorial of %d is %llu\n", number, result);
    return 0;
}