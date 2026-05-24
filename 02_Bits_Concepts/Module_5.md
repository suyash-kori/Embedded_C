# Bit Fields & Strcuts in C  

### Why this matters for interviews  

This module separates hobbyist embedded developers from professional firmware engineers. Misuse of bit fileds and structs causes some of the nastiest, hardest to reproduce bugs in production silicon. Knowing the pitfalls signls you write production quality code.  

## Struct Padding - The Silent Memory Waster  

The compiler inserts invisible padding bytes to keep members naturally aligned (an int must sit at a 4-byte boundary, etc.).  

struct Bad  
{
    char a; // 1 byte  
    int b;  // 4 bytes, but needs 4-byte alignment!  
    char c; // 1 byte  
};  

What actually sits in memory:  
[a][pad][pad][pad][b b b b][c][pad][pad][pad]  
-1-------3------------4-----1------3----------= 12 bytes  

sizeof(struct Bad) = 12  // NOT 6!  

Reordering eliminates padding:  

struct Good  
{
    int  b;  // 4 bytes  
    char a;  // 1 byte  
    char c;  // 1 byte  
             // 2 bytes pad at end for alignment  
};  
sizeof(struct Good) = 8 // better  

Rule: Order members largest to smallest to minimizes padding.  

## --attribute__((packed))  -- Use with Caution  

Forces the compiler to remove all padding.  

struct __attribute__((packed)) PackedReg  
{
    char a; // 1 byte  
    int b;  // 4 bytes - starts at offset 1(unaligned!)  
    char c; // 1 byte  
};  
sizeof(PackedReg) = 6 // no padding  

The danger - unaligned access:  

- On x86: works, but slower (hardware fixes it)  
- On ARM Cortex - M0/M0+: hard fault - no unalligned access support  
- On ARM Cortex - M3/M4: configurable, can be enabled but still slower.  
- On MIPS, some DSPs: immediate fault  

// This can crash on ARM Cortex - M0:  
PackedReg *p = (PackedReg*)some_buffer;  
int val = p->b;  // unaligned 4-byte read-> HardFault!  

Safe rule: Only use packed when you control the base pointer alignment AND you've verified your target CPU supports unaligned access.  

## Bit Fields - Readable Register Mapping  

typedef struct  
{
    uint32_t clk_en    : 1;  // bit 0  
    uint32_t prescaler : 1;  // bit 1  
    uint32_t src       : 2;  // bits 3:2  
    uint32_t divider   : 4;  // bits 7:4  
    uint32_t reserved  : 24; // bits 31:8  
} ClockReg_t;  

Much more readable than raw mask/shift macros:  

ClockReg_t *CLK = (ClockReg_t*)0x40001000;  
CLK->clk_en = 1;  // vs: REG |= (1 << 0)  
CLK->divider = 6; // vs: REG = (REG & ~0xF0) | (6 << 4)  

## The Critical Bit Field Warnings  

Warning 1 - Bit order is implementation defined  

The C standard does not specify whether bit fields are allocated LSB-first or MSB-first. GCC on ARM allocates LSB-first by default, but this can differ between:  

 - Compilers (GCC vs Keil vs IAR)  
 - Compiler versions  
 - Target architectures  

Consequence: A struct mapped to a hardware register may work perfectly with one compiler and silently break with another. This is why MISRA-C Rule 6.1 forbids bit fields for hardware register mapping in safety critical code.  

Warning 2 - No "&" operator on bit fields  

uint32_t *ptr = &CLK->divider; // ERROR - can't take address of bit field  

Bit fields don't have byte level addresses. You can't pass them to functions expecting a pointer.  

Warning 3 - Bit fields can't span storage units  

typedef struct  
{
    uint8_t a:6;  
    uint8_t b:4;  // may NOT span into next byte - behavior varies  
} Tricky;  

Compiler may insert padding between a and b to avoid crossing the byte boundary.  

Warning 4 - Signed bit fields are tricky  

typedef struct  
{
    int val:3;  // range: -4 to 3 (3-bit signed)  
} S;  
S s;  
s.val = 4;   // silently wraps to -4!  

Always use "unsigned" bit fields unless you explicitly need sign extension.  

## The Union Trick - Best of Both Worlds  

The professional solution: use a union to access the register both as raw bits AND as named fields.  

typedef union  
{
    uint32_t raw;         // full 32-bit access  
    struct  
    {
        uint32_t clk_en   : 1;  
        uint32_t prescaler: 1;  
        uint32_t src      : 2;  
        uint32_t divider  : 4;  
        uint32_t reserved : 24;  
    } bits;  
} ClockReg_t;  

volatile ClockReg_t *CLK = (ClockReg_t*) 0x40001000;  

// Named field access - readable:  
CLK->bits.divider = 6;  

// Raw access - atomic, safe for full register writes:  
CLK->raw = 0x00000061;  

// Read current, modify one filed, write back:  
ClockReg_t temp;  
temp.raw = CLK->raw;    // read  
temp.bits.divider = 8;  // modify  
CLK->raw = temp.raw;    // write  

This pattern is used extensively in STM32 HAL, Nordic SDK, and most professional firmware SDKs.  

## "volatile" The Most Critical Keyword in Embedded C  

volatile tells the compiler: do not optimize accesses to this variable - always go to actual memory.  

