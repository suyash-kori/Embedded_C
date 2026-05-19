# Bit Manipulation Mastery
### Bit Manipulation Techniques  

### Why This Matters for interviews
This is the most directly tested module in embedded interviews. Every register configuration, every GPIO toggle, every flag check in real firmware uses exaactly these patterns. Expect 2-3 direct questions from this area in any Qualcomm/Cadence etc interviews.

## The Four Fundamentals Operations

Every bit manipulation task reduces to one of these four. Memorize them like multiplication tables.  

// n = bit position (0-indexed from LSB)  

// SET bit n -> force it to 1  
x |= (1u << n);  

// CLEAR bit n -> force it to 0  
x &= ~(1U << n);  

// TOGGLE bit n -> flip it  
x ^= (1u << n);  

// CHECK bit n -> is it 1?  
(x >> n) & 1          // returns 0 or 1  
(x & (1u << n)) != 0  // returns false or true  

### Why 1u and not 1?  

Because 1 is a signed int. 1 << 31 is undefined behaviour.  
1u << 31 is perfectly valid unsigned shift. Always use 1u or cast explicitly when building masks.  

## Working with multiple Bits (Bit Fields in Register)  

Real registers rarely deal with single bits, you'll configure multi-bit fields.  

EXAMPLE: CLOCK DIVIDER REGISTER
Bits [7:4] = divider value
Bits [3:2] = clock source
Bit [1] = prescaler enable
Bit [0] = clock enable

Step 1 - Build the mask:  

#define DIV_SHIFT 4  
#define DIV_MASK  (0xF << DIV_SHIFT)  // 0b11110000  

Step 2 - Read-Modify-Write(RMW):  

uint32_t reg = CLK_REG;                   // READ  
reg &= ~DIV_MASK;                         // clear bits 7:4  
reg |= ((value << DIV_SHIFT) & DIV_MASK); // set new value  
CLK_REG = reg;                            // WRITE  

This is the RMW pattern, the single most important pattern in firmware.  
Never write a whole register when you only mean to change a few bits, you'll corrupt other fields.

## Isolate the lowest Set Bit  

y = x & (-x);  

x =  0b0010 1100  (44)  
-x = 0b1101 0100  (two's complement)  

x & (-x) = 0b00000100 -> isolates bit 2  
Use cases:  

- Find which bit triggered an interrupt (in a pending register)  
- Process set bits one by one in a loop  
- Check if exactly one bit is set

## Clear the lowest Set Bit  

y = x & (x - 1);  

x      =     0b00101100  
x-1    =     0b00101011  
x&(x-1)=     0b00101000 -> lowest set bit gone  

This is the foundation of Brian Kernighan's popcount (Discussed in Module 4).  

BONUS: If x & (x-1) == 0 and x != 0, then x is a power of 2, it had exactly one set bit which just got cleared.  

## Power of 2 Check  

bool isPowerOf2(uint32_t x)  
{  
    return x && !(x & (x-1));  
}  

The "x &&" part handles the edge case i.e.  
0 would pass !(0 & -1) incorrectly without it.  

x = 8 -> 1000 & 0111 = 0000 -> true  
x = 6 -> 0110 & 0101 = 0100 -> false  
x = 0 -> guarded by x && ...-> false  

## Extract a Bit Range  

To extract bits [high:low] from a register:  
#define EXTRACT(reg, high, low) (((reg) >> (low)) & ((1u << ((high)-(low)+1)) -1))  
Example:-   
reg = 0x10110110  
high = 5  
low = 3  
uint32_t val = EXTRACT(reg, 5, 3); // Shifts right by 3, masks with 0b111(7)  
i.e. we want to extract 110 from reg value[5-3]
Let's decode 1 by one

((reg) >> (low)) = 00010110  
((1u) << ((high)-(low)+1)) = (1u << 3) = 00001000  (8)  
8 - 1 = 7 (00000111)  

So, AND both value we will get, 0x00000110(expected)  
This is how you decode sensor data, protocol frames, and packed register values.  

## Set a specific Bit Range to a Value

// Set bits [high:low] of reg to val  
void setBitField(volatile uint32_t *reg, uint8_t high, uint8_t low, uint32_t val)  
{
&emsp;uint32_t mask = ((1u << (high - low + 1)) -1) << low;    
&emsp;*reg = (*reg & ~mask) | ((val << low) & mask);  
}

The "&mask" on the val side prevents val from accidentally setting bits outside the target field if val is too large.    

Example: -  
Set bits [5:3] of reg(0x1011 0110) to val = 000  
reg = 0x1011 0110

mask = ((1u << (high - low + 1)) - 1) << low;  
((1u << (3)) - 1)  
(00001000(8) - 1) = 7(0000 0111)  
low = 3  
mask = 0000 0111(7) << 3  
mask = 0011 1000  
~mask = 1100 0111
*reg = (*reg & ~mask) | ((val << low) & mask);  
(*reg & ~mask) =  
     1011 0110  
     1100 0111  
    -----------  
     1000 0110  
    -----------  

((val << low) & mask)  
((0   << 3  ) & 0011 1000)    
((0) & 0011 1000) = 0  
So final o/p = 1000 0110  

## Bit Manipulation in real embedded context  
GPIO config on STM32 (MODER register - 2 bits per pin):  
#define PIN          5  
#define MODE_OUTPUT  0x1  

GPIOA->MODER &= ~(0x3 << (PIN * 2));       // clear 2 bits  
GPIOA->MODER |= (MODE_OUTPUT << (PIN * 2)); // set output mode  

## Q&A- TEST YOURSELF  

Q1] CLASSIC: How do you toggle bits 3 and 5 simultaneously in one operation?  

