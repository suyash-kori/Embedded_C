# Pointers Arithmetic & Arrays  

## The fundamental rule of pointer arithmetic  

When you do math on a pointer, the compiler automatically scales by the size of the type it points to. This is the single most important thing to understand about pointer arithmetic.  

int *p = (int*)0x2000;  

p + 1 // NOT ox2001, it's 0x2004 (moved by sizeof(int) = 4 bytes)  
p + 2 // 0x2008  
p + 3 // 0x200C  

This makes sense when you think about it, if "p" points to an "int" at "0x2000", the next "int" is at "0x2004", not "0x2001". The compiler knows this and does the scaling for you.  

char *cp = (char*)0x2000;  
cp + 1 // 0x2001 - char is 1 byte, so advances by 1  

double *dp = (double*)0x2000;  
dp + 1 // 0x2008 - double is 8 bytes, so advances by 8  

This is why char* and uint8_t* are used for raw byte level memory operations in embedded, because they advance exactly one byte at a time.  

## Arrays in memory  

An array in C is a contiguous block of memory. When you declare:  

int arr[5] = {10, 20, 30, 40, 50};  

Memory looks like this (on a 32-bit system):  

Address--------------Value-------------Element  
0x2000----------------10-----------------arr[0]  
0x2004----------------20-----------------arr[1]  
0x2008----------------30-----------------arr[2]  
0x200C----------------40-----------------arr[3]  
0x2010----------------50-----------------arr[4]  

Every element is exactly sizeof(int) = 4 bytes apart. The array name "arr" refers to the starting address 0x2000.  

## The array-pointer relationship, and the key distinction  

This is where most people get confused, so read carefully.  

int arr[5] = {10, 20, 30, 40, 50};  
int *p = arr;  // valid, arr decays to &arr[0]  

Array name decays to a pointer to it's first element. When you use "arr" in most expressions, the compiler treats it as &arr[0] -> a pointer to the first element.  

But, and this is critical, "arr" is not a pointer variable. It has no storage. You cannot do this:  

arr = p;   // ERROR - arr is not an assignable variable  
arr++;     // ERROR - cannot increment an array name  

Whereas "p" is real pointer variable stored in memory, you can reassign it, increment it, and point it elsewhere.  

p++;  // fine - p now points to arr[1]  
p = arr + 3;  // finr - p now points to arr[3]  

This distinction is a very common interview question.  

arr[i] and *(arr + i) are identical  

The compiler translates array subscript notation into pointer arithmetic. These four expressions are exactly the same, the compiler generates identical machine code for all of them:  

arr[i]  
*(arr + i)  
*(i + arr)  // addition is commutative  

int arr[5] = {10, 20, 30, 40, 50};  

printf("%d\n", arr[2]);  // 30  
printf("%d\n", *(arr + 2)); // 30  

This equivalence is not just a curiosity, it tells you that there is no "array access" instruction at the hardware level.  
It's always pointer arithmetic and dereference. On an embedded processor, arr[i] compiles to: multiply "i" by element size, add base address, load from that address.  

## Pointer increment and decrement  

int arr[5] = {10, 20, 30, 40, 50};  
int *p = arr;  // points to arr[0]  

printf("%d\n", *p);  // 10  
p++;  // advance to arr[1]  
printf("%d\n", *p);  // 20  
p++;  
printf("%d\n", *p);  // 30  

p--;  // back to arr[1]  
printf("%d\n", *p);  // 20  

A very common pattern in embedded, iterating over a buffer with a pointer:  

uint8_t buf[256];  
uint8_t *ptr = buf;  
uint8_t *end = buf + 256;  

while (ptr < end)  
{  
    *ptr++ = 0xFF;  // write 0xFF and advance pointer  
}  

*ptr++ is a critical idiom. The ++ is post-increment, so it increments "ptr" after the dereference. The precedence is: dereference "ptr" (get/set value), then increment "ptr". You'll see this everywhere in embedded C for copying and initializing buffers.  

## Pre vs Post increment with pointers, a common trap  

