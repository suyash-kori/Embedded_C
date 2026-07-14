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

