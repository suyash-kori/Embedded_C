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

The linker script provides all of this.It has the ".ld" extension (e.g. STM32F103CBTX_FLASH.ld).  

## Linker Script Structure, The Skeleton  

Every linker script has two main parts:  

/* 1. MEMORY -> describes physical memory regions */  
MEMORY {.....}  

/* 2. SECTIONS -> describes what goes where */  
SECTIONS {.....}  

That's it. Everything else is detail inside these two blocks.  

## Part 1, The MEMORY Block  

MEMORY  
{  
&emsp;/*-----Name--------Access------Origin--------Length---------*/  
&emsp;|------Flash-------(rx)---:ORIGIN=0x08000000,LENGTH=64K-------|  
&emsp;|------RAM--------(xrw)---:ORIGIN=0x20000000,LENGTH=20K-------|  
}  

#### Breaking it down:  

i) FLASH -> Name (you choose this, used in SECTIONS block)  

ii) (rx) -> Access attributes:  
&emsp; -> r = readable  
&emsp; -> x = executable (can run code from here)  
&emsp; -> w = writable  

iii) ORIGIN:0x08000000 -> Start address (from memory map)  

iv) LENGTH = 64K -> size of region  

#### For a chip with external SRAM:  

MEMORY  
{  

&emsp;FLASH (rx): ORIGIN = 0x08000000, LENGTH = 512K  
&emsp;RAM (xrw): ORIGIN = 0x20000000, LENGTH = 128K  
&emsp;EXT_SRAM (xrw): ORIGIN = 0x60000000, LENGTH = 8192K  
&emsp;BACKUP_RAM (xrw): ORIGIN = 0x40024000, LENGTH = 4K  

}  
You define as many regions as your hardware has. Each gets a name you reference later.

## Part 2, The SECTIONS Block  

This is the meat. Each entry says: "take this input section from .o files and place it at this location in memory."  

SECTIONS  
{
&emsp;/* - Section name in output - */  
&emsp;.isr_vector:  
&emsp;{
&emsp;&emsp;/* - What goes into it - */  
&emsp;&emsp;. = ALIGN(4); /* align to 4-byte boundary */  
&emsp;&emsp;KEEP(*(.isr_vector)) /* all .isr_vector from all .o */  
&emsp;&emsp;. = ALIGN(4);  
&emsp;} >FLASH  /* place this section in FLASH */  

&emsp;.text:  
&emsp;{
&emsp;&emsp;. = ALIGN(4);  
&emsp;&emsp;*(.text) /* .text from ALL object files */  
&emsp;&emsp;*(.text*) /* .text.functionname (GCC -ffunction-sections) */  
&emsp;&emsp;*(.glue_7) /* ARM/Thumb interworking glue */  
&emsp;&emsp;*(.glue_7t)  
&emsp;&emsp;. = ALIGN(4);  
&emsp;&emsp;_etext = .;  /* symbol marking end of .text */  
&emsp;} >FLASH  

&emsp;.rodata:  
&emsp;{
&emsp;&emsp;. = ALIGN(4);  
&emsp;&emsp;*(.rodata)  
&emsp;&emsp;*(.rodata*)  
&emsp;&emsp;. = ALIGN(4);  
&emsp;} >FLASH  

&emsp;/* ... more sections ... */  
} /* end SECTIONS */  

## The (.) (dot), Location Counter  

The dot is the current address as the linker walks through sections. It automatically advances as content is added.  

. = 0x08000000;  /* set current address to Flash start */  
"*(.isr_vector)" -> place vector table here (advances)  
/* . is now 0x08000000 + size_of_vector_table  */  
"*(.text)" -> place code right after vector table  
/* . advanced again  */  
. = ALIGN(4);  /* round up to next 4-byte boundary */  

## The Full Linker Script, Line by Line  

Here is a complete, production style linker for STM32F103C8. Let's read every single line:  

/* Entry point — tells debugger where program starts */  
ENTRY(Reset_Handler)  

/* Highest address of stack = top of RAM */  
_estack = ORIGIN(RAM) + LENGTH(RAM);  /* = 0x20000000 + 20K = 0x20005000 */  

/* Minimum stack and heap sizes — linker will error if not enough room */  
_Min_Heap_Size  = 0x200;   /* 512 bytes  */  
_Min_Stack_Size = 0x400;   /* 1024 bytes */  

MEMORY  
{  
&emsp;FLASH ( rx )  : ORIGIN = 0x08000000, LENGTH = 64K  
&emsp;RAM   ( xrw ) : ORIGIN = 0x20000000, LENGTH = 20K  
}  

