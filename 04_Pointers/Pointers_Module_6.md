# Pointers to Structs, Arrays of Pointers & Advanced Embedded Patterns  

## Pointers to structs, the foundation of everything in embedded C  

Almost every non-trival C program uses structs. Almost every non-trival use of structs involves pointers to them. Understanding exactly how pointer to struct works at the memory level is essential.  


typedef struct  
{  
&emsp;uint8_t id;  
&emsp;uint16_t voltage;  
&emsp;int16_t temperature;  
&emsp;uint8_t status;  

} sensor_t;  

sensor_t s;  /* the actual struct, lives on stack or in static memory */  

sensor_t *p = &s; /* pointer to the struct, holds address of s */  

In memory, the struct's field are laid out sequentially (with possible padding):  

Address------Field-----------Size  

0x2000---------id----------1 byte  
0x2001-------[padding]-----1 byte <- compiler adds this for allignement  
0x2002-------volatage------2 bytes  
0x2004-------temperature---2 bytes  
0x2006-------status--------1 bytes  
0x2007-------[padding]-----1 bytes<- to make total size a multiple os 2  

"p" holds "0x2000". p->voltage means: go to 0x2000, add the offset of voltage (which is 2), read 2 byes from 0x2002. The compiler computes all these offsets at compile time, accessing struct members through a pointer is just pointer arithmetic with a named offset.  

## Arrow operator vs dot operator  

sensor_t s;  
sensor_t *p = &s;  

/* Dot operator, used when you have the struct directly */  
s.voltage = 3300;  

/* Arrow operator, used when you have a pointer to the struct */  
p->voltage = 3300;  

/* Arrow is exactly shorthand for: */  
(*p).voltage = 3000;  /* dereference p, then access field */  

The arrow "->" is just syntactic sugar for dereference then dot. The compiler generates identical code for "p->voltage" and "(*p).voltage". Always use "->" with pointers, it's cleaner and universally understood.  

A common mistake:  

sensor_t *p;  /* pointer, not initialized! */  
p.voltage = 3300;  /* COMPILE ERROR, p is a pointer, use "->" */  
p->voltage = 3300;  /* still wrong, p is uninitialized, dangling */  

## Struct alignment and padding, critical for embedded  

The compiler adds padding bytes between struct fields to ensure each field is aligned to it's natural alignment boundary. On most 32-bit ARM systems:  

- "uint8_t" -> 1 byte alignment  
- "uint16_t" -> 2 byte alignment (must be at even address)  
- "uint32_t" -> 4 byte alignment (must be at address divisible by 4)  

/* Poorly ordered, wastes memory due to padding */  
typedef struct {  
&emsp;uint8_t a;  /* 1 byte at offset 0 */  
&emsp;&emsp;&emsp;/* 3 bytes padding */  
&emsp;uint32_t b; /* 4 bytes at offset 4 */  
&emsp;uint8_t c; /* 1 byte at offset 8 */  
&emsp;&emsp;&emsp;/* 3 bytes padding */  
}  bad_t;  /* sizeof = 12 bytes - 6 wasted  */  

/* Well ordered, fields from largest to smallest */  
typedef struct {  
&emsp;uint32_t b; /* 4 bytes at offset 0 */  
&emsp;uint8_t a; /* 1 byte at offset 4 */  
&emsp;uint8_t c; /* 1 byte at offset 5 */  
&emsp;&emsp;&emsp;/* 2 bytes padding */  
} good_t;  /* sizeof = 8 bytes, only 2 wasted  

In embedded with tight SRAM, struct layout matters. Order fields from largest to smallest type to minimize padding.  

"__attribute__((packed))" -> removes all padding:  

typedef struct __attribute__((packed))  
{
&emsp;uint8_t a;  
&emsp;uint32_t b;  
&emsp;uint8_t c;  
} packed_t;  /* sizeof = 6 -> no padding */  

