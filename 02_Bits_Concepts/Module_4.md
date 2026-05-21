# Bit Manipulation Mastery  

### Counting Bits & Parity  

## Why this matters for interviews  

Popcount and parity questions are extremely common in Qualcomm, Cadence, and any semiconductor companies. They test algorithmic thinking AND hardware knowledge simultaneously. Interviewers love asking "now give me a faster version" repeatedly until you reach the optimal solution.  

1] Count Set Bits - 3 levels of Solutions  

### Level 1 - Naive 0(32) - Every Begineer knows this  

int countBits_naive(uint32_t x)  
{
    int count = 0;  
    while(x)  
    {
        count += (x & 1); // check LSB
        x >>= 1;          // shift right  
    }
    return count;
}
Loops exactly 32 times regardless of how many bits are set.  
Gets the job done but an interviewer will immediately ask - can you do better?  

### Level 2 - Brian Kernighan's Algorithm 0(k)  

k = number of set bits. If the number is sparse (few 1s), this is much faster.  

int countBits_kernighan(uint32_t x)  
{
    int count = 0;
    while(x)  
    {
        x &= (x-1);  //clear lowest set bit  
        count++;
    }  
    return count;  
}  
x        =   1011 (11)  
x-1      =   1010 (10)  
x & (x-1)=   1010 (10)   -> 1 set bit gone, count = 1  

x        =   1010 (10)    
x-1      =   1001 (9)  
x & (x-1)=   1000 (8)    -> one more set bit gone, count = 2  

x        =   1000 (8)  
x-1      =   0111 (7)  
x & (x-1)=   0000 (0)    -> one more set bit gone, count = 3  

Loop runs exactly k times where k = popcount.  
For a number like 0x00000001, this is 1 iteration vs 32 naive.  

### Level 3 - SWAR Parallel Bit Count 0(1) - The Impressive Answer  
No loop at all. Uses bit parallel arithmetic to count all 32 bits simultaneously.  

int countBits_swar(uint32_t x)  
{
    x = x - ((x >> 1) & 0x55555555);  
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);  
    x = (x + (x >> 4)) & 0x0F0F0F0F0F;  
    return (x * 0x01010101) >> 24;  
}  
How it works - step by step with 0b11001010:  

Step 1: count bits in pairs  
0x55555555 = 0101 0101 0101 0101...  
Counts each pair independently -> 2-bit counts  

Step 2: sum pairs into nibbles  
0x33333333 = 0011 0011 0011 0011...   
Each nibble now holds count of its 4 bits  

Step 3: sum nibbles into bytes  
0x0F0F0F0F = 00001111 00001111...  
Each byte now holds count of it's 8 bits  

Step 4: sum all bytes via multiply trick  
Multiply by 0x01010101 propagates all byte sums into the top byte, then >> 24 extracts it  
Example: -  
x = 0b11010110  
1] x = x - ((x >> 1) & 0x55);  
0x55   =   01010101  
x >> 1 =   01101011  
(x >> 1) & 0x55 =   
           01101011
           01010101
           --------  
           01000001  
           --------  
x - ((x >> 1) & 0x55) =   
           11010110  
           01000001  
           --------  
           10010101  
           --------  
we know,  
Pair------# of 1's--------Binary result  
11---------->2---------------->10  

01---------->1---------------->01  

10---------->1---------------->01  
So, each 2 - bit group stores the count of one's in that group.  
So, now x = 10010101  
This means each pair of bits now stores count of 1's  
Pairs now:  
10-01-01-01  
Counts-> 2,1,1,1  
Now,  

x = (x & 0x33) + ((x >> 2) & 0x33);  
for 8-bit:->0x33 = 00110011  
Goal of this line, combine 2-bit counts into 4-bit counts(nibbles)  
x & 0x33 -> 00010001  
(x >> 2) & 0x33 -> 00100001  
(x & 0x33) + ((x >> 2) & ox33) = 00110010  
Group into nibbles (4 bits):  
x = 0011 0010  
Nibble-------->Binary--------->Decimal---------->Meaning  

left------------0011-------------3-------------bits in first 4 bits  

right-----------0010-------------2-------------bits in last 4 bits  
Original number = 1101 0110 (3,2)  
Now, 

x = (x + (x >>4)) & 0x0F;  
0x0F = 00001111(mask)  
Goal, combine nibble counts -> byte count  
x      = 0011 0010   
x >> 4 = 0000 0011  
x + (x >> 4) = 0011 0101  
(x + (x >> 4)) & 0x0F = 0000 0101  
So, x = 0000 0101  
This means the byte has 5 ones  
Now,  
return (x *0x01) >> 0;  
In 8-bit version multiplication is trivial:  
0000 0101 * 0000 0001 = 0000 0101  
Result = 5  

This is what __builtin_popcount(x) compiles to on most architectures. On ARM Cortex-M4 and above, it maps to a single VCNT instruction.  

In production always use:  

int count = __builtin_popcount(x);  // GCC/Clang  

## Parity - Even or Odd Number of set Bits?  

