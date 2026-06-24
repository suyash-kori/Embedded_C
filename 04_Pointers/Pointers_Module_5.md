# Dynamic Memory & Memory Management  

First, understand what the heap actually is  

Before touching "malloc", you need to know where heap memory comes from physically.  

The heap is a region of SRAM sitting between the BSS segment and the stack. On a bare-metal microcontroller, the startup code (usually in startup_stm32xxx.s or similiar) sets up the heap boundaries. The C runtime's "malloc" implementation manages this region, tracking which parts are free, which are allocated, and handling requests for memory.  

SRAM Layout (typical Cortex-M):  

<------------------------------------> <- top of SRAM (e.g  0x20010000)  
|-------------STACK------------------|  
|------------------------------------|  
|-----------(free space)-------------|  grows downward
|------------------------------------|    
|-------------HEAP-------------------|  grows upward  
<------------------------------------> <- heap start  
|------------------------------------|
|-------------BSS--------------------|  uninitialized globals
|-------zeroed at startup------------|  
|------------------------------------|  
<------------------------------------> 
|------------------------------------|  
|-------------DATA-------------------|  initialized globals
|------------------------------------|
<------------------------------------> <- 0x2000000 (start of SRAM)  

The stack grows down, the heap grows up. If they meet, heap exhausion or stack overflow, your program is in undefined territory. On a desktop OS, the OS catches this. On bare-metal embedded, nothing catches it. The stack silently overwrites heap data or vice versa, and you get mysterous corruption that is extremely hard to debug.  

## "malloc"- how it works internally  

"malloc(n)" asks the heap manager for "n" bytes of usable memory. The heap manager maintains a free list, a data structure tracking available blocks. When you call malloc:  

1] The manager searches the free list for a block large enough  
2] It splits the block if it's larger than needed  
3] It adds a small header before the returned pointer containing metadata (size, whether allocated, pointer to next block)  
4] Returns a pointer to the usable portion.  

void *p = malloc(16);  

--What actually exists in memory:   
--[header: 8-16 bytes][your 16 bytes]
----------------------^p points here

Image is attached with name "malloc_allocation_diagram":-  

1. The free list is just a to-do list for the heap manager.  
Think of it like a landlord's notebook: "I have these empty rooms available". When you call "mlloc(16)", the manager flips through that notebook looking for a room big enough.  

2. It splits the block. If Block A has 64 bytes free and you only need 16, it carves out 16 for you and puts the leftover 40 bytes back into the free list as a new smaller block. No waste.  

3. A secret header is written just before your pointer. You never see it, but it's always there, storing the size, whether the blocks is in use, and a link to the next free block. This is the magic behind free(p), it just looks 8-16 bytes before "p" to find the size. You never have to tell it.  

4. "p" points to your bytes, not the header. The header lives in memory right before "p". This is why:  

- Writing before "p" (underflow) -> you corrupt the header->the heap manager gets confused and crashes later in mysterious ways  

- Writing past "p+16" (overflow) -> you corrupt the next block's header-> same chaotic result.  

The tricky part about heep corruption is that crash rarely happens at the bad write, it happens later, when the heap manager tries to use the metadata you silently broke. That's what makes these bugs hard to debug.  

This header is why you never need to pass the size to free(), the size is stored in the header just before the pointer. It's also why writing before "p"(buffer underflow) or past (p+16) (buffer overflow) corrupts heap metadata, you'reoverwriting these hidden headers.  

## The four heap functions  

### "malloc", allocate uninitialized memory  

void *malloc(size_t size);  

Allocates "size" bytes. Contents are uninitialized, whatever was in that memory before is still there. Always check for NULL return.  

int *arr = malloc(10 * sizeof(int));  
if(arr == NULL)  
{  
&emsp;/* allocation failed, handle ir */  
&emsp;return -1;  
}  
/* arr contents are garbage, must initialize before use */  
arr[0] = 42;  /* OK, now it's initialized */  

A common bug:  

int *p = malloc(sizeof(int));  
if (*p == 0) {...}  /* BUG, p is uninitialized, reading garbage */  

### "calloc", allocate zeroed memory  

void *calloc(size_t count, size_t size);  

Allocates "count * size" bytes and zeroes the entire region. Slightly slower than malloc because of the zeroing, but elimates the uninitialized memory bug.  

int *arr = calloc(10, sizeof(int));  
if (arr == NULL) { return -1; }  
/* arr is guaranteed to be all zeros */  
if (arr[0] == 0){...} /* safe, calloc zeroed it */  