SECTIONS  
{  
&emsp;/*  
&emsp;* ① VECTOR TABLE  
&emsp;* Must be first in Flash at 0x08000000  
&emsp;* KEEP() prevents linker from discarding it  
&emsp;* (it's never "called" so linker might remove it as dead code)  
&emsp;*/  
&emsp;.isr_vector :  
&emsp;{  
&emsp;&emsp;. = ALIGN(4);  
&emsp;&emsp;KEEP(*(.isr_vector))  
&emsp;&emsp;. = ALIGN(4);  
&emsp;} >FLASH  

&emsp;/*  
&emsp;* ② CODE + CONSTANTS  
&emsp;* All compiled functions and read-only data  
&emsp;*/  
&emsp;.text :  
&emsp;{  
&emsp;&emsp;. = ALIGN(4);  
&emsp;&emsp;*(.text) /* main code sections */  
&emsp;&emsp;*(.text*) /* e.g. .text.main, .text.foo  */  

&emsp;&emsp;/* ARM Thumb/ARM interworking support */  
&emsp;&emsp;*(.glue_7)  
&emsp;&emsp;*(.glue_7t)  
&emsp;&emsp;*(.eh_frame)  

&emsp;&emsp;/* Constructor/destructor tables (for C++ and __attribute__((constructor))) */  
&emsp;&emsp;KEEP(*(.init))  
&emsp;&emsp;KEEP(*(.fini))  

&emsp;&emsp;. = ALIGN(4);  
&emsp;&emsp;_etext = .;  /* symbol: end of text   */  
&emsp;} >FLASH  

&emsp;/*  
&emsp;* ③ CONSTRUCTOR/DESTRUCTOR ARRAYS  
&emsp;* Called by __libc_init_array() before main()  
&emsp;*/  
&emsp;.preinit_array :  
&emsp;{  
&emsp;&emsp;PROVIDE_HIDDEN(__preinit_array_start = .);  
&emsp;&emsp;KEEP(*(.preinit_array*))  
&emsp;&emsp;PROVIDE_HIDDEN(__preinit_array_end = .);  
&emsp;} >FLASH  

&emsp;.init_array :  
&emsp;{  
&emsp;&emsp;PROVIDE_HIDDEN(__init_array_start = .);  
&emsp;&emsp;KEEP(*(SORT(.init_array.*)))  
&emsp;&emsp;KEEP(*(.init_array*))  
&emsp;&emsp;PROVIDE_HIDDEN(__init_array_end = .);  
&emsp;} >FLASH  

&emsp;.fini_array :  
&emsp;{  
&emsp;&emsp;PROVIDE_HIDDEN(__fini_array_start = .);  
&emsp;&emsp;KEEP(*(SORT(.fini_array.*)))  
&emsp;&emsp;KEEP(*(.fini_array*))  
&emsp;&emsp;PROVIDE_HIDDEN(__fini_array_end = .);  
&emsp;} >FLASH  

&emsp;/*  
&emsp;* ④ READ-ONLY DATA  
&emsp;* const variables, string literals  
&emsp;*/  
&emsp;.rodata :  
&emsp;{  
&emsp;&emsp;. = ALIGN(4);  
&emsp;&emsp;*(.rodata)  
&emsp;&emsp;*(.rodata*)  
&emsp;&emsp;. = ALIGN(4);  
&emsp;} >FLASH  

&emsp;/*  
&emsp;* ⑤ ARM EXCEPTION TABLES  
&emsp;* Used for C++ exceptions and stack unwinding  
&emsp;* On bare-metal, usually empty — but must be present  
&emsp;*/  
&emsp;.ARM.extab :  
&emsp;{  
&emsp;&emsp;*(.ARM.extab* .gnu.linkonce.armextab.*)  
&emsp;} >FLASH  

&emsp;.ARM :  
&emsp;{  
&emsp;&emsp;__exidx_start = .;  
&emsp;&emsp;*(.ARM.exidx*)  
&emsp;&emsp;__exidx_end = .;  
&emsp;} >FLASH  

&emsp;/*  
&emsp;* ⑥ .data SECTION — THE KEY ONE  
&emsp;*  
&emsp;* AT>FLASH means:  
&emsp;*   VMA (runtime address) = RAM  (where CPU accesses it)  
&emsp;*   LMA (load address)    = FLASH (where init values are stored)  
&emsp;*
&emsp;* _sidata = start of init values IN FLASH  
&emsp;* _sdata  = start of .data IN RAM  
&emsp;* _edata  = end of .data IN RAM  
&emsp;*/  
&emsp;_sidata = LOADADDR(.data);   /* LMA: Flash address of init values */  

&emsp;.data :  
&emsp;{  
&emsp;&emsp;. = ALIGN(4);  
&emsp;&emsp;_sdata = .;  /* VMA: RAM start of .data  */  

&emsp;&emsp;*(.data)  
&emsp;&emsp;*(.data*)  

&emsp;&emsp;. = ALIGN(4);  
&emsp;&emsp;_edata = .; /* VMA: RAM end of .data  */  

} >RAM AT>FLASH/* VMA in RAM(accessed from RAM at runtime),LMA in FLASH(stored in Flash)*/  
&emsp;* ⑦ .bss SECTION   
&emsp;* No AT>FLASH needed — no init values to store  
&emsp;* Linker just records the start/end addresses  
&emsp;* Startup code zeros this range  
&emsp;*/  
&emsp;.bss :  
&emsp;{  
&emsp;&emsp;. = ALIGN(4);  
&emsp;&emsp;_sbss = .;   /* start of .bss  */  
&emsp;&emsp;__bss_start__ = _sbss;  

&emsp;&emsp;*(.bss)  
&emsp;&emsp;*(.bss*)  
&emsp;&emsp;*(COMMON)  /* uninitialized C commons */  

&emsp;&emsp;. = ALIGN(4);  
&emsp;&emsp;_ebss = .; /* end of .bss */  
&emsp;&emsp;__bss_end__ = _ebss;  

&emsp;} >RAM  

&emsp;/*  
&emsp;* ⑧ USER STACK AND HEAP  
&emsp;* Linker just verifies enough RAM remains  
&emsp;* Does NOT zero or initialize them  
&emsp;*/  
&emsp;._user_heap_stack :  
&emsp;{  
&emsp;&emsp;. = ALIGN(8);  
&emsp;&emsp;PROVIDE(end = .); /* 'end' = start of heap (used by newlib malloc) */  
&emsp;&emsp;PROVIDE(_end = .);  
&emsp;&emsp;. = . + _Min_Heap_Size;  /* reserve minimum heap space */  
&emsp;&emsp;. = . + _Min_Stack_Size; /* reserve minimum stack space */  
&emsp;&emsp;. = ALIGN(8);  
&emsp;} >RAM  

&emsp;/*  
&emsp;*⑨ DISCARD unused sections  
&emsp;*Removes C++ exception handling tables (not needed bare-metal)  
&emsp;*/  
&emsp;/DISCARD/ :  
&emsp;{  
&emsp;&emsp;libc.a ( * )  
&emsp;&emsp;libm.a ( * )  
&emsp;&emsp;libgcc.a ( * )  
&emsp;}  

&emsp;.ARM.attributes 0 :  
&emsp;{  
&emsp;&emsp;*(.ARM.attributes)  
&emsp;}  
}  

