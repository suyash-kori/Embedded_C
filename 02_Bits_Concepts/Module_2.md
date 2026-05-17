# Bit Manipulation Mastery
Bitwise operators DeepDive

### Why this matters for Interviews

Bitwise operators are the bread and butter of every embedded interview.
Qualcomm and Cadence will throw expressions at you and expect instant evaluation, no calculator, no compiler. Master these cold.

## The Six Operators

a = 0b1100 (12)
b = 0b1010 (10)

AND(1 only if both are 1)
a & b ----> 0b1000(8)

OR(1 if EITHER is 1)
a | b -----> 0b1110(14)

XOR(1 if bits are different)
a ^ b ------> 0b0110(6)

NOT(flip every bit)
~a -------> 0b0011 (-13)

SHL(shift left (x 2))
a << 1 -----> 0b11000(24)

SHR(shift right (/ 2))
a >> 1 ------> 0b0110

1) & -> intersection (both must agree on 1)
2) | -> union (either can bring a 1)
3) ^ -> difference (disagree = 1)

## AND - Used for masking & Checking

x & 0x0F       // extract lower nibble
x & OxFF       // extract lower byte
x & 1          // check if odd (LSB)

Practical embedded use:

if (STATUS_REG & (1 << 3))
&emsp;transmit_ready();

## OR - Used for setting bits
 1) x | 0x80      // force bit 7 high
 2) x | (1 << n)  // set bit n

Practical embedded use:

GPIOA->ODR |= (1 << 5); // drive pin 5 HIGH

## XOR - The magic operator

XOR has three golden properties:
a = 1010
b = 1110
1) a ^ a = 0 (self-cancellation) 
     1010
     1010
    -------
     0000
    -------
2) a ^ 0 = a (identity)
     1010
     0000
    ------
     1010
    ------
3) a ^ b ^ b = a (undo)
      1010 -> a
      1110 -> b
    --------
      0100
      1110 -> b
    --------
      1010 -> a

Used for: swap without temp, toggling bits, finding unique elements, simple encryption, parity calculation.

  x ^= (1 < n)  // toggle bit n

## NOT (~) - Flip Everything

~0x00 -------> 0xFF  // all bits set
~0xFF -------> 0x00  // all bits clear
~0    -------> -1    // signed: all 1s = -1

Most common use: clearing bits
x &= ~(1 << n);   // clear bit n safely
~(1 << n) creates a mask with all 1s except bit n.

## Left Shift (<<) - Multiply by powers of 2

x << 1 = x * 2
x << 2 = x * 4
x << n = x * 2^n

Examples:- 
1) 1 << 0 = 1
2) 1 << 4 = 16
3) 1 << 8 = 256

Trap: Shifting into or past the sign bit of a signed integer -> undefined behavior in C.

int x = 1 << 1;  // Undefined behaviour: (1u << 31) instead
Always use 1u or (uint32_t)1 when building bit masks.

## Right shift (>>) - Two different behaviours

This is where most people get caught in interviews

Logical shift (unsigned): fills with 0
Arithmetic shift (signed): fills with sign bit

unsigned int a = 0x8000 0000;
a >> 1 ------>   0x4000 0000 // logical: fills 0

signed int b = 0x8000 0000; // = -2147483648
b >> 1 ------> 0xC000 0000; // arithmatic: fills 1 (sign extended)

C standard says: 
right shift on signed negative is implementation defined.
Most compilers do arithmatic shift, but never rely on it in portable code.

Safe pattern:
// Always cast to unsigned before right-shifting
uint32_t val = (uint32_t)signed_val >> n;

## Operator Precedence - The #1 Bug Source

This catches even experienced engineers:

     if (x & 0x01 == 1)   // WRONG

"==" has higher precedence than &. so this evaluates as:

if(x & (0x01 == 1))    // = x & 1..accidentally works here
if(x & 0x02 == 1)      // = x & (0x02 == 2) = x & 0 = always false!

Always parenthesize bitwise expressions:

if((x & 0x01) == 1)    // CORRECT
if((x & 0x02) == 2)    // CORRECT

Precedence order (high -> low):

~ (highest)
<< >>
&
^
| (lowest among bitwise)

## Q&A - Test Yourself

Q1) EVALUATE: Without a calculator, what is 0xA5 & 0x0F?
Ans:-
0xA5 = 1010 0101
0x0F = 0000 1111
      -----------
       0000 0101 = (0x05)
      -----------
AND with 0x0F always extracts the lower nibble

Q2) TRICKY: What does ~0 give on a 32-bit system?
Ans:- (-1)
~0 flips all bits of 0x00000000 -> 0xFFFFFFFF.
As a signed int, that's -1 in 2's complement.
This is how memset(buf, 0xFF, size) works internally, settings all bits is the same as setting to -1 signed, or 255 unsigned per byte

Q3) INTERVIEW: What is the output?
int x = 5;
x = x | x << 1;
printf("%d", x);
Ans:-
5      = 0b0101
5 << 1 = 0b1010 = 10
        ---------
         0b1111 = 15
        ---------
"<<" has higher precedence than |, so no extra parens needed here, but always add them for readability.

Q4) TRICKY: True or False: x << 32 is 0 for a 32 bit integer.
Ans:- FALSE
It's undefined behaviour in C. 
Shifting by an amount >= bit width of the type is Undefined behaviour.
The hardware may give 0, or it may give x unchanged (x86 masks shift count to 5 bits, so x << 32 = x << 0 = x).
Always guard:
if(n < 32)
&emsp;result = x << n;

Q5) Embedded: A UART status register is at 0x40011000. Bit 5 = TX empty, Bit 6 = RX ready. Write code to wait until RX is ready,then read data register at 0x40011004
Ans:-

#define UART_SR (*((volatile uint32_t*)0x40011000))
#define UART_DR (*((volatile uint32_t*)0x40011004))
#define RX_READY (1 << 6)

while(!(UART_SR & RX_READY)); // poll bit 6
unit8_t data = UART_DR & 0xFF; // read data

key points: volatile is mandatory, mask DR to 8 bits, always poll before read.
         
