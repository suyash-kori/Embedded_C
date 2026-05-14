# Number Systems & Binary Fundamentals

## Why this matters for interviews 

Every embedded/firmware question, registers/protocols/memory has binary at its root. Companies like Qualcomm and Cadence interviews often start here to test fundamentals before throwing tricky problems at you.

## 1) Number Systems at a Glance

Base&emsp;Name&emsp;Digits&emsp;Example

2&emsp;Binary&emsp;0,1&emsp;0b1101

8&emsp;Octal&emsp;0-7&emsp;015

10&emsp;Decimal&emsp;0-9&emsp;13

16&emsp;Hex&emsp;0-9,A-F&emsp;0x0D


All four represent the same number 13 above. In C, prefix 
0b = binary
0 = octal
0x = hexadecimal
Octal is a classic trap - int x = 013 is 11, not 13!
Confirmation:- 1*(8^1) + 3*(8^0)
             = 8 + 3
             = 11

So Octal of 13 is->>

         8 | 13
           | 1 - 5 
(13) in octal - 15

Confirmation:-  1*(8^1) + 5*(8^0)
               = 8 + 5
               = 13


## 2) 2's complement - The Most important concept
Computers don't store negative numbers with a minus sign. They use 2's complement.

Rule:- 
-> Flip all bits, then add 1

         0000 0101  (5)
         1111 1010  (flip)
       +         1
        ------------
         1111 1011 = -5
        ------------

Why 2's complement and not just a sign bit?
-> Because addition and subtraction use the exact same hardware:-

        5 + (-5)
    =  0000 0101
      +1111 1011
      ------------
       0000 0000 (carry discarded)
      ------------
No separate subtraction circuit needed. Elegant

## 3) Signed vs Unsigned - Ranges

Type&emsp;8-bit range&emsp;32-bit range

Unsigned&emsp;0 to 255&emsp;0 to 4,294,967,295

Signed&emsp;-128 to 127&emsp;-2,147,483,648 to 2,147,483,647

Notice: one more negative than positive. Because zero takes a slot on the positive side. This is why INT_MIN has no positive equivalent, -INT_MIN overflows!

## 4) Endianness - Critical in Embedded

When a multi-byte value is stored in memory, which byte goes first?

Value: 0x12345678

Big-endian&emsp;-> [12][34][56][78]  (MSB first - network byte order)
Little-endian&emsp;-> [78][56][34][12]  (LSB first - x86, ARM default)

REAL INTERVIEW SCENARIO:-

You read a 32-bit sensor register over SPI and get garbage. Endianness mismatch is the first thing to check.
Always use ntohl()/htonl() when crossing network <-> host boundaries.

## Q&A - Test Yourself

Q1- TRICKY: What is int x = 0x8000 0000 for a 32-bit signed int? What is -x?
Ans: - 
0x8000 0000 = INT_MIN
            = i.e -2,147,483,648.
And -x = -(INT_MIN) is undefined behaviour in C, the positive equivalent doesn't exist in 32-bit signed range.In practice it wraps back to INT_MIN itself on most hardware.

Q2- CONCEPT: How many bytes does 0xDEADBEEF occupy, and what is it in decimal?
Ans: - 
4 bytes(32 bits). Decimal = 3,735,928,559.
Commonly used as a magic number/memory poison value in embedded debugging to mark uninitialized memory.

Q3- INTERVIEW: A firmware engineer reads a 16-bit value 0x0102 from a big-endian sensor on a little-endian MCU. What value does the MCU interpret it as without conversion?
Ans: -
The MCU reads bytes as [02][01] -> interprets as 0x0201 = 513 instead of the correct 258. Always byte swap when crossing endian boundaries.
Use -> __builtin_bswap16(val) in GCC.

Q4- CLASSIC: Without running it, what does this print?

int x = -1;
unsigned int y = x;
printf("%u", y);
Ans: -
4294967295(=2^32 - 1 = 0xFFFF FFFF)
Signed -1 in 2's complement is all 1s. Reinterpreted as unsigned, that's the max unsigned value. This implicit conversion is a very common bug in Embedded C.


## Module 1 Summary
- Binary, Octal, Decimal, Hex - same value, different representations
- 2's complement : flip + add 1; enables single hardware adder
- Signed range is asymmetric, one more negative than posiive
- Endianness defines byte order in memory, always matters in embedded
- Implicit signed <-> unsigned conversion is a silent killer in C