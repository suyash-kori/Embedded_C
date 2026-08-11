# Linker Scripts, Startup Code and the Boot Sequence  

## Why you MUST understand linker scripts  

Most embedded engineers treat the linker script as a black box, copy it from a template and never touch it. That's fine until:  

- You need to place code in a specific memory region.  
- You're adding external RAM or Flash.  
- You have a bootloader + application split  
- You're debugging a HardFault caused by wrong memory placement  
- You want to optimize Flash/RAM usage  

The linker script is the contract between your code and the hardware memory map. Let's read and write one from scratch.  

## What is a Linker Script?  

The linker(ld) takes all your ".o" object files and combines + places them into a final binary. Without a linker script it has no idea:  

- Where Flash starts or how big it is  
- Where SRAM starts  
- Which section goes where  
- What symbols like "_estack" or "_sidata" mean  

The 
