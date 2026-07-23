# Memory Map: How the CPU sees all Memory  

## The Core Idea, "Everything is an address"  

Here's the most important mental shift for embedded engineers.  

"The CPU doesn't know the difference between RAM, Flash or a GPIO pin. It only knows addresses"  

When the CPU wants to read or write something, it puts an address on the bus and reads/writes data.What's at that address, whether it's Flash cell, a RAM cell, or a hardware register that controls a pin, is determined by the memory map.  

CPU says: "Give me what's at address "0x8000000"  
-> Bus routes it -> Flash responds -> CPU gets your program instruction  

CPU says: "Write 0x01 to address 0x40010C0C"  
-> Bus routes it -> GPIO peripheral register responds -> Pin goes HIGH  

This is called "Memory-Mapped I/O", peripherals are controlled by reading/writing to specific addresses, just like memory.  

## The ARM Cortex-M Memory Map  

ARM defined a standard 4GB address space for all Cortex-M processors(32-bit address bus = 2^32 = 4,294,967,296 addresses = 4GB). This space is divided into fixed regions.  

  
0xFFFFFFFF  |------------------------------------------------|  
____________|--------Vendor Specific / Reserved--------------|  
____________|------------------------------------------------|   --> ~0.5GB  
____________|------------------------------------------------|  
0xE0000000  |------------------------------------------------|  
____________|------------------------------------------------|  
____________|-----Private Peripheral Bus (PPB)---------------|  
____________|------(NVIC, SysTick, CoreDebug)----------------|   --> 512MB  
0xA0000000  |------------------------------------------------|  
____________|------------------------------------------------|  
____________|---------External Device Space------------------|   --> 1 GB  
____________|---------(External peripherals)-----------------|  
____________|------------------------------------------------|  
0x60000000  |------------------------------------------------|  
____________|------------------------------------------------|  
____________|----------External RAM region-------------------|   --> 1 GB  
____________|---------(FSMC/FMC SDRAM, SRAM)-----------------|  
____________|------------------------------------------------|  
0x40000000  |------------------------------------------------|  
____________|------------------------------------------------|  
____________|----------Peripheral RAM Region-----------------|   --> 512MB  
____________|--------(GPIO, UART, SPI, YIM,..)---------------|  
____________|------------------------------------------------|  
0x20000000  |------------------------------------------------|  
____________|------------------------------------------------|  
____________|------------SRAM Region-------------------------|   --> 512MB  
____________|-----------(Internal SRAM)----------------------|  
____________|------------------------------------------------|  
0x00000000  |------------------------------------------------|  
____________|------------------------------------------------|  
____________|------------Code Region-------------------------|  
____________|--------(Flash, Boot ROM, ITCM)-----------------|   --> 512 MB  
____________|------------------------------------------------|  
____________|------------------------------------------------|  


Key Insight:  
Only a tiny fraction of this 4GB space is actually populated. The rest is empty (reading it causes a HardFault).  

## STM32F103 Concrete Memory Map  

Let's make this real. Here's the actual memory map of the Blue Pill's STM32F103C8T6:  

