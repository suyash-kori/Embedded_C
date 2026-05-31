# Pointer Fundamentals & Memory Model  

## What is a pointer?  

A pointer is a variable that stores a memory address. That's it. But the implications of that simple idea touch everything in C.  

Every variable you declare lives somewhere in memory, it has an address. A pointer let's you store that address and work with it directly. This is what gives C its power, and also its danger.  

int x = 10;  
int *p = &x;  

Here, x lives at some address in memory, say 0x2000. The "&x" expression gives you that address. "p" stores it. Now p holds 0x2000, and "*p" means "go to address 0x2000 and read/write whatever's there."  

### Memory: the mental model you must have  

Before anything else, you need to visualize memory as a long array of bytes, each with a unique address.  

Address &emsp;&emsp;  Value  

0x1FF0----------------[some other data]  
0x2000----------------10  <- x lives here  
0x2004----------------0x2000  <- p lives here (stores address of x)  
ox2008----------------[something else]  

On a 32-bit system, int is 4 bytes, so x occupies 0x2000 through 0x2003. The pointer p also occupies 4 bytes (because all pointers are 4 bytes on a 32-bit system), and it's value is 0x2000.  

This memtal model is non-negotiable in embedded work. You're often dealing with exact addresses, peripheral registers, DMA buffers, flash memory. You need to see memory as physical thing.  

## The two fundamental pointer operators  

#### "&" -> address of operator  

Gives you the address of a variable.  

int x = 10;  
printf("%p\n", (void*)&x);  // prints something like 0x7ffd2a30  

#### "*" -> dereference operator  

Goes to the address stored in a pointer and reads or writes the value there.  

int x = 10;  
int *p = &x;  

printf("%d\n", *p);  // 10 - reads value at the address p holds  

*p = 99;  // writes 99 to the address p holds  
printf("%d\n", x);  // 99 - x itself changed  

This is the core of why pointers are useful, through p, you can modify x from anywhere in the program.  

## Pointer declaration syntax -> the confusing part  

int *p;   // p is a pointer to an int  
char *c;  // c is a pointer to a char  
float *f; // f is a pointer to a float  
void *v;  // v is a pointer to "nothing" - a generic pointer  

the * here is part of the declarator, not the type. This trips everyone up:  

int *p, q;  // p is a pointer to int, but q is just a plain int!  
int *p, *q;  // both p and q are pointers to int  

The "int*" style "(int* p)" looks cleaner but is misleading when declaring multiple variables. In embedded codebases you'll see "int *p" far more often.  

## Pointer size, critical for embedded  

On any given architecture, all pointers are the same size, regardless of what they point to.  

Architecture---------------------Pointer size  

8-bit AVR (Arduino UNO)------------2 bytes  

32-bit ARM Cortex-M----------------4 bytes  

64-bit Linux/desktop---------------8 bytes  

int *p1;  
char *p2;  
double *p3;  
void *p4;  

// On a 32-bit ARM:  
sizeof(p1) == 4  // true  
sizeof(p2) == 4  // true  
sizeof(p3) == 4  // true  
sizeof(p4) == 4  // true  

This is a very common interview question. The pointer size is determined by the address bus width of the processor, not by what it points to.  

## NULL pointer  

NULL is a pointer that points to nothing. It's defined as (void*)0 or just 0 depending on the header.  

int *p = NULL;  // p doesn't point to anything valid  

if (p != NULL)  
{  
    *p = 10;  // safe only dereference if not NULL  
}  

Dereferencing NULL is undefined behaviour. On embedded systems (Cortex-M), it typically causes a HardFault because address 0 is often not mapped to valid RAM. On Linux, it causes a segfault. Either way, your program is dead.  
In embedded firmware, you'll always see NULL checks before dereferencing function return values like malloc(), configuration pointers passed from HAL layers, and so on.  

## Pointer to Pointer  

A pointer to a pointer stores the address of anaother pointer.  

int x = 10;  
int *p = &x;  // p holds address of x  
int **pp = &p;  // pp holds address of p  

Visualizing it:  

x (0x2000) = 10  
p (0x2004) = 0x2000  <- points to x  
pp(0x2008) = 0x2004  <- points to p  

Dereferencing:  

*pp   // gives p (the address 0x2000)  
**pp  // gives x (the value 10)  

**pp = 99;  // changes x to 99 - two hops thrugh memory  

