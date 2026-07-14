# Memory types: What they are & where they live  

### The Big Picture First  

A microcontroller is a single chip that contains:  

- A CPU core (does computation)  
- Memory (stores code + data)  
- Peripherals (GPIO, UART, SPI, etc)  

All of these are on the same silicon die(or very close on the same package). This is what separates a microcontroller from a microprocessor, everything is integrated.  

Now, why do we need multiple types of memory? Because no single memory technology can be fast, non-volatile, dense, and cheap all at once. Each type makes a different trade-off.  

## 1. Flash Memory (Program Memory)  

#### What is it?  

Flash is non-volatile, ,meaning it retains data even when power is off. It's built using floating gate transistors, charge is trapped in an insulated gate to store a 0 or 1.  

#### Where does it sit on the board?  

On most MCUs (STM32, AVR, PIC, etc.), Flash is internal, baked right into the MCU chip itself. You don't see it as a separate component.  


|--------------------------------------------|
|--------------------------------------------|  
|--------------------------------------------|  
|------CPU--------Flash----------------------| <- 64KB lives INSIDE this chip  
|-----(ARM)-------(64KB)---------------------|    
|--------------------------------------------|    
|------------------------SRAM----------------|    
|-----------------------(20KB)---------------| 
|--------------------------------------------|  

#### Purpose -  

What it stores  

1) Your compiled program (instructions)  
Example:-  main(), ISRs, all functions  

2) "const" variables  
Example:-  Lookup tables, string literals  

3) Interrupt vector table  
Example:-  Address of every ISR  

4) Initial values of global variables  
Example:- int x = 5; the "5" lives in Flash  

#### Key Characterstics  

- Read: Fast (1 clock or with wait states at high speeds)  
- Write (Erase/Program): SLOW, must erase entire page/sector before writing  
- Endurance: 10,000 to 1,00,000 erase cycles (then it wears out!)  
- Typical sizes on MCUs: 16KB -> 2MB  
- Voltage: Needs higher voltage internally to write (~12V generated on-chip via charge pump)  

#### Why can't we just use Flash for everything?  

- You can't write to it freely at runtime (not byte-writable without erase)  
- It's slower to write than RAM  
- Repeated writes wear it out.  

## 2. SRAM (Static Random Access Memory)  

### What is it?  

SRAM uses 6 transistor per bit (a flip flop) to store data. It holds it's value as long as power is on, no refresh needed (unlike DRAM).  

### Where does it sit?  

Again, internal to the MCU chip on most microcontroller. This is your primary working memory.  

### Purpose  

What it stores--------------------------------------------Example-------------  

Local variables<--------------------------------> "int i = 0;" inside a function  
Stack (function calls, return addresses)<-------> Every function call uses this  
Heap (dynamic memory)<--------------------------> "malloc()" allocations   
Global/static variables (runtime values)<-------> "int counter = 0;"  
Peripheral buffers<----------------------------->UART receive buffer  

### Key Characteristics  

- Read/Write: Very fast, same speed as CPU (often 1 clock cycle)  
- Volatile: Loses everything on power loss  
- No wear-out: Can be written infinite times.  
- Expensive per bit: 6 transistors vs Flash's 1, so MCU's have much less SRAM than flash.  
- Typical sizes: 2KB (tiny AVR) -> 512KB (STM32H7)  

#### The Golden Rule in embedded  

Flash is always bigger than SRAM. A typical ratio is 4:1 or 8:1 (e.g, 256KB Flash/ 64KB SRAM).This shapes every decision you make about how you write code.  

## 3. EEPROM (Electricity Erasable Programmable Read-Only Memory)  

### What is it?  

Like Flash but byte-erasable. You can erase and rewrite individual bytes instead of entire pages/sectors.  

### Where does it sit?  

- Internal: Some MCUs have a small dedicated EEPROM block (AVR ATmega has it, some STM32 simulate it)  

- External: A separate IC on the board connected via I2C or SPI (e.g AT24C256)  

|-----------------| <---------> |-------------|
|-------MCU-------| <---ICU---> |-------------|
|-----------------| <---------> |---AT24256---| 
|-----------------| <---------> |(32KB EEPROM)|  
|-----------------| <---------> |-------------|  

### Purpose  

Storing small amounts of data that must survive power cycles but change occasionally:  

- Calibration values  
- User settings / configuration  
- Device serial numbers  
- Error logs / fault counters  
- Last known state  

### Key characteristics  

- Byte writable: Can update 1 byte at a time (huge advantage over Flash)  
- Endurance: ~100000 to 1000000 write cycles per byte  
- Speed: Slow write (I2C at 400kHz, multiple ms per write)  
- Size: Small, KBs, not MBs  
- Non-volatile: Retains data without power  

## 4. Registers, The CPU's Own Memory  

### What are they?  