Note the argument order: count first, then size. The reason for two arguments instead of one is historical, it allows the implementation to check for integer overflow in (count * size) before allocating.  

### "realloc", resize an allocation  

void *realloc(void *ptr, size_t new_size);  

Resizes a previously allocated block. May move it to a new location if the current block can't expand in place. Returns the new pointer, which may or may not be the same as the old one.  

The classic mistake:  

/* WRONG. if realloc fails, original pointer is lost */  

ptr = realloc(ptr, new_size);  
if(ptr == NULL)  
{
&emsp;/*original memory is now leaked, ptr is NULL, old address is gone 
}  

/* CORRECT, use a temporary */  
void *temp = realloc(ptr, new_size);  
if (temp == NULL)  
{  

&emsp;/* allocation failed, original ptr is still valid */  
&emsp;free(ptr);  /* or keep using it */  
&emsp;return -1;  

}  
ptr = temp;  /* safe to update now */  

### "free", release memory back to heap  

void free(void *ptr);  

Returns the block to the free list. After calling "free(ptr)", the memory no longer belongs to you. Using it afterward is undefined behaviour.  

free(ptr);  
ptr = NULL; // always NULL after free, prevents use-after-free  

Setting to NULL after free is a discipline, not a language requirement. It means a subsequent accidental dereference gives a NULL dereference (crash immediately, easy to debug) rather than a use-after-free (silent corruption, incredibly hard to debug).  

## The classic heap bugs, memorize these  

### Bug 1: Memory leak  

Allocated memory that is never freed. The program slowly consumes all available memory.  

void process_data(void)  
{  

&emsp;uint8_t *buf = malloc(256);  
&emsp;if (buf == NULL) return;  

    /* ......do processing.......*/  

&emsp;if (error_condition)  
&emsp;{  

&emsp;&emsp;return;  /* BUG, buf is never freed on this path */  

&emsp;}  

&emsp;free(buf);  /* only reached on success path */  

}  

On embedded systems, this is fatal. There's no OS to reclaim memory when the function exits. After enough calls, "malloc" returns NULL everywhere and the system grinds to a halt, often hours or days after deployment. This is a common cause of embedded systems needing periodic reboots.  

FIX:---  ensure every exit path frees the memory  

void process_data(void)  
{  

&emsp;uint8_t *buf = malloc(256);  
&emsp;if (buf == NULL) return;  

&emsp;/* .......do processing......... */  

&emsp;if (error_condition)  
&emsp;{  

&emsp;&emsp;free(buf); /* free before every return */  
&emsp;&emsp;return;  

&emsp;}  

&emsp;free(buf);  

}  

### Bug 2: Use after free (dangling pointer)  

Accessing memory after it has been freed. The memory may have been reallocated to something else.  

int *p = malloc(sizeof(int));  
*p = 42;  
free(p);  

printf("%d\n", *p);  /* BUG, p is dangling, memory was freed */  
*p = 99;  /* BUG, corrupting memory that may belong to someone else */  

This is undefined behaviour. It may appear to work (if the memory hasn't been reallocated yet), it may crash, or it may silently corrupt a completely unrelated data structure. The third outcome is the worst, you see corruption somewhere else and spend days looking in the wrong place.  

free(p);  
p = NULL;  /* prevent dangling, next use crashes immediately */  


### Bug 3: Double free  

Calling "free" twice on the same pointer. The second "free" corrupts the heap's free list metadata.  

int *p = malloc(sizeof(int));  
free(p);  
free(p);  /* BUG, heap corruption */  

The heap manager's free list now has a corrupted block, a block that appears free but is already on the free list. The next "malloc" may return this block to two different callers simultaneously. Results range from immediate crash to security vulnerability (this is a class of explots in desktop systems).  

Fix: NULL after free (free(NULL))"is defined to do nothing.  

free(p);  
p = NULL;  
free(p);  /* safe, free(NULL) is a no-op */  


### Bug 4: Buffer overflow (heap)  

Writing past the end of an allocated block. Corrupts adjacent heap metadata or adjacent allocations.  

int *arr = malloc(5*sizeof(int));  /* space for 5 ints  
for (int i = 0; i <= 5; i++)  /* BUG, writes index 0...5, that's 6 elements  
{  
&emsp;arr[i] = i;  /* arr[5] is one past the end */  
}  

arr[5] overwrites the header of the next heap block. The heap manager now has corrupt metadata. The bug may not manifest until the next "malloc" or "free" call, making it very hard to associate with the actual cause.  

### Bug 5: Forgetting to check NULL  

"malloc" returns NULL when it can't fulfill the request. Dereferencing NULL causes a crash.  

int *p = malloc(1024 * 1024);  /* requesting 1 MB on a 64 KB SRAM system 
*p = 42; /* crash, p is NULL, malloc failed silently */  

Always check. Every single time. In embedded, allocation failures are not threoretical, they happen regularly given the small heap sizes.  

## Heap fragmentation, why embedded avoids dynamic allocation  

Even if you free everything correctly, the heap can become fragmented. Imagine this sequence:  

Allocate A (100 bytes)  
Allocate B (200 bytes)  
Allocate C (100 bytes)  
Free A  
Free C  
Allocate D (150 bytes) <- FAILS even though 200 bytes are free total  

After freeing A and C, you have two free blocks of 100 bytes each, 200 bytes free total. But they are not contigous. A request for 150 bytes fails because no single block is large enough. This is fragmentation.  

Memory state after freeing A and C:  
[FREE:100][B:200][FREE:100]  

Request for 150 bytes fails, no contiguous block of 150 exists.  

On a desktop system with gigabytes of RAM and virtual memory, fragmentation is manageable. On a microcontroller with 64 KB of SRAM, it can make the system unusable within hours. And fragmentation is non-deterministic, the behaviour depends on the exact sequence of allocations and frees at runtime, making it nearly impossible to test exhaustively.  

## Why embedded firmare avoids "malloc"  

Beyond fragmentation, there are several reasons professional embedded firmware avoids dynamic allocation:  

Non-deterministic timing:-  
"malloc" runtime varies depending on the heap state. In a real-time system, you need deterministic response times. A "malloc" call might take 1 microsecond or 1 millisecond depending on how fragmented the heap is. You cannot guarantee timing.  

No safety net:-  
On Linux, if you access freed memory, the OS usually catches it with a segfault. On bare metal, there's nothing. Heap corruption causes silent data corruption that manifests as bizzare behaviour far from the actual bug.  

Stack/heap collision:-  
With no MMU (common on Cortex-M0/M0+), there's nothing preventing the stack and heap from colliding. the system silently corrupts both.  

MISRA-C compliance:-  
MISRA-C(the automotive/safety critical C standard) prohibits dynamic memory allocation entirely after initialization. Rule 21.3: "The memory allocation and deallocation functions of <stdlib.h> shall not be used."  

## What embedded firmware does instead  

Static allocation  

/* Fixed buffer, size known at complile time */  
static uint8_t uart_rx_buffer[256];  
static uint8_t uart_tx_buffer[256];  

/* Fixed pool of structs */  
static sensor_reading_t reading_pool[MAX_READINGS];  
static uint8_t reading_pool_used[MAX_READINGS]; /* usage bitmap */  

Size is known at compile time. The linker places it in BSS or Data segment. No runtime, no fragmentation, fully deterministic.  

## Memory pool allocator  

A fixed block allocator, allocates same-size blocks from a pre-allocated pool. Constant time, no fragmentation, no heap involvement.  

#define POOL_BLOCK_SIZE 64  
#define POOL_BLOCK_COUNT 16  

typedef struct  
{  

&emsp;uint8_t blocks[POOL_BLOCK_COUNT][POOL_BLOCK_SIZE];  
&emsp;uint16_t free_bitmap;  /* bit set = block is free */  

} mem_pool_t;  
/* all 16 blocks free initially */  
static mem_pool_t pool = {.free_bitmap = 0xFFFF};  

void *pool_alloc(mem_pool_t *p)  
{  
&emsp;for (int i = 0; i < POOL_BLOCK_COUNT; i++)  
&emsp;{  

&emsp;&emsp;if (p->free_bitmap & (1 << i))  
&emsp;&emsp;{  

&emsp;&emsp;&emsp;p->free_bitmap &= ~(1 << i);  /* mark as used */  
&emsp;&emsp;&emsp;return p->blocks[i];  

&emsp;&emsp;}  

&emsp;}  
&emsp;return NULL; /* pool exhausted */  

}  

void pool_free(mem_pool_t *p, void *block)  
{  

&emsp;int i = ((uint8_t*)block - p->blocks[0]) / POOL_BLOCK_SIZE;  
&emsp;p->free_bitmap |= (1 << i); /* mark as free */  

}  

This gives you the flexibility of dynamic allocation with the predictability of static allocation. No fragmentation, every block is the same size. Constant time alloc and free. This pattern is used in FreeRTOS message queues, 1wIP packet buffers, and many embedded middleware libraries.  

### EXPLANATION:-  

The 2D Array  

uint8_t blocks[POOL_BLOCK_COUNT][POOL_BLOCK_SIZE];  
//---------------[16]---------------[64]----------  

Think of it as 16 rows, 64 columns, a grid of bytes:  

blocks[0] -> [ 64 bytes ] <- Block 0  
blocks[1] -> [ 64 bytes ] <- Block 1  
blocks[2] -> [ 64 bytes ] <- Block 2  
...
block[15] -> [ 64 bytes ] <- Block 15  

Each row (blocks[i]) is one allocatable chunk. When you call "pool_alloc()", it hands you a pointer to one of these rows.  

#### - The Bitmap, (uint16_t free_bitmap)  

This is the clever part. Instead of a linked list or array of flags, it tracks all 16 blocks in a single 16-bit integer, one bit per block.  

free_bitmap = 1111 1111 1111 1111 (0xFFFF) -> all 16 blocks free  
free_bitmap = 1111 1111 1111 1110 -> block 0 is in use  
free_bitmap = 0000 0000 0000 0000 -> pool exhausted  

Bit i = 1 means block "i" is free. Bit "i" = 0 means it's in use.  

#### - pool_alloc, line by line  

if (p->free_bitmap & (1 << i) ) // is bit i set?  (is block i free)  

1 << i shifts the number 1 left by "i" positions, creating a mask with only bit "i" set. ANDing with the bitmap checks just that bit.  

p->free_bitmap &= ~(1 << i);  // Clear bit i (mark block as used)  

~(1 << i) flips all bits, soallbits are 1 except bit "i". ANDing clears just that one bit.  

return p->blocks[i];  // return pointer to that row  

#### - pool_free, the pointer arithmetic  

int i = ((uint8_t*)block - p->blocks[0]) / POOL_BLOCK_SIZE;  

This reverse-calculates which block index a pointer belongs to:  

block -> address of e.g. blocks[3] = base + (3 * 64) = base + 192  
p->blocks[0] -> base address  
difference -> 192 bytes  
divide by 64 -> index 3  

Then,  

p -> free_bitmap |= (1 << i);  // Set bit i (mark block as free)  

ORing sets that bit back to 1.  

The full picture:-  

pool_alloc()  
-> scan bitmap for a 1 bit
-> clear that bit
-> return pointer to blocks[i]  

pool_free()  
-> calc index from pointer  
-> set that bit back to 1  
-> done, 0(1)  

The O(1) claim on free is true because it's pure arithmetic, no searching. The alloc is technically O(n) in the worst case (scans bits left to right), but with only 16 blocks and a single integer, it's effectively constant in practice. You could make it truly O(1) using a __builtin_ctz() (count trailing zeroes) to find the first free bit instantly.  

Why don't we need to empty the main block of memory, i.e blocks[POOL_BLOCK_COUNT][POOL_BLOCK_SIZE];  

#### - The bitmap is the source of truth  
The allocator only cares about the bitmap, not the actual bytes inside the block. When you call "pool_alloc()":  

if (p->free_bitmap & (1 << i))  // only checks the BIT, never looks at block contents  

&emsp;return p->blocks[i]  // just hands you the pointer  

It never inspects what's inside blocks[i]. So leftover data from the previous user sitting in those 64 bytes doesn't affect the allocator's logic at all.  

The returned memory is "dirty", and that's okay  

Before free: blocks[3] = [ 0x12, 0xAB, 0x00, ....] <- old data still there  

After free: blocks[3] = [ 0x12, 0xAB, 0x00, ....] <- exactly the same, unchanged  

After realloc: blocks[3] = [ 0x12, 0xAB, 0x00, ...] <- next user gets this dirty block  

The next caller who gets blocks[3] is responsible for writing their own data into it before reading. That's standard C, just like "malloc()" returns uninitialized memory.  

#### When you would want to zero it out  

There are specific situations where you'd explicitly clear the block:  

void pool_free(mem_pool_t *p, void *block)  
{  

&emsp;int i = ((uint8_t*)block - p->blocks[0]) / POOL_BLOCK_SIZE;  
&emsp;memset(p->blocks[i], 0, POOL_BLOCK_SIZE);  // optional scubbing  
&emsp;p->free_bitmap |= (1 << i);  

}  

You'd do this when:  

- Security - the block held a password, key, or sensitive data and you don't want the next user to see it.  

- Debugging - fill with 0xDEAD so use-after-free bugs are immediately obvious  

- Protocol requirement - some middleware expects zeroed buffers (like 1wIP in certain configs)  

## Stack vs heap, the complete comparison  

Stack:-  

1) Allocation -> Automatic, compiler manages  
2) Deallocation -> Automatic,on function return  
3) Speed -> Extremely fast, just move stack pointer  
4) Size -> Small, fixed (typically 1-8KB embedded)  
5) Fragmentation -> None  
6) Thread safety -> Each thread has own stack  
7) Deterministic -> Yes  
8) Overflow detection -> Rarely (stack sentinel pattern)  

