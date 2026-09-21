# Advanced Memory: Cache, MPU, DMA, Flash Wear & Carrer Mastery  

### What Separates a Junior from a Senior Embedded Engineer  

Module 1-4 covered what memory is and how it's organized. Today is about is about what happens when memory interacts with the rest of the system, and this is where real bugs live, the ones that take days to find, the ones that make you question your sanity at 2am.  

Every topic in this module is something a senior engineer has been burned by at least once.  

## 1. Cache Memory(Deep Dive)  

What is Cache?  

Cache is a small, ultra fast SRAM buffer inside the CPU core that transparently stores recently accessed data/instructions. When the CPU needs something, it checks cache first:  

|--------CPU-requests-address-X---------|  
|------------------|--------------------|  
|------------------V--------------------|  
|------------Cache-Lookup---------------|  
|------------------|--------------------|  
|------------______________-------------|  
|-----------|--------------|------------|  
|----------HIT-----------MISS-----------|  
|-----------|--------------|------------|  
|-----------V--------------V------------|  
|----Return-cached---Fetch-from-Flash/--|  
|----data(fast!)-----RAM(slow!)-Store---|  
|-------------------in-cache-Return-data|  

Cache on ARM Cortex-M  

Core---------------I-Cache------------D-Cache-----------Cache-Size  

Cortex-M0/M0+--------X-------------------X---------------None  
Cortex-M3------------X-------------------X---------------None  
Cortex-M4------------X-------------------X---------------None(has-flash-accelerator-instead)  
Cortex-M7----------Present-------------Present-----------4-64KB each  
Cortex-M33---------Optional------------optional----------Vendor-dependent  

So on STM32F4 (M4), there's no cache, but there's a Flash ART Accelerator (Adaptive Real Time) that works similarly for instructions only. On STM32H7 (M7), there's full I-Cache + D-Cache.  

### Enabling Cache on Cortex-M7 (STM32H7)  

// Done in SystemInit() or early in main():  

// Enable I-Cache  
SCB_EnableICache();  
// Equivalent to:  
SCB->ICIALLU = 0UL;  // Invalidate entire I-Cache  
__DSB(); __ISB();  // Memory barriers  
SCB->CCR |= SCB_CCR_IC_Msk; // Enable I-Cache bit  
__DSB(); __ISB();  

// Enable D-Cache  
SCB_EnableDCache();  
// Invalidate + enable D-Cache  

### Cache Lines  

Cache doesn't store single bytes. It stores cache lines, typically 32 bytes on Cortex-M7.  

You access address 0x20001004 (1 byte)  
-> Cache fetches the entire 32-byte line:  
0x20001000 - 0x2000101F -> all stored in cache  
-> Next 31 bytes are "free" -> already in cache  
-> This is spatial locality, the basis of cache efficiency  

#### The Cache Coherency problem, The #1 DMA Bug  

This is the most common bug on high-end MCUs with cache. Here's the scenario:  

uint8_t rx_buffer[256];  // in D-Cached SRAM  

// Step 1: CPU writes some data to rx_buffer  
// -> Data goes to D-Cache first (NOT yet in actual SRAM!)  

// Step 2: You start a DMA transfer, DMA fills rx_buffer from UART  
DMA_Start(USART1, rx_buffer, 256);  

// Step 3: DMA completes  
// -> DMA wrote to ACTUAL SRAM directly (DMA bypasses cache!)  
// -> But D-Cache still has OLD data!  

// Step 4: CPU reads rx_buffer  
uint8_t first_byte = rx_buffer[0];  
// -> CPU reads from D-Cache -> gets STALE DATA!  
// -> The actual new data from DMA is sitting in SRAM, ignored!  

##### The core problem: DMA and CPU cache don't know about each other  

on a Cortex-M7 (or similar cached core), the CPU doesn't talk to SRAM directly for every access. It goes through the D-Cache first:  

CPU <--> D-Cache <--> SRAM  

But DMA is a separate hardware engine that moves data directly between peripherals (like UART) and SRAM, it has no idea the D-Cache exists:  

DMA <--> SRAM (cache is NOT in this path)  
This is the root of the bug: two different paths to the same memory, and only one of them updates the cache.  

Walking through your example:  

Step 1 - CPU writes to "rx_buffer":  
When the CPU writes, the value typically lands in D-Cache first (with write-back caching, it may not even reach SRAM yet, it sits "dirty" in cache).  

Step 2 - DMA_Start()  
You kick off a DMA transfer. The DMA controller is told: "write incomming UART bytes to address "rx_buffer" in SRAM."  

Step 3 - DMA completes  
The DMA controller writes the 256 new bytes directly into SRAM, bypassing the cache entirely. From the cache's point of view, nothing happened, it still holds the old cached copy of that memory region, and doesn't know it's now wrong(stale).  