|-------------------------------------------------------------------|  
|-Address-------|-What's-there-----------------|--Size--------------|  
|-------------------------------------------------------------------|  
|---------------|------------------------------|--------------------|  
|0xFFFFFFFF-----|-----(Empty---HardFault)------|--------------------|  
|---------------|------------------------------|--------------------|  
|0xE000F000-----|------------------------------|--------------------|  
|-------------------------------------------------------------------|  
|0xE000FFFF-----|------------------------------|--------------------|  
|---------------|-----Core-Debug-Registers-----|--------------------|  
|0xE000EF00-----|------(DWT,-ITM,-TPIU)--------|--------------------|  
|-------------------------------------------------------------------|  
|0xE000EFFF-----|------------------------------|--------------------|  
|---------------|----NVIC-(Interrupt-ctrl)-----|--------4KB---------|  
|0xE000E000-----|--------SysTick,-SCB----------|--------------------|  
|-------------------------------------------------------------------|  
|0x40023000-----|------------------------------|--------------------|  
|---------------|---------(Empty)--------------|--------------------|  
|0x40021000-----|------------------------------|--------------------|  
|-------------------------------------------------------------------|  
|0x40021000-----|-----RCC-(Clock-control)------|--------1KB---------|  
|-------------------------------------------------------------------|  
|0x40013C00-----|--------USART1----------------|--------1KB---------|  
|0x40013800-----|---------SPI1-----------------|--------1KB---------|  
|0x40013400-----|---------TIM1-----------------|--------1KB---------|  
|0x40013000-----|-------ADC-(1-&-2)------------|--------1KB---------|  
|-------------------------------------------------------------------|  
|0x40011C00-----|-----------GPIOE--------------|--------1KB---------|  
|0x40011800-----|-----------GPIOD--------------|--------1KB---------|  
|0x40011400-----|-----------GPIOC--------------|--------1KB---------| <- PC13 is the LED!  
|0x40011000-----|-----------GPIOB--------------|--------1KB---------|  
|0x40010C00-----|-----------GPIOA--------------|--------1KB---------|  
|-------------------------------------------------------------------|  
|0x40000000-----|----APB1-Peripherals-start----|--------------------|  
|---------------|-------(TIM2-7,-UART2-3,)-----|--------------------|  
|---------------|-----(SPI2,-I2C1-2,-etc.)-----|--------------------|  
|-------------------------------------------------------------------|  
|0x20004FFF-----|---------^SRAM-End------------|--------------------|  
|---------------|-------Internal-SRAM----------|--------20KB--------|  
|0x20000000-----|---------SRAM-Start-----------|--------------------|  
|-------------------------------------------------------------------|  
|0x1FFFF800-----|----Option-Bytes--------------|--------16B---------|  
|0x1FFFF000-----|--System-Memory-(Bootloader)--|--------2KB---------|  
|-------------------------------------------------------------------|  
|0x0800FFFF-----|-------^Flash-End-------------|--------------------|  
|---------------|------Internal-Flash----------|--------64KB--------|  
|0x08000000-----|--------Flash-Start-----------|--------------------|  
|-------------------------------------------------------------------|  
|0x00000000-----|-----Aliased-to-Flash-or------|--------------------|  
|---------------|---SRAM-(boot-mode-config)----|--------------------|  
|-------------------------------------------------------------------|  

## The code Region (0x00000000)  

This is subtle and confuses many begineers. Address "0x00000000" is aliased, it doesn't point to a fixed physical location. Instead, what it points to depends on the BOOT pins:  

BOOT1------BOOT0-----|0x00000000-aliases-to-----------|Boots-from........|  

X-----------0--------|Flash-(0x08000000)--------------|Your-program------|  
0-----------1--------|System-Memory-(0x1FFFF000)------|ST-Bootloader-----|  
1-----------1--------|SRAM-(0x20000000)---------------|RAM-(debug use)---|  

Why does this matter?  
The CPU always starts executing from address "0x00000000" at reset. By aliasing it to Flash, your program runs. By aliasing to System Memory, you can flash via UART without a programmer!  

## The Bus Architecture, How Data Actually Travels  

|--------------CPU (Cortex-M3)----------------|  
<---------------------|-----------------------> ICode Bus (Instruction from flash)  
<---------------------|----------------------->DCode Bus (Data from Flash,const,literals)     
<---------------------|-----------------------> System Bus (SRAM + Peripheral)    
<---------------------|----------------------->  
<---------------------V----------------------->  
|-----------------|..AHB-Bus..|---------------| <- High speed bus(same speed as CPU clock)  
|-----------------|..Matrix...|---------------|  
<-------------------|-------|----------------->  
<-------------------|-------|----------------->  
<-------------------V-------V----------------->  
|----------------|SRAM|---|Flash|-------------| <- Directly on AHB (fast)    
|----------------|20KB|---|64KB|--------------|  
<------------------|-------------------------->  
<------------------|-------------------------->  
<------------------V-------------------------->  
|------------|..AHB-to-APB..|-----------------| <- Bridge (adds latency)  
|------------|..Bridge......|-----------------|  
<--------------|----------|------------------->  
<--------------|----------|------------------->  
<--------------|----------|------------------->  
<--------------V----------V------------------->  
|------------|APB2|-----|APB1|----------------|    
|------------|fast|-----|slow|----------------|  
<--------------|----------|------------------->  
<--------------|----------|------------------->  
<--------------V----------V------------------->  
|--------|GPIO,SPI1|--|UART2-5|---------------|  
|--------|USART1|-----|SPI2,I2C|--------------|  
|--------|ADC|--------|TIM2-7|----------------|  

Wht does this matter?  

Bus-----------Max-Speed(STM32F103)------------What's on it  