Parity = 0 if even number of 1s, if odd number of 1s.  
Naive apprach:  
int parity = 0;  
while(x)  
{  
    parity ^= (x & 1);  
    x >>= 1;  
}  
XOR accumalates--> even number of 1s cancels to 0, odd leaves 1.  

## Parity in real Embedded Systems  

### UART Parity Bit  

Every UART frame optionally includes a parity bit for error detection.  

FRAME:- [START][D0][D1][D2][D3][D4][D5][D6][D7][PARITY][STOP]  

Even parity: parity bit set so total 1s is even  
Odd parity: parity bit set so total 1s is odd  

Data = 0b1001 0001 -> three 1s (odd count)  
Even parity bit = 1 (makes total = 4, even)  
Odd parity bit = 0 (keeps total =3, odd)  

Hardware UART does this automatically. But knowing the math helps you:  

- Debug parity mismatch errors  
- Implement software UART (bit-banging)  
- Understand why parity only catches single bit errors (two flipped bits = same parity = undetected!)  

### CAN Bus CRC  

CAN uses a 15-bit CRC, a much stronger form of parity across the entire frame. Same XOR principle, extend to a polynomial. Catches burst errors up to 15 bits long.  

ECC Memory (Error Correcting Code)  
Used in automotive, aerospace, server grade embedded systems.  
Single Error Correct, Double Error Detect (SECDED):    
- Extra parity bits cover specific bit groups.   
- Hamming code positions: 1, 2, 4, 8, 16...  
- Can pinpoint AND correct a single flipped bit  
- Detects (bit can't correct) two flipped bits  

The entire ECC scheme is built on XOR parity across different subsets of bits.  

## Hamming Distance  
The number of bit positions where two values differ.  

int hammingDistance(uint32_t a, uint32_t b)  
{
    return __builtin_popcount(a ^ b);  
}  
XOR gives 1 wherever bits differ, popcount counts those positions.  

a = 0x1D = 0001 1101  
b = 0x0F = 0000 1111  
a ^ b    = 0001 0010 -> 2 bits differ  
Hamming distance = 2  

Why it matters in embedded:  
- Minimum hamming distance of a code = its error detection strength  
- CAN bus, RS-232, I2C all rely on this concept  
- Used in comparing configuration register states during self-test.  

## Q&A - Test yourself  

Q1] CLASSIC: What is the time complexity of Brian kernighan's algorithm and when is it better than naive?  
Solution:-  

O(k) where k = number of set bits. Better than 0(32)  
Naive when the number is sparse - has few set bits.  
For 0x0000 0001,  
kernighan = 1 iteration  
naive = 32.  
For 0xFFFF FFFF, both = 32 iterations. In practice, interrupt pending registers are often sparse - very applicable.  

Q2] INTERVIEW: What is the parity of 0xABCD?  
Solution:-  

0xABCD = 1010 1011 1100 1101  
Set bits: count them -> 10 set bits  
10 is even -> parity = 0  
Quick way:  XOR all nibles:  
A^B^C^D = 1010^1011^1100^1101  
        = 0001 ^ 1100 ^ 1101  
        = 1101 ^ 1101  
        = 0000 -> parity = 0.  

Q3] EMBEDDED: A UART receives 0x41 ('A') with even parity. The received parity bit is 0. Is the frame valid?  
Solution: -  

0x41 = 0100 0001 -> two set bits -> even count.  
Even parity bit should be 0 (total already even, no adjustment needed).
Received parity = 0, So Frame is valid.  
If parity bit was 1, it would indicate a transmission error.  

4] TRICKY: Can parity detect all errors? What is it's weakness?  
Solution: -  

No, Parity only detects odd numbers of bit flips. If 2 bits flip, parity is unchanged, error goes undected. Also can't locate which bit flipped, only that an error occured. For stronger protection: CRC(detects burst errors), ECC/Hamming (detects and corrects single-bit errors)  

5] QUALCOMM STYLE: Write the most efficient way to check if a 32-bit value has exactly 2 bits set.  
Answer:-  

bool exactlyTwoBits(uint32_t x)  
{
    return x && !(x & (x-1)) == false && __builtin_popcount(x) == 2;  
}  
// x -> checks not zero  
// !(x & (x-1))  == false -> checks if it has more than one bit  


// Cleaner:  
bool exactlyTwoBits(uint32_t x)  
{
    uint32_t y = x & (x-1);  // clear lowest set bit  
    return y && !(y & (y - 1));  // remaining y is power of 2  
}  
Second version: after clearing one set bit, check if remainder is exactly a power of 2 (one bit left). No popcount needed, pure bit tricks.  

6] CONCEPT: Why are hamming code check bit positions always powers of 2?  
Answer: -  

Each power of 2 position (1,2,4,8..)  covers a unique subset of data bits based on binary representation of their positions. Bit 1 covers all positions with bit 0 set, bit 2 covers all with bit 1 set, etc. This orthogonal coverage ensures any single error maps to a unique syndrome, the XOR of failed parity checks directly gives the error position in binary.  