Heap:-  

1) Allocation -> Manual, programmer manages  
2) Deallocation -> Manual, must call "free()"  
3) Speed -> Slower, search free list  
4) Size -> Larger, up to availabe SRAM  
5) Fragmentation -> Yes, over time  
6) Thread safety -> Shared, needs locking  
7) Deterministic -> No  
8) Overflow detection -> Rarely on bare metal  

## A complete safe usage example  

#include <stdlib.h>  
#include <string.h>  
#include <stdint.h>  

typedef struct  
{  

&emsp;uint8_t *data;  
&emsp;size_t len;  
&emsp;size_t capacity;  

} buffer_t;  

/* Initialize a dynamic buffer */  
int buffer_init(buffer_t *buf, size_t initial_capacity)  
{  

&emsp;buf->data = malloc(inital_capacity);  
&emsp;if (buf->data == NULL)  
&emsp;{
&emsp;&emsp;return -1;  /* allocation failed */  
&emsp;}  
&emsp;buf->len = 0;  
&emsp;buf->capacity = initial_capacity;  
&emsp;return 0;  

}  

/* Append data to buffer, growing if necessary */  
int buffer_append(buffer_t *buf, const uint8_t *src, size_t len)  
{  

&emsp;if (buf->len + len > buf->capacity)  
&emsp;{  

&emsp;&emsp;size_t new_cap = buf->capacity*2;  
&emsp;&emsp;uint8_t *new_data = realloc(buf->data, new_cap); /*use temp!*/  
&emsp;&emsp;if (new_data == NULL)  
&emsp;&emsp;{  
&emsp;&emsp;&emsp;return -1; /* original buf->data still valid */              

&emsp;&emsp;}  
&emsp;&emsp;buf->data = new_data;  
&emsp;&emsp;buf->capacity = new_cap;  

&emsp;}  

&emsp;memcpy(buf->data + buf->len, src, len);  
&emsp;buf->len += len;  
&emsp;return 0;  

}  