// WITHOUT volatile - compiler may optimize this loop away:  
uint32_t *STATUS = (uint32_t*)0x40000000;  
while(*STATUS & 0x01);// compiler:"STATUS never changes,infinite loop!"  

// With volatile - correct:  
volatile uint32_t *STATUS = (volatile uint32_t*)0x40000000;  
while(*STATUS & 0x01); // compiler: re-read every iteration  

Three situations where volatile is mandatory:  

// 1. Hardware registers  
volatile uint32_t *UART_SR = (volatile uint32_t*)0x40011000;  

// 2. Variables shared between ISR and main code  
volatile bool data_ready = false;  // ISR sets it, main reads it  

// 3. Variables modified by DMA  
volatile uint8_t dma_buffer[256];  // DMA writes, CPU reads  

What volatile does NOT do:  

- Does not make operations atomic (RMW still not thread safe)  
- Does not add memory barriers (need __DMB() on ARM for that)  
- Does not prevent reordering across volatile accesses in all compilers  

## "const volatile" - Read only Hardware Registers  

Some registers are read only from the CPU's perspective (status registers, ID registers).  

// Read-only hardware register  
const volatile uint32_t *CHIP_ID = (const volatile uint32_t*)0xE0042000;  

uint32_t id = *CHIP_ID;  // read OK  
*CHIP_ID = 0x1234; // compiler ERROR - const prevents accidental write  

This is the correct type for status/ID registers. The "const" catches accidental writes at compile time, "volatile" ensures the read always goes to hardware.  

## Struct Size - The "sizeof" Interview Questions  

Always mentally calculates struct sizes in interviews:  

struct A  
{
    char a;  // 1 byte + 3 pad  
    int  b;  // 4 bytes  
    short c; // 2 bytes + pad  
};  
// sizeof = 12  

struct B  
{
    int b;   // 4 bytes  
    short c; // 2 bytes  
    char a;  // 1 byte + 1 pad  
};  
// sizeof = 8  

struct C  
{
    int b;  // 4 bytes  
    short c;// 2 bytes  
    char a; // 1 byte  
    char d; // 1 byte - no pad needed!  
};  
// sizeof = 8  

Trailing padding rule: Struct size is always a multiple of it's largest member's alignmemt. "struct B" ends with short(2) + char(1) = 3 bytes used of a 4-byte slot -> 1 byte of trailing padding added.  

## Q&A - Test Yourself  

Q1] INTERVIEW: What is "sizeof" this struct and why?  
struct Foo  
{
    char a;  
    char b;  
    int c;  
    char d;  
};  

Answer:  

12 bytes  
[a][b][pad][pad][c c c c][d][pad][pad][pad]  
-1--1-----2---------4-----1-----3-----------= 12  

After d, 3 bytes of trailing padding added so struct size is multiple of 4 (largest member alignment).  
Reorder to int c; char a; char b; char d; -> sizeof = 8.  

Q2] TRICKY: Why does this code potentially fail on an ARM Cortex-M0?  

struct __attribute__((packed)) Packet  
{
    uint8_t cmd;  
    uint32_t payload;  
} pkt;  
uint32_t val = pkt.payload;  
Answer:-  

"payload" starts at offset 1 (after cmd), unaligned 4-byte access.  
Cortex-M0 has no unaligned access support -> HardFault.  
FIX:-  
use memcpy(&val, &pkt.payload, 4) - complier generates byte by byte copy which is always safe. Or restructure to avoid packing.  

Q3] EMBEDDED: A variable data_ready is set in an ISR and polled in main. What's wrong?  
Answer:-  

bool data_ready = false;  

void UART_IRQHandler(void)  
{
    data_ready = true;  
}

int main(void)  
{
    while(!data_ready);  // wait  
    process();  
}  
Answer:-  

"data_ready" is not volatile. The compiler sees data_ready never changes in main() and optimizes the while loop to while(true) - the read is cached in a register, ISR update never seen.  
FIX:-  

volatile bool data_ready = false;  

Q4] INTERVIEW: Can you take the address of a bit field member?  

typedef struct  
{
    uint32_t flag : 1;  
} Reg;  
Reg r;  
uint32_t *p = &r.flag;  // ?  

Answer:-  

Compile error. Bit fields don't have addressable memory locations, they share a byte/word with neighboring fields. The address of operator "&" is not allowed on bit fields. This is one reason MISRA-C restricts bit filed usage in interfaces.  

Q5] QUALCOMM STYLE: A register union is defined correctly but reads return wrong values. Volatile is missing. Where exactly should it be placed?  
typedef union  
{
    uint32_t raw;  
    struct  
    {
        uint32_t en : 1;  
        uint32_t mode:3;  
    } bits;  
} Reg_t;  
Reg_t *REG = (Reg_t*)0x40005000;  

Answer:-  

volatile Reg_t *REG = (volatile Reg_t*) 0x40005000;  

"volatile" must be on the pointer declaration, not the typedef. The typedef describes the type "volatile" describes the access semantics of this specific pointer. Without it, the complier catches the first read and never re-reads from hardware.  

Q6] CONCEPT: What does "const volatile" mean and when would you use it?  

Answer:- 
Read only from software but can change externally.  
Use for: hardware status registers the CPU should never write (chip ID, silicon revision, read-only status flags). "const" causes a compile error if code tries to write it - catches bugs early. "volatile" ensures every read goes to actual hardware. Together they perfectly model a read only hardware register.  