Use packed structs for network protocol headers and communication frames where byte layout must match exactly. But be careful, accessing unaligned "uint32_t" on ARM can cause a HardFault on some cores (Cortex-M0 doesn't support unaligned access). Always benchmark and test packed and test packed struct access on your target.  

## "offsetof" -> finding field offsets at compile time  

"offsetof(type, member)" gives the byte offset of a field within a struct. It's from "<stddef.h>" and is extremely useful for protocol parsers and generic data structures.  

#include <stddef.h>  

typedef struct {  

&emsp;uint8_t header;  
&emsp;uint16_t length;  
&emsp;uint32_t timestamp;  
&emsp;uint8_t payload[64];  

} frame_t;  

offsetof(frame_t, header)  /* 0 */  
offsetof(frame_t, lenght) /* 2 - after header(1) + padding(1) */  
offsetof(frame_t, timestamp) /* 4 */  
offsetof(frame_t, payload) /* 8 */  


## Passing structs to functions, always by pointer  
/* BAD, copies entire struct onto stack on every call */  
void process_sensor(sensor_t s)  
{  
&emsp;/* s is a full copy, 8 bytes copied */  
&emsp;printf("voltage: %u\n", s.voltage);  
}  

/* GOOD, passes 4-byte pointer, no copy */  
void process_sensor(const sensor_t *s)  
{
&emsp;printf("voltage: %u \n", s->voltage);  
}  

/* GOOD, pointer, function can modify the original */  
void update_sensor(sensor_t *s, uint16_t new_voltage)  
{  

&emsp;s->voltage = new_voltage;  

}  

For small structs (1-2 words) passing by value is sometime fine. For anything larger, always pass by pointer. On Cortex-M, function arguments beyond what fits in R0-R3 go onto the stack, a large struct passed by value can eat hundreads of bytes of stack space per call.  

## Arrays of pointers vs pointer to array  

This distiction is tested constantly in interviews. The two declarations look similar but are completely different.  

### Array of pointers: "int *arr[5]"  

This is an array of 5 pointers to int. The array itself holds 5 pointer-sized elements. Each element is a pointer that can point to a different integer (or array of integers) anywhere in memory.  

int a = 1, b = 2, c = 3, d = 4, e = 5;  
int *arr[5] = { &a, &b, &c, &d, &e };  

*arr[0]   /* 1 -> dereference first pointer */  
*arr[2]   /* 3 -> dereference third pointer */  

Memory layout:-  

arr[0] -> &a -> 1  
arr[1] -> &b -> 2  
arr[2] -> &c -> 3  
arr[3] -> &d -> 4  
arr[4] -> &e -> 5  

Each pointer can point to a different to a different location. This is how "char *argv[]" works, an array of pointers, each pointing to a different string in memory.  

### Pointer to array: "int (*arr)[5]"  

This is a pointer to an array of 5 ints. The pointer itself is one pointer-sized value. When you increment it, it jumps by "5 * sizeof(int) = 20" bytes, past the entire array.  

int matrix[3][5] = {{1,2,3,4,5}, {6,7,8,9,10}, {11,12,13,14,15}};  
int (*row)[5] = matrix;  /* row points to first row */  

row[0][2]  /* 3 -> element [0][2] */  
row++; /* advance past entire first row */  
row[0][2] /* 8 -> now points to second row, elemt [2] */  

This is the correct type for a pointer to a row of a 2D array. Used for passing 2D arrays to functions cleanly.  

Quick memory picture  

int *arr[5]:
[ptr][ptr][ptr][ptr][ptr]  
each ptr is independent  

int (*arr)[5]:  
[ptr] -> [int][int][int][int][int]  
onr ptr to a whole array  

Reading the declarations, read from the variable name outward:  

- int *arr[5] -> "arr" is an array "[5]" of pointers "*" to "int"  
- int (*arr)[5] -> "arr" is a pointer "*" to an array "[5]" of "int"  
- int (*fp)(int, int) -> "fp" is a pointer "*" to a function "(int,int)" returning "int"  

## Linked list, the classic self-referential struct  

A linked list node contains data and a pointer to the next node. The pointer is the same type as the struct itself, self-referential.  

typedef struct Node  
{  

&emsp;int data;  
&emsp;struct Node *next; /* must use 'struct Node', not 'Node', typedef not complete yet  

} Node;  

Why "struct Node *next" and not "Node *next"? Because at the point where "next" is declared, the "typedef" for "Node" isn't complete yet. Using the struct tag "struct Node" is always void.  

#### Building and traversing a linked list  

/* Insert at head -> 0(1) */  
Node* insert-head(Node *head, int data)  
{  

&emsp;Node *new_node = malloc(sizeof(Node));  
&emsp;if (new_node == NULL) return head;  /* allocation failed */  
&emsp;new_node->data = data;  
&emsp;new_node->next = head; /* new node points to old head */  
&emsp;return new_node; /* new node is the new head */  

}  

/* Traverse - 0(n) */  
void print_list(const Node *head)  
{  

&emsp;for(const Node *p = head; p != NULL; p = p->next)  
&emsp;{  

&emsp;&emsp;printf("%d -> ", p->data);  

&emsp;}  
&emsp;printf("NULL\n");  

}  

/* Free entire list */  
void free_list(Node *head)  
{  

&emsp;while (head != NULL)  
&emsp;{  

&emsp;&emsp;Node *next = head->next;  /* save next BEFORE freeing current */  
&emsp;&emsp;free(head);  
&emsp;&emsp;head = next;  

&emsp;}  

}  
/* USAGE */  
Node *list = NULL;  
list = insert_head(list, 3);  /* 3 -> NULL */  
list = insert_head(list, 2);  /* 2->3->NULL */  
list = insert_head(list, 1);  /* 1->2->3->NULL */  
print(list);  
free_list(list);  
list = NULL;  

The "free_list" function is a common interview question. The key mistake is freeing "head" before saving "head->next", after "free(head)", accessing "head->next" is undefined behaviour.  

## Memory mapped register

This is where everything comes together for embedded. The CPU accesses hardware peripheralsby reading and writing to specific memory addresses. We map C structs to those addresses to get named, typed access to hardware registers.  

#include <stdint.h>  
/* UART register layout for STM32F4, matches hardware exactly */  
typedef struct  
{
&emsp;volatile uint32_t SR;  /* 0x00 - Status rgister */  
&emsp;volatile uint32_t DR;  /* 0x04 - Data register */  
&emsp;volatile uint32_t BRR  /* 0x08 - Baud rate register */  
&emsp;volatile uint32_t CR1  /* 0x0C - Control register 1 */  
&emsp;volatile uint32_t CR2  /* 0x10 - Control register 2 */  
&emsp;volatile uint32_t CR3  /* 0x14 - Control register 3 */  
&emsp;volatile uint32_t GTPR; /* 0x18 - Guard time & prescaler */  
}  USART_Typedef;  

/* Map to hardware base addresses */  
#define USART1 ((USART_TypeDef*)0x40011000)  
#define USART2 ((USART_TypeDef*)0x40004400)  
#define USART3 ((USART_TypeDef*)0x40004800)  

/* Bit definitions */  
#define USART_SR_TXE (1U << 7)   /* Transmit data register empty */  
#define USART_SR_RXNE (1U << 5)  /* Read data register not empty */  
#define USART_CR1_UE (1U << 13)  /* USART enable */  
#define USART_CR1_TE (1U << 3)   /* Transmitter enable */  
#define USART_CR1_RE (1U << 2)   /* Receiver enable */  

/* Send a single byte */  
void uart_send_byte(USART_TypeDef *uart, uint8_t byte)  
{
&emsp;while (!(uart->SR & USART_SR_TXE)); /* wait for TX buffer empty */  
&emsp;uart -> DR = byte;  
}  

/* Recieve a single byte */  
uint8_t uart_recv_byte(USART_TypeDef *uart)  
{  

&emsp;while (!(uart->SR & USART_SR_RXNE)); /* wait for data */  
&emsp;return (uint8_t)(uart->DR & 0xFF);  

}  

/* Works for any UART instance */  
uart_send_byte(USART, 'A');  
uart_send_byte(USART, 'B");  

Every field is "volatile" because hardware changes these values, the commpiler must not cache them. The struct is laid out to match the hardware register map exactly, each field is at the correct offset from the base address. This is the pattern used in CMSIS (Cortex Microcontrollers Software Interface Standard) and every STM32/NXP/Nordic HAL.  

## Bit field structs, named bit access  

Instead of manual bit masking, you can define a struct where fields map to specific bit positions.  

/* Maps to UART CR1 register, 32 bits total */  

typedef struct  
{
&emsp;uint32_t SBK : 1; /* bit 0: send break */  
&emsp;uint32_t RWU : 1; /* bit 1: receiver wakeup */  
&emsp;uint32_t RE : 1; /* bit 2: receiver enable */  
&emsp;uint32_t TE : 1; /* bit 3: transmitter enable */  
&emsp;uint32_t IDLEIE : 1; /* bit 4: idle interrupt enable */  
&emsp;uint32_t RXNEIE : 1; /* bit 5: RXNE interrupt enable */  
&emsp;uint32_t TCIE : 1; /* bit 6: TXE interrupt enable */  
&emsp;uint32_t TXEIE : 1; /* bit 7: TXE interrupt enable */  
&emsp;uint32_t PEIE : 1; /* bit 8: parity error interrupt */  
&emsp;uint32_t PS : 1; /* bit 9: parity selection */  
&emsp;uint32_t PCE : 1; /* bit 10: parity control enable */  
&emsp;uint32_t WAKE : 1; /* bit 11: wakeup method */  
&emsp;uint32_t M : 1; /* bit 12: word length */  
&emsp;uint32_t UE : 1; /* bit 13 USART enable */  
&emsp;uint32_t res : 1; /* bit 14: reserved */  
&emsp;uint32_t OVER8 : 1; /* bit 15: oversampling mode */  
&emsp;uint32_t pad : 16; /* bits 16-31: reserved */  
} USART_CR1_bits_t;  

/* Use via union for both bitfield and raw access */  
typedef union  
{  
&emsp;USART_CR1_bits_t bits;  
&emsp;uint32_t raw;  
}  USART_CR1_t;  

volatile USART_CR1_t *cr1 = (USART_CR1_t*)0x4001100C;  

cr1->bits.UE = 1;  /* enable USART, much more readable than: */  
/* USART->CR1 |= (1U << 13)  */  

cr1->bits.TE = 1;  /* enable transmitter */  
cr1->bits.RE = 1; /* enable receiver */  

Bit fields are readable but have portability caveats, bit ordering within a word is implementation defined. For safety critical embedded code (MISRA), manual bit masking is preferred. For readability in non-critical code, bit fields are fine.  

## Function pointer in struct, complete driver pattern  

Expanding on Day 3's introduction, here's complete, realistic embedded driver abstraction:  

#include <stdint.h>  
#include <stddef.h>  

/* Generic UART driver interface */  
typedef struct uart_driver  
{  
&emsp;uint32_t baud_rate;  
&emsp;uint8_t data_bits;  
&emsp;uint8_t stop_bits;  

    /* Internal state */  
&emsp;uint8_t tx_buffer[256];  
&emsp;uint8_t rx_buffer[256];  
&emsp;uint16_t tx_head, tx_tail;  
&emsp;uint16_t rx_head, rx_tail;  

    /* Function pointers, the "methods"  */

&emsp;int (*init) (struct uart_driver *self);  
&emsp;int (*send_byte) (struct uart_driver *self, uint8_t byte);  
&emsp;int (*recv_byte) (struct uart_driver *self, uint8_t *out);  
&emsp;int (*send_buf) (struct uart_driver *self, const uint8_t *buf, size_t len);  
&emsp;void (*flush) (struct uart_driver *self);  
&emsp;void (*deinit) (struct uart_driver *self);  

}  uart_driver_t;  

/* Concrete implementation for USART1 */  
static int usart1_init(uart_driver_t *self)  
{  

    /* Configure USART1 hardware using self->baud_rate etc  */  
&emsp;USART->BRR = compute_brr(self->baud_rate);  
&emsp;USART->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;  
&emsp;return 0;  

}  

static int usart1_send_byte(uart_driver_t *self, uint8_t byte)  
{  
&emsp;(void)self;  
&emsp;while(!(USART1->SR & USART_SR_TXE));  
&emsp;USART->DR = byte;  
&emsp;return 0;  
}  

/* Driver instance, fill in function pointers at init time */  
uart_driver_t usart1_driver = {
&emsp;.baud_rate = 115200,
&emsp;.data_bits = 8,  
&emsp;.stop_bits = 1,  
&emsp;.init = usart1_init,  
&emsp;.send_byte = usart1_send_byte,  
};  

/* Application code, works with any uart_driver_t */  
void send_string(uart_driver_t *drv, const char *str)  
{
&emsp;drv->init(drv);  
&emsp;while(*str)  
&emsp;{  
&emsp;&emsp;drv->send_byte(drv, (uint8_t)*str++);  
&emsp;}  
&emsp;drv->flush(drv);  
}  

/* Same function works for USART1, USART2, SPI-UART bridge, USB-CDC */  
send_string(&usart1_driver, "Hello\r\n");  
send_string(&usart2_driver, "Hello\r\n");  

This code implements a "virtual interface" pattern in C, mimicking object-oriented polymorphism using structs and function pointers. It's a very common technique in embedded systems (drivers for microcontrollers). Let me break it down piece by piece.  

1) The "uart_driver_t" struct, an "interface" + "object" combined  

typedef struct uart_driver  
{  
&emsp;uint32_t baud_rate;  
&emsp;uint8_t data_bits;  
&emsp;uint8_t stop_bits;  
 
&emsp;uint8_t tx_buffer[256];  
&emsp;uint8_t rx_buffer[256];  
&emsp;uint16_t tx_head, tx_tail;  
&emsp;uint16_t rx_head, rx_tail;  



&emsp;int (*init) (struct uart_driver *self);  
&emsp;int (*send_byte) (struct uart_driver *self, uint8_t byte);  
....  
} uart_driver_t;  


Think of this struct as a class:  

- Data fields (baud_rate, buffers,head/tail indices) = the object's state (like private member variables).  

- Function pointers (init, send_byte, etc) = the object's methods (like a virtual method table/vtable in C++).  

Each function pointer takes a "self" pointer as it's first argument, this is exactly what C++ does implicitly with "this". In C, since there's no language support for that, you pass it explicitly.  

2. Concrete implementation, "subclassing" via function pointers  

static int usart1_init(uart_driver_t *self)  
{
&emsp;USART->BRR = compute_brr(self->baud_rate);  
&emsp;USART->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;  
&emsp;return 0;  
}  

static int usart_send_byte(uart_driver_t *self, uint8_t byte)  
{  
&emsp;(void)self;  // unused here since USART1 is a fixed hardware register  
&emsp;while(!(USART->SR & USART_SR_TXE));  // wait until transmit register is empty  
&emsp;USART1->DR = byte;  // write byte to data register  
&emsp;return 0;  
}  

- "usart_init" configures the actual USART1 hardware peripherals baud rate register (BRR) and enables it (UE = USART Enable, TE = Transmit Enable, RE = Receive Enable) using bit flags typical of STM32-style register programming.  

- "usart_send_byte" busy-waits until the hardware's transmit buffer is empty (TXE flag), then writes the byte to the data register (DR) to send it.  

- "static" here means these functions are private to this file, they're not meant to be called directly by other code, only through the function pointers.  

3. The instance, filling in the vtable  

uart_driver_t usart1_driver = {
&emsp;.baud_rate = 115200,  
&emsp;.data_bits = 8,  
&emsp;.stop_bits = 1,  
&emsp;.init = usart1_init,  
&emsp;.send_byte = usart1_send_byte,  
};  

This creates a global "usart_driver" object and wires it's function pointers to the USART1 specific implementations. This is the "constructing an object of a concrete subclass" step. Note: "recv_byte", "send_buf", "flush", "deint" aren't shown here (perhaps omitted for brevity, or would be filled in elesewhere), calling them as is would call through a NULL pointer and crash.

4. Generic application code, polymorphism in action  

void send_string(uart_driver_t *drv, const char *str)  
{
&emsp;drv->init(drv);  
&emsp;while(*str)  
&emsp;{  
&emsp;&emsp;drv->send_byte(drv, (uint8_t)*str++);  
&emsp;}  
&emsp;drv->flush(drv);  
}  

This function doesm't know or care whether "drv" is USART1, USART2, an SPI-UART bridge, or USB-CDC. It just calls "drv->init(drv)" and "drv->send_byte(drv, ...), the actual hardware specific code that runs depends entirely on which functions were stored in those pointers when the driver instance was built.  

This is exactly like calling a virtual method in C++/Java: the caller only knows the interface, and the correct implementation is dispatched at runtime based on the object.  

send_string(&usart1_driver, "Hello\r\n"); // runs usart1_send_byte initially  
send_string(&usart2_driver, "Hello\r\n"); // would run usart2_send_byte internally  

#### Why this pattern is used in embedded C  

- C has no classes/inheritance, so this struct-of-function-pointers trick is the idiomatic way to get polymorphism.  

- It lets you write hardware agnostic application code (send_string) that works across many peripherals (UART, SPI-bridge, USB-CDC) without "#ifdef" spaghetti or duplicated logic.  

- It's the same underlying idea as Linux kernel driver structs (file_operations, net_device_ops, etc).  


## Vector table, array of function pointers in embedded  

The Cortex-M interrupt vector table is literally an array of function pointers stored at a fixed address in Flash. When an interrupt fires, the hardware reads the function pointer from this table and jumps to it.  

/* Function pointer type for interrupt handlers */  
typedef void (*isr_t)(void);  

/* Vector table, placed at address 0x0000 0000 by linker script */  
__attributr__((section(".isr_vector")))  
const isr_t vector_table[] = {  

&emsp;(isr_t)0x2001 0000, /* [0] initial stack pointer, not a function */  
&emsp;reset_handler, /* [1] reset */  
&emsp;nmi_handler, /* [2] non-maskable interrupts */  
&emsp;hardfault_handler, /* [3] hard fault */  
&emsp;memmanage_handler, /* [4] memory management fault */  
&emsp;busfault_handler, /* [5] bus fault */  
&emsp;usagefault_handler, /* [6] usage fault */  
&emsp;0, 0, 0, 0, /* [7-10] reserved */  
&emsp;svcall_handler, /* [11] SVCall */  
&emsp;0, 0, /* [12-13] reserved */  
&emsp;pendsv_handler, /* [14] PendSV */  
&emsp;systick_handler, /* [15] SysTick */  
&emsp;/* External interrupts start at [16] */  
&emsp;uart1_irq_handler, /* [16+37] USART1 global interrupt */  
};  

When USART1 fires an interrupt, the hardware:  

1. Saves CPU state (registers) to the stack  
2. Reads "vector_table[16+37]", gets the address of "uart1_irq_handler"  
3. Jumps to that address  
4. Executes the handler  
5. Restores CPU state and returns to interrupted code  

This is the most hardware fundamental use of function pointers in embedded C. Understanding it means you understand how interrupts physically work on Cortex-M.  

## Question and Answers  


Q1] What is the difference between "int *arr[5]" and "int (*arr)[5]"?  

Answer:-  

"int *arr[5]" -> array of 5 pointers to int. Five pointer sized slots, each can point to different memory locations. Used for arrays of strings, argv-style argument lists, pointer to different structs.  

"int (*arr)[5]" -> a single pointer to an array of 5 ints. Incrementing it moves past an entire 5 element array (20 bytes on 32-bit). Used as a parameetr type for 2D array rows.  

Q2] What is struct padding and how do you minimize it?  