Step 4 - CPU reads "rx_buffer[0]"  
The CPU checks D-Cache first, finds a "valid" cached line for that address, and returns the old value, not the fresh byte the DMA just wrote. This is a cache coherency bug: cache and SRAM disagree, and the CPU trusts the wrong one.  

Why this is dangerous  

It's not a crash, it's silent data corruption. Your code runs "fine", but "rx_buffer" contains garbage/old data instaed of what UART actually received. These bugs are notoriously hard to debug because:  

- It often works in the debugger (because debugger memory views may bypass cache, or timing changes)  
- It may onlt show up intermittently, depending on cache state.  
- It "looks correct" in code review.  

Common Fixes:  
 
1) Invalidate the D-Cache for that memory region after DMA completes, before the CPU reads it:  

SCB_InvaliateDCache_by_Addr((uint32_t*)rx_buffer, 256);  
This forces the CPU to discard it's stale cached copy and re-fetch from actual SRAM.  

2) Place DMA buffers in a non-cacheable memory region, configure the MPU (Memory Protection Unit) to mark that SRAM section as non-cacheable, so the CPU always goes straight to SRAM for it.  

The same problem exists in reverse:-  
If CPU writes to a buffer that DMA is about to read from (TX direction), you need to clean/flush the cache first, so the dirty CPU-written data actually reaches SRAM before DMA reads it.  

## 2. MPU (Memory Protection Unit)  

### What is MPU?  

The MPU is a hardware unit that enforces access rules on memory regions. It sits between the CPU and the bus and raises a MemManage fault if a rule is violated.  

Available on: Cortex-M0+, M3, M7, M23, M33 (not base M0)  

CPU tries to access address X  
|------------|--------------|  
|------------V--------------|  
|-----------MPU-------------| <- checks rules  
|------------|--------------|  
|-------|----------|--------|  
|----ALLOWED-----DENIED-----|  
|-------|----------|--------|  
|-------V----------V--------|  
|----Access-----MemManage---|  
|----granted------Fault!----|  
|---------------------------|  

### What MPU can do  

|------------------------------------------------------|  
|---MPU-Capabilities:----------------------------------|  
|------------------------------------------------------|  
|-Stack-overflow-detection-----------------------------|  
|---->Mark-region-below-stack-as-No-Access-------------|  
|---->Stack-overflow->immediate-MemManage-fault--------|  
|---->You-know-exactly-where-and-why-------------------|  
|------------------------------------------------------|  
|-Protect-Flash-from-accidental-writes-----------------|  
|---->Mark-Flash-as-read-only--------------------------|  
|---->Stary-pointer-write->fault,not-silent-corupt-----|  
|------------------------------------------------------|  
|-RTOS-task-isolation----------------------------------|  
|---->Each-task-gets-it's-own-MPU-config---------------|  
|---->Task-A-cannot-corrupt-Task-B's-memory------------|  
|------------------------------------------------------|  
|-Mark-peripheral-regions-as-non-cacheable-------------|  
|---->Critical-for-DMA+cache-coherency-----------------|  
|------------------------------------------------------|  
|-Execute-Never(XN)-regions----------------------------|  
|---->Prevent-executing-code-from-RAM------------------|  
|---->Blocks-certain-classes-of-exploits---------------|  
|------------------------------------------------------|  

## 3. DMA (Direct Memory Access (Memory Perspective))  

We touched on DMA + cache. Let's go deeper into how DMA interacts with the memory system.  

### What DMA Does  

Without DMA:  
UART receives byte -> interrupt -> CPU wakes up  
-> CPU reads UART -> DR -> CPU writes to buffer[i++]  
-> For 1MB of data: 1,000,000 interrupts, CPU busy entire time  

With DMA:  
UART receives byte -> DMA hardware reads UART->DR  
|-------------------> DMA writes directly to buffer  

-> CPU is FREE to do other work  
-> One interrupt when entire transfer is done  

### DMA Bus Arbitration  

DMA and CPU share the same buses. When both want the bus simultaneously, there's arbitration:  

AHB Bus:  

|--------|-------|---------------|   
|--CPU---|------>|--Bus--Matrix--|-------->|---SRAM---|  
|--------|-------|---(Arbiter)---|  
|--------|-------|---------------|  
|--DMA---|------>|---------------|  
|--------|-------|---------------|  

DMA can starve the CPU if it's doing continous transfers on the same bus as the CPU's SRAM accesses:  

// BAD: DMA transferring 1MB at max speed on AHB  
// CPU is trying to run from SRAM (.ramfunc)  
// -> CPU constantly loses bus arbitration -> huge slowdown  

// SOLUTION on STM32: Use DMA2 for transfers to/from  
// memory regions on separate AHB ports from CPU's SRAM access  
// Or: Enable DMA burst mode + FIFO to reduce bus occupancy  





