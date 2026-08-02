# Code Execution: What Lives Where When Your Program Runs  

## The Big Picture First  

When you hit "Build" in your IDE, your C code goes through this pipeline:-  

|---------Your-(.c)-files-----------|  
|------------|----------------------|  
|------------V----------------------|  
|---Compiler(gcc-arm-none-eabi)-----|  
|------------|----------------------|->Translates C -> Assembly -> Machine code  
|------------V----------------------|  
|--------Assembler------------------|  
|------------|----------------------|-> Produces (.o) object files  
|------------V----------------------|  
|--------Linker(ld)-----------------|  
|------------|----------------------|-> Combines all (.o) files, Uses linker script  
|------------V----------------------|-> to decide where everything goes in memory  
|--------(.elf-file)----------------|-> .bin/.hex file  
|------------|----------------------|  
|------------V----------------------|  
|---Flashed-into-MCU-Flash_memory---|  

The linker script is the map that decides which piece of code/data goes to which address. We'll go deep on that in next Module. In this we will focus on what the final result looks like at runtime.  

## Memory Sections, The Fundamental Building Blocks  

Your compiled program is divided into sections. Each section has a specific type of content and lives in a specific memory:  

|--------------------------------------------------------|  
|------------------FLASH-(0x08000000)--------------------|  
|--------------------------------------------------------|  
|-----Vector-Table-(.isr_vector)-------------------------|  
|-----.text-(your-compiled-code-all-functions)-----------|  
|-----.rodata-(read-only-data-const,string-literals)-----|  
|-----.data-(INIT-VALUES)<-copy-of-initialized-globals---|  
|--------------------------------------------------------|  

|--------------------------------------------------------|  
|--------------------SRAM-(0x20000000)-------------------|  
|--.data-(RUNTIME-COPY-initialized-globals)--------------|<-copied-from-Flash-at-startup  
|--.bss-(uninitialized/zero-initialized-globals)---------|<-zeroed-at-startup  
|---Heap-(dynamic-memory-malloc/free)--------------------|<-grows-^-upward  
|----------[free-space]----------------------------------|  
|---Stack-(local-vars,function-calls)--------------------|<-grows-v-downward  
|---0x20004FFF-(top-of-SRAM)<---Stack-starts-here--------|  
|--------------------------------------------------------|  


## 1. Vector Table (.isr_vector), The first thing in Flash  

The very first thing at "0x80000000" is not your code. It's the vector table, a table of 32-bit addresses that tell the CPU where to jump when exceptions/interrupts occur.  

|----Address-----------|--Entry----|----What-it-holds--------------------------------|  
|---0x08000000---------|--Entry-0--|Initial-Stack-Pointer-value----------------------|  
|---0x08000004---------|--Entry-1--|Reset-Handler-address<-CPU-jumps-here-on-reset!--|  
|---0x08000008---------|--Entry-2--|NMI-Handler-address------------------------------|  
|---0x0800000C---------|--Entry-3--|Hardfault-Handler-address------------------------|  
|---0x08000010---------|--Entry-4--|Memmanage-Handler--------------------------------|  
|---0x08000014---------|--Entry-5--|BusFault-Handler---------------------------------|  
|---0x08000018---------|--Entry-6--|UsageFault-Handler-------------------------------|  
|---....---------------|---....----|....---------------------------------------------|  
|---0x08000058---------|--Entry-22-|EXTI0-IRQ-Handler(external-interrupt-0)----------|  
|---....---------------|---....----|....---------------------------------------------|  
|---0x0800014C---------|--Entry-83-|Last-STM32F103-interrupt-------------------------|  

Entry 0 is special, it's not a function address. It's the initial value of the Stack Pointer. The CPU hardware reads this value at reset and loads it into SP before anything else happens.  

// In startup_stm32f103xb.s-(assembly-startup-file):  
g_pfnVectors:  
&emsp;.word _estack  // 0x20004FFF+1 = top of SRAM -> initial SP  
&emsp;.word Reset_Handler  // What runs on reset  
&emsp;.word NMI_Handler  
&emsp;.word HardFault_Handler  
&emsp;//...all-your-interrupt-handlers  

At reset, the CPU hardware does exactly 2 things:  

1. Reads "0x00000000" -> loads into SP (Stack Pointer)  
2. Reads "0x00000004" -> loads into PC(Program Counter) -> jumps there  

## 2. .text Section, Your Code  

".text" contains all compiled machine instructions, every function you write, every library function linked in.  

// This C code:  

int add(int a, int b)  
{  
&emsp;return a+b;  
}  