Answer:-  

The compiler inserts padding bytes between struct fields to ensure each field meets it's alignment requirement. Minimize by ordering fields from largest to smallest type, all "uint32_t" fields first, then "uint16_t", then "uint8_t". Use "__attribute__((packed))" to eliminate padding entirely, but be careful of unaligned access faults on Cortex-M0.  


Q3] How does memory-mapped register access work in C?  

Answer:-  

A peripheral's register are mapped to specific physical addresses by the chip's bus fabric. In C, you cast that address to a pointer to an appropriate struct or integer type and access it through the pointer. The "volatile" qualifier is mandatory to prevent the compiler from caching register values. The struct's fields must match the hardware register layout exactly, correct types, correct order, no unintended padding.  

Q4] What is a self-referential struct and where is it used?  

Answer:-  

A struct containing a pointer to another instance of the same type. Must use the struct tag (not the typedef name) when declaring the pointer since the typedef isn't complete yet. Used for linked lists, trees, queues, any dynamic data structure where nodes point to each other.  

Q5] Why must you save "next" before freeing in a linked kist traversal?  

while (head != NULL)  
{  

&emsp;Node *next = head->next;  /* must save before free */  
&emsp;free(head); /* head's memory returned to heap */  
&emsp;head = next; /* accessing head->next here would be UB */  

}  

After "free(head)", the memory is returned to the heap manager. Accessing "head->next" after that is use-after-free, undefined behaviour. On embedded, it typically reads corrupt heap metadata.  

Q6] How does the Cortex-M vector table relate to function pointers?  

Answer:-  

The vector table is an array of function pointers stored in Flash at address 0. Each entry holds the address of an interrupt handler. When an interrupt fires, the hardware uses the interrupt number as an index into this array, reads the function pointer, and branches to it. Writing embedded firmware means you're filling in this array, directly working with function pointers at the hardware interface level.  



















