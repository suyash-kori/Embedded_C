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

    /* Function pointers, the "methods"  

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

    /* Configure USART1 hardware using self->baud_rate etc  
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
&emsp;.init = usart_init,  
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






















