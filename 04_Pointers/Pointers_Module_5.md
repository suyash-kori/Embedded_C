# Dynamic Memory & Memory Management  

First, understand what the heap actually is  

Before touching "malloc", you need to know where heap memory comes from physically.  

The heap is a region of SRAM sitting between the BSS segment and the stack. On a bare-metal microcontroller, the startup code (usually in startup_stm32xxx.s or similiar) sets up the heap boundaries. The C runtime's "malloc" implementation manages this region, tracking which parts are free, which are allocated, and handling requests for memory.  

SRAM Layout (typical Cortex-M):  

|------------------------------------| <- top of SRAM (e.g  0x20010000)  
/&emsp;&emsp;-STACK&emsp;&emsp;&emsp;/  
/&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;/  
/&emsp;&emsp;(free space)&emsp;&emsp;/  grows downward
/&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;/  
/&emsp;&emsp;-HEAP-&emsp;&emsp;&emsp;/  grows upward  
|------------------------------------| <- heap start  
/&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;/
/&emsp;&emsp;-BSS--&emsp;&emsp;&emsp;/  uninitialized globals
/&emsp; zeroed at startup&emsp;&emsp;/  
/&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;/  
|-------------------------------------|  
/&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;/  
/&emsp;&emsp;-DATA-&emsp;&emsp;&emsp;/  initialized globals
/&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;/  
|-------------------------------------| <- 0x2000000 (start of SRAM)  

The stack grows down, the heap grows up. If they meet, heap exhausion or stack overflow, your program is in undefined territory. On a desktop OS, the OS catches this. On bare-metal embedded, nothing catches it. The stack silently overwrites heap data or vice versa, and you get mysterous corruption that is extremely hard to debug.  

## "malloc"- how it works internally  

"malloc(n)" asks the heap manager for "n" bytes of usable memory. The heap manager maintains a free list, a data structure tracking available blocks. When you call malloc:  

1] The manager searches the free list for a block large enough  
2] It splits the block if it's larger than needed  
3] It adds a small header before the returned pointer containing metadata (size, whether allocated, pointer to next block)  
4] Returns a pointer to the usable portion.  

void *p = malloc(16);  

/* What actually exists in memory: */  
/* [header: 8-16 bytes][your 16 bytes] */  
/*&emsp;&emsp;&emsp;&emsp;^p points here */  

EXPLAINATION:-  

1. Free list before malloc(16)  

/&emsp;Block A&emsp;/-/&emsp;Block B&emsp;/-/&emsp;Block C&emsp;/;  
