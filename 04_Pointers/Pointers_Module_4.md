# const, volatile, restrict & Type Qualifiers  

## Why these qualifiers matter more in embedded than anywhere else  

In application programming, "const" is a mild nicety and "volatile" is almost never used. In embedded C, these qualifiers are load-bearing, they directly affect hardware correctness, compiler optimization behaviour, and whether your firmware works at all.  

A missing "volatile" on a hardware register can cause your firmware to appear to work in debug builds and completely fails in release builds. That's not a hypothetical, it happens reguraly, and it's one of the hardest bugs to track down because the source ofthe problem looks completely unrelated to the symptom.  

### const, what it really means  

"const" means "I promise this value will not be changed through this name." It's compile time constraint, not a runtime one. The compiler enforces it by flagging any attempt to write through a const qualified name as an error.  

const int x = 10;  
x = 20;  // compile error, cannot assign to const variable  

In embedded, "const" global variables are placed in Flash(read only memory) by the linker, not in SRAM. This is critical, SRAM is scarce on microcontroller (often 20-256 KB), Flash is larger (often 256 KB-2 MB). Lookup tables, configuration tables, string constants, all should be "const" so they live in Flash and don't consume precious SRAM.  

/* Without const, linker puts this in SRAM */  
uint8_t crc_table[256] = { 0x00, 0x07, 0x0E,...};  

/* With const, linker puts this in Flash */  
const uint8_t crc_table[256] = {0x00, 0x07, 0x0E,..};  

On a STM32 with 64KB SRAM and 512 KB Flash, putting a 256-byte lookup table in SRAM vs Flash is the difference between wasting 0.4% of your SRAM or none at all. Multiply that across a whole codebase.  

## The four "const" + pointer combinations  

This is one of the most testd topics is C interviews. There are exactly four combinations and you need to know all of them cold.  

The rule for reading them: read right to left, pausing at the "*".  

int *p;  // p is a pointer to int, nothing is const  

const int *p;  // p is a pointer to const int, data is read only  

int * const p;  // p is a const pointer to int, pointer is fixed  

const int * const p;  // p is a const pointer to constant int, both fixed  

Let's go through each one carefully.  

#### Combination 1: (int *p), nothing is const  

int x = 10, y = 20;  
int *p = &x;  

*p = 99;  // OK, can change the data  
p = &y;  // OK, can move the pointer  

Fully mutable. Use for output parameters where the function needs to both write through the pointer and potentially advance it.  

#### Combination 2: (const int *p), data is read-only, pointer is movable  

int x = 10, y = 20;  
const int *p = &x;  

*p = 99;  // COMPLETE ERROR, cannot write through const pointer  
p = &y;  // OK, can move the pointer itself  

Read it as: "p points to a const int." The "int" it points to cannot be modified through "p". But "p" itself can be reassigned to point somewhere else.  

This is the most common form in embedded. Use it for:  

- Function parameters where you read a buffer but don't modify it.  
- Pointers into Flash memory (read-only by nature)  
- Any "input" parameter.  

/* src is read-only, const int* communicates intent and enforces it */  

void uart_send(const uint8_t *src, size_t len)  
{  
&emsp;for (size_t i = 0; i < len; i++)  
&emsp;{  
&emsp;&emsp;while(!(UART->SR & TX_READY));  
&emsp;&emsp;UART->DR = src[i];  // reading src, fine  
&emsp;&emsp;// src[i] = 0;  // would be a compile error  

&emsp;}

}  

#### Combination 3: (int * const p), pointer is fixed, data is writable  

int x = 10, y = 20;  
int * const p = &x;  // must initialize at declaration, can't reassign later  

*p = 99;  // OK, can change the data  
p = &y;  // COMPILE ERROR, cannot change where p points  

Read it as: "p is a const pointer to int". The pointer itself is locked, it will always point to the same address. But the data at that address can change.  

Use case in embedded: a pointer to a specific hardware register or a fixed buffer, you want the address to be immutable but the value to change.  

/* This GPIO register is always at 0x40020014, pointer never changes */  
uint32_t * const GPIOA_ODR = (uint32_t*) 0x40020014;  

*GPIOA_ODR |= (1 << 5);  // OK, toggle pin 5  
GPIOA_ODR = 0x40020018;  // COMPILE ERROR, cannot move the pointer  