int arr[3] = {10, 20, 30};  
int *p = arr;  

*p++;  // dereferences p (gets 10), THEN increments p, p now points to arr[1]  

*++p  // increments p FIRST, THEN dereferences, gets arr[1] if p was at arr[0]  

(*p)++  // dereferences , then increments the VALUE at that address  

++*p  // increments the VALUE at address p (same as (*p)++ but pre)  

## Pointer subtraction  

You can subtract two pointers that point into the same array. The result is the number of elements between them, not bytes.  

int arr[5] = {10, 20, 30, 40, 50};  
int *p1 = &arr[1];  // points to arr[1]  
int *p2 = &arr[4];  // points to arr[4]  

ptrdiff_t diff = p2 - p1;  // 3 - three elements apart  

The result type is "ptrdiff_t" (a signed integer type from <stddef.h>). Internally, the compiler computes (address of p2 - address of p1) / sizeof(int).  

Never subtract pointers from different arrays, undefined behaviour. The addresses may be close in memory by coincidence and give a meaningless result, or worse, appear to work.  

## Pointer comparison  

You can compare pointers using <,>,<=,>=,==,!= - but only meaningfully when they point into the same array.  

int arr[5];  
int *p = arr;  
int *end = arr + 5; // one past the last element, valid address but don't dereference  

while(p < end)  
{  
    *p++ = 0;  
}  

"arr + 5" is a pointer to one-past-the-end. C explicitly allows this address to exist and be compared against, you just can't dereference it. This is the standard idiom for end-of-array sentinel.  

## Passing arrays to functions  

This is where the array-pointer relationship becomes practically important.  

Arrays are never passed by value in C. When you pass an array to a function, you're passing a pointer to it's first element. The function receives a pointer, not a copy of the array.  

void print_array(int *arr,int len)  
{  
    for(int i = 0; i < len; i++)  
    {  
        printf("%d ", arr[i]);  
    }  
}  

int arr[5] = {1,2,3,4,5};  
print_array(arr, 5);  // arr decays to &arr[0]  

These three function signatures are identical to the compiler:  

void foo(int *arr, int len);  
void foo(int arr[], int len);  
void foo(int arr[5], int len);  // the 5 is ignored!  

All three say "arr is a pointer to int". In the third form, the 5 is completely ignored by the compiler. This is a common source of confusion, you cannot pass the array size information through the type in C. You must pass it as a separate parameter.  

Consequence: "sizeof(arr)" inside the function gives "sizeof(int*)", not the size of the original array. This is the "sizeof array decay" bug:  

void foo(int arrr[])  
{  
    printf("%zu\n", sizeof(arr));  // prints 4 or 8 - size of pointer, NOT array!  
}  

int main(void)  
{  
    int arr[10];  
    printf("%zu\n", sizeof(arr));  // prints 40, correct, 10 * 4  
    foo(arr);  

}  

## 2D arrays in memory, row-major layout  

A 2D array is stored in row-major order, all elements of row 0, then all of row 1, and so on, contiguously in memory.  

int matrix[3][4] = {  
    {1, 2, 3, 4},  // row 0  
    {5, 6, 7, 8},  // row 1  
    {9, 10, 11, 12}// row 2  
};  

Memory layout:  

[1][2][3][4][5][6][7][8][9][10][11][12]  
^row0--------^row1-------^row2---------  

Accessing matrix[r][c] is equivalent to *(*(matrix + r) + c), which the compiler translates to:  

base_address + (r * number_of_columns + c) * sizeof(int)  

For matrix[2][1]:  

base + (2*4 + 1)*4 = base + 36 bytes  

When passing a 2D array to a function, you must specify the column count because the compiler needs it to do the row arithmetic:  

When passing a 2D array to a function, you must specify the column count because the compiler needs it to do the row arithmetic:  

void print_matrix(int arr[][4]. int rows)  // column count mandatory  
{  
    for (int r = 0; r < rows; r++)  
        for (int c = 0; c < 4; c++)  
            printf("%d ", arr[r][c]);  
}  

## A complete practical example  