AHB--------------72MHz--------------------CPU,SRAM,Flash,DMA  
APB2-------------72MHz--------------------GPIO,SPI1,USART1,ADC  
APB1-------------36MHz--------------------I2C,SPI2,USART2-5,most timers  

APB1 runs at HALF the CPU speed. This means:  

- I2C peripheral clock <= 36MHz  
- Timers on APB1 get 72MHz (there's a *2 multiplier for timers when APB1 prescaler != 1)  
- When you configure baud rates, you need to know which bus your peripheral is on!  

## Three Separate Buses to Flash - ICode, DCode, System  

This is an often missed detail in ARM Cortex-M:  

|---------Flash Memory (0x08000000)-------------|  
|---------------------|-------------------------|  
|---------------------|-------------------------|  
|------------_________|_________----------------|  
|-----------|-------------------|---------------|  
|-----------V-------------------V---------------|  
|-------ICode-Bus---------DCode-Bus-------------|  
|-------(Instructions)----(Data/Constants)------|  
|-----------|-------------------|---------------|  
|-----------V-------------------V---------------|  
|-----CPU-fetch-Unit---CPU-Load/Store-Unit------|  

The CPU has two separate buses going to Flash:  

- ICode: Fetches instructions (your code)  
- Dcode: Fetches data from Flash (const arrays, string literals)  


This means the CPU can fetch the next instruction AND read a "const" table from Flash simultaneously, no stall! This is a hardware level pipeline optimization you get for free.  

The system bus handles everything else, SRAM reads/writes, peripheral access.  

## Flash Wait States, A Critical Performance Detail  

Flash is slower than the CPU at high clock speeds. The CPU has to wait for Flash to respond:  

STM32F103 Flash wait states:  



|----SYSCLK-----------|--Wait-States(LATENCY)------|  
|----0-24-MHz---------|--0(no-wait)----------------|  
|----24-48MHz---------|--1-wait-state--------------|  
|----48-72MHz---------|--2-wait-states-------------|  



At 72MHz with 2 wait states:  

Clock:--|^^|__|^^|__|^^|__|^^|__|^^|__|  
Flash:--[-request-][data-out]    
CPU:----[-fetch-][WAIT][WAIT][execute]  
|-------------------^-----^------------------------|    
|-------------wasting-2-cycles-waiting-for-Flash!--|  

This is why prefetch buffer and cache exist, to hide this latency. STM32F103 has a prefetch buffer that reads 64 bits at a time from Flash, so sequential instructions don't all pay the full wait state penalty.  

## The SRAM Region, 0x20000000  

SRAM starts at "0x20000000". On STM32F103C8, it's 20KB, so it goes from "0x20000000" to "0x20004FFF".  

0x20004FFF  
|----------------------------|  
|---------Stack------------|<-Grows DOWNWARD v
|---(local vars,-----------|  
|----return address)-------|  
|----------------------------|<-SP-(Stack-Pointer)-moves-here
|-----(Free-space)---------|  
|----------------------------|<-Heap-grows-UPWARD ^
|--------Heap--------------|  
|----(malloc-space)--------|  
|----------------------------|  
|--------(.bss)------------|<-Uninitialized-globals-(zeroed)  
|----------------------------|  
|--------(.data)-----------|<-Initialized-globals-(copied-from-Flash)  
|----------------------------|  
0x20000000  

We'll go much deeper on ".bss", ".data", stack and heap on next Module.  

## System Memory, The hidden Bootloader  

At "0x1FFFF000" on STM32F103 lives 2Kb of factory-programmed bootloader from ST. You can't erase it. When you set BOOT0 = 1, BOOT1 = 0 and reset the chip, the alias makes "0x00000000" point here.  

This bootloader:  

- Communicates over UART1 (and USB/SPI on other STM32 families)  
- Accepts the STM32 protocol (used by "stm32flash", STM32CubeProgrammer)  
- Lets you flash your MCU without a JTAG/SWD prgrammer, just a USB-to-UART adapter!  

Embedded carrer tip: Always know your MCU's bootloader capabilities. It can save you when a customer's device is bricked in the field.  

## Reading the Memory Map in the Reference Manual  

Every MCU has a Reference Manual(RM). The memory map is always in Chapter 2 or 3. Here's how to read it:  

|---Boundary-address---|--Peripheral--|--BUS--|  
|--0x4001-3C00-3FFF----|--USART1------|APB2---|  
|--0x4001-3800-3BFF----|--SPI---------|APB2---|  
|--0x4001-3000-37FF----|ADC1-ADC2-----|APB2---|  
|--0x4001-1000-13FF----|GPIOA---------|APB2---|  

Make it a habit: When you start with a new MCU, open the reference manual and spend 20 minutes reading the memory map chapter. You'll save hours of debugging later.  


## Practical implications, Real Bugs from not knowing this  

### Bug 1: Forgetting "volatile"  

uint32_t *reg = (uint32_t*)0x40010C08;  // IDR register  
while(*reg == 0);  // Wait for pin to go high  

Without "volatile", the compiler reads once, caches in a CPU register, and loops forever, the actual memory is never re-read!  

### Bug 2: Accessing wrong bus address  

You wanted to access UART2 (APB1: 0x40004400)  
But typed UART1's address (APB2: 0x40013800)  
Both are valid addresses, no crash, just wrong behaviour  

### Bug 3: Unaligned access  

ARM requires word accesses to be 4-byte aligned:  

uint8_t buffer[5];  
uint32_t *p = (uint32_t*)(&buffer[1]); // Not aligned to 4 bytes!  
uint32_t val = *p;  // Hardfault on Cortex-M0! (M3/M4 handle it buy slowly)  

Explanation of above code:-  

Step 1: What does "aligned" mean?  

A "uint32_t"(4 bytes) is "4-byte aligned" if it's memory address is a multiple of 4, like 0,4,8,12,16,20,100,1000,etc.  
ARM Cortex-M cores load 32-bit values fastest (or, on M0, only) when they start at one of these multiple of 4 addresses. This is because CPU's data bus fetches memory in aligned 4-byte chunks, like grabbing a whole box of 4 items at once. If the box boundaries don't line up with where your data starts, hardware either can't do it (M0) or has to do extra work (M3/M4: grab twp boxes and stitch peices together).  

Step 2: Let's use real addresses  

Suppose "buffer" happens to be placed in memory starting at address "0x2000_0000 (this is aligned nicely to 4, as arrays often are, though not guranteed).  

uint8_t buffer[5];  

Since buffer is an array of uint8_t (1 byte each), each element sits at consecutive addresses:  

Element----------------------Addresses  

buffer[0]--------------------0x2000_0000  
buffer[1]--------------------0x2000_0001  
buffer[2]--------------------0x2000_0002  
buffer[3]--------------------0x2000_0003  
buffer[4]--------------------0x2000_0004  

Step 3: Now look at what you're doing  

uint32_t *p = (uint32_t*)(&buffer[1]);  
"&buffer[1]" is address 0x2000_0001.  
Is it a multiple of 4? No, it's not 4-byte aligned.  

You've just told the compiler: "Treat the byte at 0x2000_0001" as the start of a 4-byte uint32_t". So when you do:  

uint32_t val = *p;  
we are asking the CPu to read 4 bytes starting at 0x2000_0001, i.e. bytes at addresses:  

0x2000_0001, 0x2000_0002, 0x2000_0003, 0x2000_0004  
(which happen to be "buffer[1] through buffer[4], that's fine conceptually, you do want those 4 bytes). The problem is purely about where that 4-byte window starts.  

Step 4: Why the starting address matters to hardware  

Cortex-M's "LDR" (load register) instruction, under the hood, doesn't crawl byte-by-byte. It issues one bus transaction asking for "the 4-byte word at address X", and the hardware/memory controller assumes X is a multiple of 4 so it can fetch a whole naturally aligned word in one shot.  

- If X = 0x2000_0000 (multiple of 4) -> fine, one clean fetch.  
- If X = 0x2000_0001 (not a multiple of 4) -> the requested 4 bytes actually straddle two aligned words in memory:  
--> Word A: 0x2000_0000 -> 0x2000_0003  
--> Word B: 0x2000_0004 -> 0x2000_0007  

Your 4 bytes (0x1,0x2,0x3,0x4) are the last 3 bytes of Word A plus the first byte of Word B. The CPU has to fetch both words and splice together the right bytes from each.  

## Module 2 Summary  

- CPU sees ONE flat 4GB address space  
- Different addresses -> different hardware  
- Flash at 0x08000000, SRAM at 0x20000000  
- Peripherals at 0x40000000+ (memory-mapped I/O)  
- Boot pins alias 0x00000000 -> Flash/ROM/SRAM  
- AHB is fast, APB1 is half speed  
- Flash needs wait states at high CPU speeds  
- Bit-banding enables atomic bit manipulation  
- volatile is mandatory for hardware registers  