#### Combination 4: (const int * const p), everything is fixed  

int x = 10, y = 20;  
const int * const p = &x;  

*p = 99;   // COMPILE ERROR, cannot change data  
p = &y;  // COMPILE ERROR, cannot change pointer  

Read it as: "p is a const pointer to const int". Neither the pointer nor the data can change.  

Use case: a pointer to a read-only hardware register at a fixed address, like a chip ID register or a status register you only ever read.  

/* Chip ID register, fixed address, read-only value */  

const uint32_t * const CHIP_ID = (const uint32_t*) 0x1FFFF7E8;  

uint32_t id = *CHIP_ID;  // OK, reading is fine  
*CHIP_ID = 0;  // COMPILE ERROR  
CHIP_ID = NULL; // COMPILE ERROR  

Quick reference, the four combinations  

Declaration--------Chnage (*p)(data)?---------Change(p)(pointer)?  

int *p-------------------YES------------------------YES----------  
const int *p-------------No-------------------------YES----------  
int * const p------------YES------------------------No----------  
const int * const p------No-------------------------YES----------  

## volatile, the most important qualifier in in embedded C  

"volatile" tells the compiler: do not make any assumptions about this variable. Every read must actually read from memory. Every write must actually write to memory. Do not cache, reorder, or eliminate any access.  

Without "volatile", the compiler is free to optimize reads and writes. It can cache a variable in a CPU register and never re-read it from memory. It can eliminate reads it thinks are redundant. It can reorder accesses. For normal variables, this is fine, even correct. For hardware registers and ISR-shared variables, it is catastrophic.  

### Why "volatile" is necessary, the optimization problem  

Consider this loop waiting for a hardware flag:  

uint32_t status;  
while(status != 1)  // wait for flag  
{  

&emsp;/* do nothing */  

}  

At optimization level O2, the compiler looks at this and reasons: "status is never written in this loop. I read it once before the loop, it was 0. It will always be 0. This loop is infinite. I'll optimize it to while(1){}."  

The compiler is not wrong, by the rules of the C abstract machine, if nothing in this thread writes "status", it can't change. The compiler doesn't know that a hardware peripheral or an ISR is writing to that memory location outside the compiler's view.  

volatile uint32_t status;  
while (status != 1)  
{  

&emsp;/* do nothing */  compiler must re-read status every iteration

}  

Now the compiler knows it cannot assume the value of "status" between iterations. Every loop iteration generates an actual load instruction from memory.  

### The three cases where you must use "volatile" in embedded  

#### Case 1:- Memory-mapped hardware registers  

Every peripheral register, UART data register, GPIO output register, timer counter, ADC result, must be accessed through a volatile pointer. The hardware can change these values at any time, and you can trigger hardware behaviour by writing to them.  

/* Correct, volatile pointer to hardware register */  
#define GPIOA_IDR (*(volatile uint32_t*) 0x40020010)  
#define UARY1_DR (*(volatile uint32_t*) 0x40011004)  
#define TIM2_CNT (*(volatile uint32_t*) 0x40000024)  

/*  Read GPIO pin state, must re-read every time */  
uint32_t pins = GPIOA_IDR;  

/* Read UART received data */  
uint8_t byte = UART1_DR;  

/* Writing to a register to trigger hardwar */  
GPIOA_ODR |= (1 << 5);  // set pin, hardware responds immediately  

Without "volatile" on these, the compiler may read the register once, cache the value, and use the catched value for all subsequent reads. Your firmware reads the same stale value forever.  

#### Case 2:- Varibles shared between an ISR and main code  

An ISR (Interrupt Service Routine) runs asynchronously, it can fire between any two instructions of your main code. If main code and an ISR share a variable, that variable must be "volatile".  

volatile uint8_t uart_byte_received = 0;  
volatile uint8_t uart_rx_data = 0;  

/* ISR, runs when UART receives a byte */  

void UART1_IRQHandler(void)  
{  
&emsp;uart_rx_data = UART1->DR;  // read received byte  
&emsp;uart_byte_received = 1;  // set flag  
}  