Registers are tiny, ultra-fast memory locations inside the CPU core itself, not on the bus, not on the chip's memory map (well, peripheral registers are, but CPU registers aren't addressable).  

### Types:  

#### a) CPU Core Registers (ARM Cortex-M example)  

R0-R12 -> General purpose (math, data movement)  
R13 (SP) -> Stack Pointer (points to current stack top)  
R14 (LR) -> Link Register (stores return address for function calls)  
r15 (PC) -> Program Counter (address of next instruction)  
xPSR -> Status register (Zero flag, Carry flag, etc)  

#### b) Peripheral registers  

These are memory-mapped (we'll cover this in next module). Every peripheral, UART, GPIO, TIMER, is controlled by writing to specific adresses that map to hardware registers inside the peripheral.  

// You're not writing to RAM. You're writing to a hardware register.  
GPIOA->ODR = 0x0001; // This address maps to GPIO hardware.  

## 5. Cache Memory (Present in high-end MCUs)  

### What is it?  
A small, ultra fast buffer between the CPU and Flash/RAM. When you read instruction from Flash, the CPU checks cache first.  

- I-Cache: Instruction cache (catches Flash reads for code)  
- D-Cache: Data cache (catches SRAM reads/writes)  

### Where?  
Inside the CPU core itself. Found in Cortex-M7 (STM32H7, STM32F7), NOT in Cortex-M0/M3/M4.  

### Why does it matter?  

Without cache: CPU at 216MHz, Flash at ~30MHz -> CPU waits -> WASTE  
With I-Cache: CPU reads from cache at 216MHz -> No wait -> FAST  

## 6. External RAM (SRAM/SDRAM/PSRAM)  

### When is it needed?  

When your application needs more RAM than the MCU provides internally:  

- Framebuffers for displays (a 320*240 RGB565 display needs 150KB just for one frame!)  
- Audio buffers  
- Large data processing  

### Types on boards  

Type------------Speed----------Size-------------Interface  
|--------------------------------------------------------------------------|
External SRAM<--->Fast<------>512KB-8MB<-->Parallel bus (FSMC/FMC on STM32)  
|--------------------------------------------------------------------------|
SDRAM<--------->Very fast<--->8MB-64MB<------>Parallel,with clock  
|--------------------------------------------------------------------------|
PSRAM<--------->Moderate<---->2MB-8MB<------->SPI or QSPI  
|--------------------------------------------------------------------------|  

### Board examples:-  

- STM32F429 - Discovery: Has 8MB SDRAM on board for LCD framebuffer  
- ESP32 - Can connect external PSRAM via SPI  

## 7. External Flash (NOR/NAND/QSPI Flash)  

### Why external Flash?  

Internal Flash on MCU's maxes out around 2MB.  

- Large firmware images  
- Audio/image/font assets  
- Filesystem storage  

You need external Flash  

Type-----------------------Use case--------------Interface
|-------------------------------------------------------------|  
NOR Flash<--->Execute in place (XIP), random read<-->SPI/QSPI  
|-------------------------------------------------------------|  
NAND Flash<-->High-density storage, filesystems<-->Parallel/SPI  
|--------------------------------------------------------------|  
eMMC<-------->Even higher storage, filesystems<--->MMC bus  

### Real Board Example, STM32 Blue Pill  

Blue Pill PCB  
|----------------------------------------------------------------------------------|  
|----------------------------------------------------------------------------------|  
|-----STM32F103C8T6-------------<-Everything inside this one-----------------------|  
|---------------------------------chip---------------------------------------------|  
|-------*ARM Cortex-M3------------64KB Flash---------------------------------------|  
|-------*64KB Flash---------------20KB SRAM----------------------------------------|  
|-------*20KB SRAM----------------All peripherals----------------------------------|  
|-------*GPIO, UART, SPI-----------------------------------------------------------|  
|----------------------------------------------------------------------------------|  
|----------------------------------------------------------------------------------|  
|-------| XTAL |------<--8MHz Crystal (clock source)-------------------------------|  
|------------------------NOT memory, but related to--------------------------------|  
|------------------------how fast memory is accessed-------------------------------|  
|----------------------------------------------------------------------------------|  
|----------------------------------------------------------------------------------|  
|----No external RAM, No external Flash--------------------------------------------|  
|----Everything fits in internal memory--------------------------------------------|  
|----------------------------------------------------------------------------------|  


Module 1 Summary  

- Flash: Stores your program. Non-volatile, slow to write, wears out. Lives inside MCU.  
- SRAM: Your working memory. Fast, volatile, unlimited writes. Lives inside MCU.  
- EEPROM: Byte writable persistent storage for settings. Small and slow.  
- Registers: CPU's internal scratch pad + hardware control points.  
- Cache: Speed bridge between CPU and slower memories (high end MCUs)  
- External memories: Used when internal isn't enough (displays, filesystems).  