// Becomes this in ".text (ARM Thumb instructions):"  
// 0x08000200: PUSH {r7}  
// 0x08000202: ADD r7,sp, #0  
// 0x08000204: STR r0, [r7, #4]; store parameter 'a'  
// 0x08000206: STR r1, [r7, #0]; store parameetr 'b'  
// 0x08000208: LDR r0, [r7, #4]; load 'a'  
// 0x0800020A: LDR r1, [r7, #0]; load 'b'  
// 0x0800020C: ADD r0, r0, r1; a + b  
// 0x0800020E: POP {r7}  
// 0x08000210: BX lr; return  

key facts about ".text":  

- Lives in Flash permanently  
- Never copied to RAM (unless you explicitly do it, we'll see why we would do it later)  
- CPU fetches instructions directly from Flash via the ICode bus  
- "const" functions, "static" functions, ISR, everything goes here  

## 3. .rodata section, Read Only Data  

".rodata" (Read Only Data) holds anything "const" and string literals:  

const int lookup_table[8] = {0, 1, 4, 9, 16, 25, 36, 49}; // -> .rodata  
const char msg[] = "Hello World";  
char *ptr = "Hello"; // The string literal "Hello" -> .rodata  
// ptr itself (the pointer) -> .data or stack  

Lives in Flash alongside ".text". The CPU reads it via the DCode bus. You cannot write to it, attempting to do so causes a HardFault.  

const int x = 5;  
int *p = (int*)&x;  
*p = 10;  // HardFault! Writing to Flash address without erase  

## 4. .data section, Initialized Global and Static Variables  

This is where embedded gets interesting. ".data" has a split personality:  

int counter = 100;  // -> .data  
static int state = 42;  // -> .data  
uint8_t buffer[4] = {1,2,3,4};  // -> .data  

The Problem:-  

These variables need their initial values(100, 42, {1,2,3,4}) to survive power-off. So the values must be in Flash. But they also need to be writable at runtime, so they must be in SRAM.  

The Solution: Two Locations:-  

FLASH:  
|-------------|
|----.text----|  
|-(your-code)-|  
|-------------|  
|---.rodata---|  --> permanent backup  
|-------------|  
|--.data-INIT-|  
|(init-values)|  
|100,42,1,2,3,4|  
|-------------|  
^  -> copy at boot 
|  
SRAM:  
|-------------|  
|---.data-----|  
|--counter=100|  --> CPU reads/writes here  
|--state=42---|  
|-buffer={..}-|  
|-------------|  

The linker assigns two addresses to ".data":  

- LMA (Load Memory Address): where init values sit in Flash  
- VMA (Virtual Memory Address): where the variable lives at runtime in SRAM  

The startup code copies from LMA -> VMA before main() runs.  

// Simplified version of what startup code does:  
extern uint32_t _sdata;  // Start of .data in SRAM (VMA)  
extern uint32_t _edata; // End of .data in SRAM  
extern uint32_t _sidata; // Start of .data init values in Flash (LMA)  


// Copy .data from Flash to SRAM  

uint32_t *src = &_sidata;  
uint32_t *dst = &_sdata;  
while(dst < &_edata)  
{  
&emsp;*dst++ = *src++;  
}  

## .bss section, Zero initialized variables  

int uninit_global;  // -> .bss  
static int counter;  // -> .bss  
uint8_t large_buffer[1024];  // -> .bss (1KB of zeros in SRAM)  

Variables with no initial value (or explicitly initialized to 0) go in ".bss"  

Key insight: ".bss" takes NO space in Flash!  

Flash:  
|------------------|  
|---.text----------|  
|------------------|  
|----.rodata-------|  
|------------------|  
|--.data_INIT------|  
|(just-a-few-KB)---|  
|NO-.bss-here!-----|  
|(just-stores-the)-|  
|start/end-addres--|  
|------------------|  

SRAM:  
|------------------|  
|---.data-(copied)-|  
|------------------|  
|-----.bss---------|  -> zero fill   
|--(all-zeros)-----|  
|--1024-bytes-of-0-|  
|------------------|  


The startup code just needs to zero-fill the SRAM range for ".bss":  

extern uint32_t _sbss;  // Start of .bss in SRAM  
extern uint32_t _ebss;  // End of .bss in SRAM  

// Zero out .bss  
uint32_t *bss = &_sbss;  
while(bss < &_ebss)  
{  
    *bss++ = 0;  
}  

Practical impact:  

uint32_t big_array[10000];  // .bss -> 0 bytes in Flash, 10KB in SRAM  
uint32_t init_array[10000] = {1};  // .data -> 10KB in Flash AND 10KB in SRAM!  

Always use ".bss" when you don't need specific initial values. It saves Flash space.  