Answer: -  

x ^= (1u << 3) | (1u << 5);  

XOR with a mask of multiple bits toggles all of them atomatically in one instruction.  

Q2] EMBEDDED: A 32-bit config register at 0x40002000 has bits [11:8] controlling clock divider. Set the divider to 6 without touching any other bits.  

Answer: -  

#define CFG_REG (*((volatile uint32_t*)0x40002000))
#define DIV_LOW  8  
#define DIV_HIGH 11   
#define DIV_MASK (0xF << DIV_LOW)  

CFG_REG = (CFG_REG & ~DIV_MASK) | ((6 << DIV_LOW) & DIV_MASK);  
Always mask the incoming value too, if someone passes 0x1F, you don't want bits outside [11:8] getting set.  

Q3] TRICKY: What does this code do?  
x &= x - 1;  
And what is it's output for x =0?  

Answer: -  
Clears the lowest set bit of x. For x = 0: 0 & (0-1) = 0 & 0xFFFFFFFF = 0.  
No crash, but semantically meaningless - 0 has no set bits. Always guard with while(1) before applying this trick in a loop.  

Q4] INTERVIEW: Without a multiply instruction, compute x*6.  

Answer: -  
int result = (x << 2) + (x << 1);  
// x*4 + x*2 = x*6  
Idea  
Break 6 into powers of 2.  i.e 6 = 4 + 2  
And powers of 2 can be made using left shift:  
For x * 2 -------> x << 1  
For x * 4 -------> x << 2  
So, x*6 = x*4 + x*2  
        = (x << 4)+(x << 2)  
If x = 1011(11)  
x << 1 (x*2)  =  10110  
x << 2 (x*4)  =  101100  

        101100  (44)  
        010110  (22)  
        ------  
       1000010 -> 64+2 = 66  

Q5] QUALCOMM STYLE: Given uint32_t reg = 0xABCD1234, extract bits[19:12]  

Answer:-  
uint32_t val = (reg >> 12) & 0xFF;  
// 0xABCD1234 >> 12 = 0x000ABCD1
// & 0xFF = 0xD1 = 209  
Bit range of 8 bits -> mask is (1 << 8) - 1 = 0xFF.  
Always-> shift first, then mask.  

Q6 - TRICKY: Is this safe in a multi-threaded RTOS environment?  
GPIOA->ODR |= (1 << 5)

Answer:-  
No.The "|=" is a RMW, it complies to load, OR, store(3 instructions). A context switch between load and store corrupts the results.  
Safe alternatives: use BSSR register (hardware atomic), or disable interrupts around the RMW, or use a hardware semaphore. This is a very common bug in RTOS based firmware.




