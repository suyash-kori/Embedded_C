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
|---------------------------------------------------|  
|SYSCLK--------------|--Wait-States(LATENCY)------|  
|0-24-MHz------------|--0(no-wait)----------------|  
|24-48MHz------------|--1-wait-state--------------|  
|48-72MHz------------|--2-wait-states-------------|  

At 72MHz with 2 wait states:  

Clock:--|^^|__|^^|__|^^|__|^^|__|^^|__|  
Flash:--[-request-][data-out]    
CPU:----[-fetch-][WAIT][WAIT][execute]  
|-------------------^-----^------------------------|    
|-------------wasting-2-cycles-waiting-for-Flash!--|  













