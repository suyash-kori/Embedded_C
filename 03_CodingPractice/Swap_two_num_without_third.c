/* Write a program to swap two numbers without using a third variable
Constarints:- 
1. No third/temporary variable allowed 
2. Try it with arithmetic(+/-) first, then think about why XOR (^) approach is 
generally prefered in interviews, there's a subtle bug that arithmetic swap can have that XOR avoids
(hint:think about very large numbers and overflow)
3. Also handle the edge cases: what happens if someone calls swap(&x, &x),
same variable passed twice? One of the two approaches breaks here.Worth thinking about which, and why*/  

#include <stdio.h>

void swap_arithmetic(int *a, int *b) {
    *a = *a + *b;
    *b = *a - *b;
    *a = *a - *b;
}

void swap_xor(int *a, int *b) {
    *a = *a ^ *b;
    *b = *a ^ *b;
    *a = *a ^ *b;
}

int main() {
    int x = 10, y = 20;
    
    printf("Before swap: x = %d, y = %d\n", x, y);
    
    // Using arithmetic swap
    swap_arithmetic(&x, &y);
    printf("After arithmetic swap: x = %d, y = %d\n", x, y);
    
    // Reset values
    x = 10; y = 20;
    
    // Using XOR swap
    swap_xor(&x, &y);
    printf("After XOR swap: x = %d, y = %d\n", x, y);
    
    return 0;
}  

/* 
1. Overflow issue with arithmetic swap

*a = *a + *b; // This can cause overflow if a and b are large enough
If both a and b are large integers, their sum can exceed the maximum value that can be stored 
in an integer type, leading to undefined behavior. 
This is why the XOR method is generally preferred in interviews, as it avoids this overflow
issue. XOR never produces a value outside the range of the data type, since it's a bitwise operation
not arithmetic. No overflow possible.

2. Edge case: swapping same variable (a == b) 
This is where XOR swap actually breaks, while arithmetic swap survives.

Walk through XOR swap with a and b both pointing to the same memory location:
*a = *a ^ *b; // This sets *a to 0, since x ^ x = 0
*b = *a ^ *b; // Now *b is also 0, since *a is 0 and *b was originally x, so 0 ^ x = 0
*a = *a ^ *b; // Now *a is 0, since both *a and *b are 0
Arithmetic swap survives this edge case because it isn't self cancelling like XOR. If you call 
swap_arithmetic(&x, &x), it will just add x to itself and then subtract it back, 
leaving x unchanged.

The real production answer: most engineers would just say "add a guard clause":

void swap_xor(int *a, int *b) {
    if (a == b) return;   // same address, nothing to do
    *a = *a ^ *b;
    *b = *a ^ *b;
    *a = *a ^ *b;
}
*/