## The AT>FLASH Magic, Visualized  

This is the single most confusing part of linker scripts. Let's make it crystal clear:  

FLASH contents (what's burned to chip):  
|------------------------------------------------------|  
|--0x08000000:-Vector-Table----------------------------|  
|--0x0800010C:-.text(all-your-code)--------------------|  
|--0x08003A00:-.rodata(const-data)---------------------|  
|--0x08003C00:-.data-INIT-VALUES-<-_sidata-points-here-|  
|-----------[100]-[42]-[1,2,3,4]-<-actual-bytes--------|  
|------------------------------------------------------|  
|---------------------|--------------------------------|  
|---------------------|startup-code-copies-this--------|  
|---------------------|to-RAM-before-main()------------|  
|---------------------V--------------------------------|  
RAM contents (after startup, before main()):  
|------------------------------------------------------|  
|--0x20000000:-.data<-_sdata---------------------------|  
|--------------counter=100<-copied-from-Flash----------|  
|--------------state=42<-copied-from-Flash-------------|  
|--------------buf={1,2,3,4}<-copied-from-Flash--------|  
|--_edata----------------------------------------------|  
|--------------.bss(all-zeros)-------------------------|  
|--_ebss-----------------------------------------------|  
|--------------Heap(uninitialized,managed-by-malloc)---|  
|--------------...-------------------------------------|  
|--0x20004FFF:Stack-top<-_estack-----------------------|  
|------------------------------------------------------|  

The linker generates TWO addresses for ".data":  

- LMA (_sidata) = where the bytes physically sit in the Flash binary  
- VMA (_sdata) = where the CPU will access them at runtime  