You'll use pointer-to-pointer when a function needs to modify a pointer itself (like modifying the head of a linked list), and in char **argv (array of strings).  

## The five memory segments, know this cold  

Every C program's memory is divided into regions. In embedded, these map directly to physical hardware (Flash, SRAM, etc).  

Text segment (Flash in embedded) Your compiled code lives here. Read-only. On a microcontroller this is typically in Flash memory starting at address 0x80000000 (STM32) or 0x00000000 (LPC series).  

Data segment (SRAM in embedded) Initialized global and static variables. int x = 5; at global scope. The initial values are stored in Flash and copied to SRAM at startup by the startup code.  

BSS segment (SRAM in embedded) Uninitialized global and static variables. int x; at global scope. The startup code zeroes this region before main() runs.  

Stack (SRAM in embedded) Local variables and function call frames. Grows downward on most architectures. On Cortex-M, the stack pointer starts at the top of SRAM. Stack size is fixed, overflow is a serious bug in embedded (no OS to catch it, it silently corrupts other data).  

Heap (SRAM in embedded) Dynamic memory from malloc()/free(). Grows upward. On many embedded systems, the heap and stack grow toward each other and can collide, another reason dynamic allocation is avoided.  

int global_init = 5;  // data segment  
int global_uninit;    // BSS segment  

void foo(void)  
{  
    int local = 10; // stack  
    int *p = malloc(4);  // p on stack, *p on heap  
    static int s = 0;  // data segment (not stack!)  
}  
The "static" keyword inside a function is important that variable lives in the data/BSS segment, not the stack. It persist across function calls. This is used a lot in embedded for state machines, counters in ISRs, etc.  

## Putting it all together, a complete example  

#include <stdio.h>  
#include <stdlib.h>  

int global = 100;  // data segment  

int main(void)  
{  
    int local = 200; // stack  
    int *p = &local;  // p is on stack, holds stack address  

    printf("local value : %d\n", local);  // 200  
    printf("local address : %p\n", (void*)&local);  
    printf("p value : %p\n", (void*)p);  // same as &local  
    printf("p address : %p\n", (void*)&p); // different-p itself is on stack  
    printf("via pointer : %d\n", *p);  // 200  

    *p = 999;  
    printf("local after : %d\n", local);  // 999 - changed via pointer  

    // pointer to pointer  
    int **pp = &p;  
    printf("via pp    : %d\n", **pp);  // 999  

    return 0;  
}  

## QUESTION AND ANSWER  

Q1] What is the size of a pointer on a 32-bit ARM Cortex-M microcontroller?  

Answer:-  

Always 4 bytes. Doesn't matter if it's char*, int*, double* or void*. The pointer holds an address, and on a 32-bit bus,all addresses are 32-bit = 4 bytes.  

Q2] What happens when you dereference a NULL pointer?  

Answer:-  

Undefined behaviour. On Cortex-M, address 0x00000000 is the vector table (in Flash), so writing through a NULL pointer can corrupt vectors, catastrophic. Most production firmware either enables the MPU to trap NULL dereferences or uses a watchdog that catches the resulting HardFault.  

Q3] What is the difference between int *p and int* p?  

Answer:-  

To the compiler, absolutely nothing. They're identical. The difference is human-intent: "int* p" suggests the type is "int pointer", but "int *p,q" makes it clear that only p is a pointer. Most C style guides (including MISRA for embedded) prefer int *p.  

Q4] How do you correctly print a pointer value?  
printf("%p\n", (void*)p);  

Answer:-  

The "*p" format specifier expects a void*, so always cast. The output is implementation defined (usually hex), but this is the correct portable form.  

Q5] What is the difference between a pointer variable and the address it holds?  

Answer:-  

The pointer variable itself occupies memory (4 bytes on 32-bit). It has it's own address. The value it stores is another address. "&p" is the address of the pointer. p is the address it holds. *p is the value at that address. Three different things.  

## Summary  

Concept-----------Key point  

&x -----------------Address of x  
*p -----------------value at address stored in p  
pointer size--------Always sizeof(void*)-architecture dependent  
NULL----------------Points to nothing-never dereference  
int **pp------------Pointer to a pointer  
Stack---------------Local variables, fast, fixed size, grows down  
Heap----------------Dynamic allocation, manual management  
BSS/Data------------Global and static variables  