/* Main loop, polls the flag */  
int main(void)  
{  
&emsp;while(1)  
&emsp;{
&emsp;&emsp;if (uart_byte_received)  // must see ISR's write  
&emsp;&emsp;{  

&emsp;&emsp;&emsp;process_byte(uart_rx_data);  // must see ISR's write  
&emsp;&emsp;&emsp;uart_byte_received = 0;  

&emsp;&emsp;}  
&emsp;}  
}  

Without "volatile", the compiler may read "uart_byte_received" once, cache "0" in a register, and your "if" never fires, even though the ISR has set it to "1" in memory.  

#### Case 3: Variables modified by multiple threads (RTOS)  

In an RTOS environment(FreeRTOS, Zephyr), variables shared between tasks without a mutex should be "volatile", though in practice, proper synchronization primitives are the right solution. "volatile" alone does not make operations atomic.  

/* Shared between Task1 and Task2 */  
volatile uint32_t shared_counter = 0;  

/* Task 1 */  
shared_counter++;  /* volatile ensures actual read-modify-write, but this is still NOT atomic on most architectures */  

"volatile" does NOT mean atomic  

This is a criticalpoint that trips up even experienced developers.  

volatile uint32_t counter = 0;  
counter++;  

This looks like one operation but compiles to three:  

1. Load "counter" from memory into a register  
2. Increment the register  
3. Store the register back to memory  

An interrupt can fire between steps 1 and 3. If the ISR also modifies "counter", you have a race condition. "volatile" ensures the load and store actually happen, but does not prevent interruption between them.  

For atomic operations in embedded, you need to either disable interrupts around the access, use atomic built-ins(__atomic_add_fetch), or use RTOS primitives (mutex, semaphore).  

/* Safe increment, disable IRQ around the operatiom */  
__disable_irq();  
counter++;  
__enable_irq();  

### "volatile const", read only hardware registers  

Combining both qualifiers gives you a register that the hardware can change (so you must re-read it, volatile)  but your code must never write to (const).  

/* Status register, hardware writes it, your code only reads it */  
#define UART1_SR (*(volatile const uint32_t*)0x40011000)  

uint32_t status = UART1_SR;  // must re-read eevry time (volatile)  
UART1_SR = 0;  // COMPILE ERROR, const prevents writing  

This is used for:  

- Status registers (read only by software defination)  
- Chip ID/revison registers
- ADC result registers on some peripherals  

### "volatile and compiler optimization, a concrete demonstration  

uint32_t *reg = (uint32_t*)0x40020014;  

/* Without volatile, compiler may generate: */  
*reg = 1;  // write 1  
*reg = 2;  // write 2  
*reg = 3;  // write 3  
/* Compiler: "only the last write matters, optimize to: *reg = 3" */  
/* Hardware never sees the 1 or 2 transitions, WRONG for GPIO toggling */  

volatile uint32_t *vreg = (volatile uint32_t*) 0x40020014;  

/* With volatile, complier generates ALL three writes */  
*vreg = 1;  // write 1, hardware sees this  
*vreg = 2;  // write 2, hardware sees this  
*vreg = 3;  // write 3, hardware sees this  

This matters for generating waveforms, bit banging protocols, and any sequence where intermediate values have hardware significance.  

## "restrict" the optimization hint  

"restrict" is a C99 qualifier applied to pointers. It tells the compiler: this pointer is the only way to access the data it points to within this scope. No other pointer aliases it.  

This is a promise from the programmer to the complier, enabling optimizations that would otherwise be unsafe.

/* Without restrict. compiler can't assume src and dst don't overlap */  

void copy(int *dst, int *src, size_t len)  
{  
&emsp;for (size_t i = 0; i < len; i++)  
&emsp;{  
&emsp;&emsp;dst[i] = src[i];  // compiler must re-read src[i] every iteration  because writing dst[i] might change src[i] if they overlap  

&emsp;}  
}  

/* With restrict, compiler knows they don't overlap */  
void copy(int * restrict dst, const int * restrict src, size_t len)  
{  
&emsp;for(size_t i = 0; i < len; i++)  
&emsp;{  
&emsp;&emsp;dst[i] = src[i];  // compiler can use SIMD, vectorize, cache src values  

&emsp;}  

}  

This is why the stanadard library "memcpy" uses restrict:  