This is the kind of code you'd write in an actual embedded firmware, processing a received UART buffer:  

#include <stdint.h>  

#include <stddef.h>  

/* Find the first occurence of a byte value in a buffer */  

uint8_t *find_byte(uint8_t *buf, size_t len, uint8_t target)  
{  
    uint8_t *end = buf + len;  
    for (uint8_t *p = buf; p < end; p++)  
    {  
        if (*p == target)  
            return p;  // return pointer to found location  
    }  
    return NULL;  // not found  
}  

/* Copy len bytes from src to dst */  

void mem_copy(uint8_t *dst, const uint8_t *src, size_t len)  
{  
    while (len--)  
    {  
        *dst++ = *src++;  // copy byte and advance both pointers  
    }  
}  

/* Usage */  
uint8_t rx_buf[64] = { 0x01, 0x02, 0xAA, 0x03, 0x04 };  

uint8_t *found = find_byte(rx_buf, 64, 0xAA);  
if (found != NULL)  
{  
    ptrdiff_t offset = found - rx_buf;  // how many bytes from start  
    // offset == 2  
}  

What is size_t?  

"size_t" is an unsigned integer type, defined in <stddef.h> (which is already included). It is the standard type used to represent sizes and counts of objects in memory.  

Property---------------Detail  

Signed?---------No, always unsigned (can't be negative)  

Size--Platform dependent:32-bit on 32-bit systems,64-bit on 64-bit system

Purpose------Represents memory sizes, array lengths, loop counts  

Defined in---------<stddef.h>,<stdlib.h>,<string.h>  

## Question and Answer  

Q1] If int "*p = arr" and "arr" is at 0x1000, what is "p+3" on a 32-bit system?  

Answer:-  

0x100C. Pointer arithmetic scales by sizeof(int) = 4, so "p + 3 = 0x1000 + 3*4 = 0x1000 + 12 = 0x100C".  

Q2] What is the difference between "arr" and "&arr"?  

Answer:-  

Same numerical address, but different types. "arr" decays to "int*", pointer to the first element. "&arr" is "int(*)[5]", pointer to the entire array. This matters for arithmetic: "arr+1" advances 4 bytes (to next int), but "&arr + 1" advances 20 bytes (past the entire 5 element array).  

int arr[5];  
printf("%p\n", arr);  // 0x2000  
printf("%p\n", arr + 1);  // 0x2004  
printf("%p\n", &arr);  // 0x2000 - same address  
printf("%p\n", &arr + 1);  // 0x2024 - jumped 20 bytes!  

Q3] Can you add two pointers?  

Answer:-  

No. Adding two pointers is undefined behaviour in C, it makes no semantic sense. Subtracting two pointers (from the same array) gives the number of elements between them, which is meaningful. Addition is not defined.  

Q4] Why does sizeof(arr)  give the wrong answer inside a function?  

Answer:-  

Because the array decays to a pointer when passed to a function. Inside the function, the parameter is a pointer, not an array. "sizeof" on a pointer gives the pointer size (4 bytes or 8 bytes), not the original array size. Always pass the length as a separate parameter.  

Q5] What does "*ptr++" do, and how is it different from (*ptr)++ ?  

Answer:-  

*ptr++ -> dereference "ptr"(reads the value), then advances the pointer to the next element. The value at the original address is unchanged.  

(*ptr)++ -> dereferences "ptr" and increments the value stored at that address. The pointer iself doesn't move.  

int arr[3] = {10, 20, 30};  
int *p = arr;  

int a = *ptr;  // a = 10, p now points to arr[1]  
int b = (*p)++; // b = 21, arr[1] becomes 21, p still points to arr[i]  

Q6] What is "ptrdiff_t" and see it instead of "int"?  

Answer:- "ptrdiff_t" is the signed integer type guranteed to hold the result of pointer subtraction. On a 32-bit system it's typically int, but on a 64-bit system it's 64-bit signed integer. Using int for pointer differences would overflow on 64-bit systems with large array. Always use "ptrdiff_t" from <stddef.h>  