/* Free the buffer */  
void buffer_free(buffer_t *buf)  
{  
&emsp;free(buf->data);  
&emsp;buf->data = NULL; /* prevent dangling */  
&emsp;buf->len = 0;  
&emsp;buf->capacity  
}  

/* Usage */  
int main(void)  
{
&emsp;buffer_t bug;  

&emsp;if(buffer_init(&buf, 64) != 0)  
&emsp;{  

&emsp;&emsp;return -1;  

&emsp;}  

&emsp;uint8_t data[] = {0x01, 0x02, 0x03};  
&emsp;buffer_append(&buf, data, sizeof(data));  

&emsp;/* use buf data....*/  
&emsp;buffer_free(&buf);  
&emsp;return 0;  

}  

The three core functions:-  

1]  "buffer_init", allocates the initial chunk of memory and sets len = 0. Returns "-1" on failure.  

2]  "buffer_append". this is the intersting one:  
- Checks if the new data fits: "buf->len + len > bug->capacity"  
- If not, doubles the capacity (new_cap = capacity * 2) and calls realloc  
- Uses a temp pointer (new_data) for the realloc result, if it returns "NULL", the original "buf->data" is still valid and usable.  
- Copies the new bytes in with "memcpy"  

3] "buffer_free", frees the memory and nulls out the pointer to prevent a dangling pointer bug (using memory after it's freed).  


The growth strategy  

Doubling capacity on each resize is the classic approach, it gives amortized 0(1) appends. However, there are two bugs worth noting:-  

- "new_cap" doesn't account for the requested "len", if "len" is larger than "capacity", doubling still won't be enough.  

-  "new_cap" isn't checked for overflow, if capacity is already huge, "capacity * 2" can wrap around  

A safer resize would be:  

size_t new_cap = (buf->capacity * 2 > buf->len + len)? buf->capacity * 2: buf->len + len;  

Memory layout at runtime  

buf.data--->[0x01 | 0x02 | 0x03 |.............]  
|----------|<-----len = 3------>|<---stack--->|  
|----------|<--------capacity = 64----------->|