void *memcpy(void * restrict dst, const void * restrict src, size_t n);  

"memcpy" assumes no overlap (use "memmove" if they might overlap). The "restrict" lets the compiler generate the fastest possible copy, often using vector/SIMD instruction.  


##### CORE IDEA  

Imagine you're a chef (the compiler) following a recipe.  
The recipe says:  
" Add salt from Bowl A, then taste Bowl A again"  
You have to walk back and check Bowl A every single time, because for all you know, someone swapped the bowls while you weren't looking.  

But if the recipe says:  
"I promise Bowl A and Bowl B are completely separate, nobody will touch them."  
Now you can be smarter.You check Bowl A once, remember the value, and work faster without constantly re-checking.  

"restrict" is that promise.  

If you lie to the compiler with "restrict", if the pointers actually do alias, the behaviour is undefined and you'll get wrong results silently. Only use it when you can guarantee no aliasing.  

Example for Aliasing:-  

Aliasing just means two pointers pointing to the same memory.  

int x = 5;  
int *p = &x;  
int *q = &x;  // p and q are aliases, both point to x  

The poblem with our copy loop which we have written above:  
dst[i] = src[i];  
What is someone called it like this?  

copy(arr+1, arr, 5);  // (Shifting elements left side)  
int arr[] = {10, 20, 30, 40, 50, 60};  

now, copy(arr + 1, arr, 5);  
This means:  
dst = arr + 1 ->points to arr[1]  
srs = arr -> points to arr[0]  
Both pointers point into same array.

Walking Through the Loop
The loop does dst[i] = src[i], which expands to arr[i+1] = arr[i].
Iteration 0: dst[0] = src[0]
arr[1] = arr[0]  →  arr[1] becomes 10

arr: [10] [10] [30] [40] [50] [60]
-----------------↑ changed!
Iteration 1: dst[1] = src[1]
src[1] is arr[1] — but arr[1] is NOW 10, not 20 anymore!

arr[2] = arr[1]  →  arr[2] becomes 10

arr: [10] [10] [10] [40] [50] [60]
Iteration 2: dst[2] = src[2]
src[2] is arr[2] — which is also NOW 10!

arr[3] = arr[2]  →  arr[3] becomes 10
You can see the problem — writing to dst is corrupting the values src hasn't read yet.

Side by Side
What you wanted:------------What you actually got:
[10, 20, 30, 40, 50]--→---[10, 10, 10, 10, 10]
 shifted by 1 cleanly-----everything became 10!

The Key Insight
The overlap means dst is always one step ahead of src in the same array. So every write stomps on a value that src still needs to read in the next iteration.
This is exactly why memmove exists — it detects overlap and copies backwards in this case (starting from the end), so it reads a value before dst can overwrite it.

###### How does "restrict resolve" it?  

"restrict" does not resolve it, "restrict" is a promise you make that the promise don't overlap. It doesn't add any safety or fix anything.  

What actually happens:-  

// You call this  
copy(arr + 1, arr, 5);  

// But the function is declared as  
void copy(int * restrict dst, const int * restrict src, size_t len)  

You just lied to the compiler. You promissed no overlap, but passed overlapping pointers.  

The compiler, trusting your promise, generates fast vectorized code that assumes no aliasing. That fast code produces wrong results silently.No error, no warning, no crash, just bad output.  

So basically "restrict" doesn't resolve overlapping, it assumes the problem doesn't exist.  

It's a speed optimization, not a safety mechanism. You're telling the compiler "trust me, I've already made sure this is safe", and if you haven't, that's entirely on you.  

## Putting it all together, a real embedded register definition  

This is exactly how STM32 HAL and CMSIS define peripheral registers:  

/* UART register map, each register is volatile */  

typedef struct  
{
&emsp;volatile uint32_t SR;  /* Status register  
&emsp;volatile uint32_t DR;  /* Data register  
&emsp;volatile uint32_t BRR; /* Baud rate register  
&emsp;volatile uint32_t CR1; /* Control register 1  
&emsp;volatile uint32_t CR2; /* Control register 2  
}  USART_TypeDef;  

/* Map struct to hardware address */  
#define USART1 ((USART_TypeDef*)0x40011000)  

/* Usage */  
/* Wait until transmit buffer empty */  
while (!(USART1->SR & (1 << 7)));  

/* Send a byte */  
USART1->DR = 'A';  

/* Configure baud rate */  
USART1->BRR = 0x0683;  /* 9600 baud at 16 MHz */  

/* Enable USART */  
USART1->CR1 |= (1 << 13);  

Every field is "volatile" because:  

- "SR" bits are set/cleared by hardware, must re-read every time.  
- "DR" triggers hardware action on both read and write  
- Configuration registers need to actually be written, not cached.  

## Question and Answer  

Q1] What happens if you forgot "volatile" on a hardware register pointer?  

Answer:-  

The compiler may cache the register value in a CPU register and never re-read from the actual hardware address. For a status register, your code would read the same stale value forever, your busy-wait loop would spin forever or never spin. For an output register, multiple writes may be collapsed into one, so hardware never sees intermediate values. This bug typically appears only in optimized builds(O1, O2, Os) and not in debug builds (O0), making it particularly nasty to find.  

Q2] Does "volatile" make an operation atomic?  

Answer:-  

An atomic operation is one that is executed as a single, indivisible unit. It cannot be interrupted or observed in a partially complete state by other threads or CPU cores.

No. "volatile" only guarantees that reads and writes actually happen and are not reodered or eliminated by the compliler. A "volatile" increment (counter++) still compiles to three separate instructions(load, add, store) and can be interrupted between them.For automicity, use interrupt disabling, compiler atomic build-ins, or RTOS synchronization primitives.  

Q3] What is the difference between "const int *p" and "int * const p"?  

Answer:-  

"const int *p" -> the data pointed to is const. You cannot write "*p = value". The pointer "p" itself can be reassigned. Used for function parameters that should not modify the data they receive.  

"int * const p" -> the pointer itself is const. You cannot write "p = &other". The data at the address can still be modified through "*p". Used for a pointer that must always refer to the same location.  

Q4] Why are "const" global variables important in embedded systems specifically?  

Answer:-  

Because the linker places "const" globals in Flash (ROM) rather than SRAM. On microcontrollers, SRAM is the scarce resource, often 10-100x smaller than Flash. Lookup tables, font data, protocol tables, and string constants can be hundreads of bytes to kilobytes. Marking them "const" keeps them in Flash and preserves SRAM for actual runtime data.  

Q5] When would you use "volatile const" together?  

Answer:-  

For hardware registers that are ready only from software's perspective but can change value due to hardware activity. Status registers (UART SR, ADC SR) and chip ID register are examples. "volatile" ensures the compiler re reads from the actual hardware address every time (doesn't cache). "const" prevents accidental writes from software that could trigger undefined hardware behaviour.  

Q6] What is pointer aliasing and how does "restrict" help?  

Answer:-  

Aliasing means two pointers pointing to the same memory. If the compiler can't rule out aliasing, it must assume a write through one pointer might change what's read through another, preventing many optimizations. "restrict" is a programmers's promise that within a scope, no other pointer aliases the restricted one. This let's the compiler vectorize loops, cache values in registers, and reoder loads/stores for maximum throughput. Lying about it is undefined behaviour.  

Q7] Why does this code potentially fail at O2 optimization?  

int flag = 0;  
void wait_for_flag(void)  
{
&emsp;while(flag == 0) {}
}

Answer:-  If "flag" is set by an ISR or another hardware source, the compiler sees no write to "flag" in "wait_for_flag" and optimizes the loop to "while(1){}, loading "flag" once before the loop to while(1){}, loading "flag" once before the loop and never again. The fix is "volatile int flag = 0".  

## Module 4 Summary  

1) const int *p  

Can't change *p, can move p  
Primary use in embedded is I/P buffer parameters, Flash data  

2) int * const p  

Can't move p, can change *p  
Primary use in embedded is Fixed hardware address pointer  

3) const int * const p  

Can't change either  
Primary use in embedded is Read-only register at fixed address  

4) volatile  

Every access must actually happen  
Primary use in embedded is Hardware registers, ISR-shared vars  

5) volatile const  

Must re-read, can't write  
Primary use in embedded is Status registers, chip ID  

6) retrict  

No aliasing, enable optimization  
Primary use in embedded is memcpy style functions, DSP loops.  













