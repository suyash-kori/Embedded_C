# Pass by value, Pass by Reference & Function Pointers  

#### First, understand what "pass by value" actually means at the hardware level.  

When you call a function in C, arguments are passed either through CPU registers or pushed onto the stack, depending on the calling convention (AAPCS for ARM, cdecl for x86, etc.). The key point is: a copy is made.  

void foo(int x)  
{  
&emsp;x = 99;  
}  

int main(void)  
{  
&emsp;int a = 10;  
&emsp;foo(a);  
&emsp;printf("%d\n", a);  // still 10, foo worked on a copy  
}  

When foo(a) is called, the value 10 is copied into a new stack slot (or register) named x. foo modifies it's own local copy. When foo returns that copy is gone. "a" in main is untouched.  

At the assembly level on ARM Cortex-M, this looks roughly like:  

LDR R0, [a]  ;  load value of a into register R0  
BL foo  ;  call foo, R0 holds the copy  
; foo does it's thing with R0, returns  
; a in memory is completely unaffected  

This is not a C language quirk. It's fundamental to how function calls work on hardware.  

#### C has no "pass by reference", only pass by pointer  

Unlike C++ which has actual references (int &x), C only has one mechanism: pass by value. But you can pass the value of a pointer, which is an address, and through that address modify the original variable. This is what people mean when they say "pass by reference in C".  

void foo(int *x)  
{  
&emsp;*x = 99;   // go to the address x holds, write 99 there  
}  

int main(void)  
{  
&emsp;int a = 10;  
&emsp;foo(&a);  // pass the address of a  
&emsp;printf("%d\n", a);  // 99 - a was changed  
}  

What happened here:  

a is at address 0x2000, value = 10  

foo receives x = 0x2000 (a copy of the address, not a copy of a)  
*x = 99 means:  go to 0x2000,  write 99  
a is now 99  

The pointer "x" itself is passed by value, "x" is a copy of "&a". But since that copy is a valid address pointing to "a", you can reach through it and modify "a". This is the entire mechanism.  

### The swap function, the classic example  

This is the most common example used to illustarte pass by pointer, and you should be able to write it from memory.  

/* Wrong, passes copies, original variables unchanged */  

void swap_wrong(int a, int b)  
{  
&emsp;int temp = a;  
&emsp;a = b;  
&emsp;b = temp;  
&emsp;&emsp;// a and b are local copies, nothing outside changes  
}  

/* Correct, passes addresses, originals are modified */  
void swap_correct(int *a, int *b)  
{  
&emsp;int temp = *a;  
&emsp;*a = *b;  
&emsp;*b = temp;  
}  

int main(void)  
{  
&emsp;int x = 5, y = 10;  

&emsp;swap_wrong(x, y);  
&emsp;printf("%d %d\n", x, y);  // 5 10 - unchanged  

&emsp;swap_correct(&x, &y);  
&emsp;printf("%d %d\n", x, y);  // 10 5 - swapped  
}  

Trace through "swap_correct" carefully:  

x = 5 at 0x2000  
y = 10 at 0x2004  

swap_correct receives: a = 0x2000, b = 0x2004  

temp = *a  -> temp = 5  
*a = *b  -> 0x2000 gets value 10 -> x = 10  
*b = temp -> 0x2004 gets value 5 -> y = 5  

## const with pointer paramters, four combinations, know them all  

When you pass pointers to functions, "const" controls what the function is allowed to do. This is heavily used in embedded for protecting buffers, config structs, and hardware register maps.  

/* 1. "Non-const" pointer to non-const data  
--Function can change both the pointer and the data */  

void foo(int *p);  

/* 2. Const pointer to non-const data  
--Function cannot move the pointer, but can change the data */  

void foo(int * const p);  

/* 3. Non-const pointer to const data <- most common in embedded  
--Function cannot change the data, but can move the pointer */  

void foo(const int *p);  

/* 4. Const pointer to const data  
--Function cannot change either */  

void foo(const int * const p);  

In practice, form 3 is by far the most common, it's how you say "I promise not to modify this buffer":  

/* src is read-only, dst is writable */  
void mem_copy(uint8_t *dst, const uint8_t *src, size_t len)  
{  
&emsp;while(len--)  
&emsp;{  
&emsp;&emsp;*dst++ = *src++;  // reading src, writing dst  
&emsp;}  
}  

/* This is also how standard library functions are declared */  
/* void *memcpy(void *dest, const void *src, size_t n);  

If you try to modify data through a "const" pointer, the compiler catches it at compile time, that's the whole point.  

## Returning pointers from functions, the "Dangling pointer" trap  

A function can return a pointer, but what it points to must still be alive after the function returns.  

WRONG, returning address of a local variable:  

int* get_value(void)  
{  
&emsp;int x = 10;  // x lives on the stack  
&emsp;return &x;  // stack frame is destroyed when function returns  
}  // caller receives a dangling pointer  

int main(void)  
{  
&emsp;int *p = get_value();  
&emsp;printf("%d\n", *p);  // undefined behaviour, stack memory was reused  
}  

This compiles with just a warning, not an error. The bug is subtle, sometimes it "works" because the stack memory hasn't been overwritten yet, which makes it even more dangerous.  

/* Option 1: return pointer to static variable */  
int* get_value(void)  
{  
&emsp;static int x = 10;  // static lives in data segment, not stack  
&emsp;return &x;  // valid - x persists after function returns  
}  

/* Option 2: caller provides the buffer, preferred in embedded */  
void get_value(int *result)  
{  
&emsp;*result = 10;  
}  

/* Option 3: return pointer to heap memory, caller must free */  

int* get_value(void)  
{  
&emsp;int *p = malloc(sizeof(int));  
&emsp;if (p) *p = 10;  
&emsp;return p;  // caller responsible for free()  
}  
In embedded firmware, option 2 is most common, you pass a buffer in, the function fills it. No dynamic allocation, no static state issues.  

## Passing arrays to functions, always a pointer  

As covered in previous Module(Module 2), arrays decay to pointers when passed. But let's go deeper on the patterns.  

/* All three are identical to the compiler */  

void process(int *arr, size_t len);  
void process(int arr[], size_t len);  
void process(int arr[10], size_t len);  // 10 is ignored  

A real embedded pattern, processing a UART receive buffer:  

typedef struct  
{  
&emsp;uint8_t data[64];  
&emsp;size_t len;  
} uart_frame_t;  

/* Pass struct by pointer, avoids copying 65 bytes onto stack */  
int parse_frame(const uart_frame_t *frame)  
{  
&emsp;if (frame == NULL || frame->len == 0)  
&emsp;&emsp;return -1;  

&emsp;const uint8_t *p = frame->data;  
&emsp;const uint8_t *end = frame->data + frame->len;  

&emsp;while (p < end)  
&emsp;{  
&emsp;&emsp;/* process each byte */  
&emsp;&emsp;p++;  
&emsp;}  
&emsp;return 0;  
}  

int main():
{
&emsp;uart_frame_t my_frame; // create a struct on stack  
&emsp;my_frame.len = 5;  

&emsp;parse_frame(&my_frame); // pass ADDRESS of the struct  
&emsp;&emsp;&emsp; ^ address of the whole struct  
}  

i)  
const uart_frame_t *frame -> The const means you cannot modify the struct through this pointer inside the function.  

frame->len = 10; // WRONG, compiler error  

uint8_t x = frame->data[0];  // CORRECT  

ii) const uint8_t *p = frame->data; and *end = frame->data+frame->len;  

Both are addresses. This is the important part, the "*" in the declaration means "this variable holds an address". There is no dereference happening on those lines:  

const uint8_t *p = frame->data;  
&emsp;&emsp; ^ p is a pointer, it stores an address  
&emsp;&emsp; p = address of frame->data[0]  

const uint8_t *end = frame->data + frame->len;  
&emsp;&emsp; end = address of frame->data[0] + number of bytes  
&emsp;&emsp;&emsp;= address just past the last valid byte  

So if "frame->data" starts at memory address "0x1000" and len = 5:  
p = 0x1000 (start address)  
end = 0x1000 + 5 (= 0x1005, one past the last byte)  

The "*" is only used as a derefernce when you write "*p" to read the value at that address, but in the declarations above, the "*" is just saying "this variables is a pointer", not reading anything.  

Always pass structs by pointer in C, never by value unless the struct is very small(1-2 words). Passing by value copies the entire stuct onto the stack, which wastes RAM and cycles on embedded systems.  

## Function pointers, the most powerful feature you're not using enough  

A function pointer stores the address of a function. Just like data pointers point to variables, function pointers point to execute code.  

Syntax - the hard part:  

int add(int a, int b)  
{  
&emsp;return a + b;  
}  

/* Declare a function pointer */  
int (*fp)(int, int);  // fp is a pointer to a function taking two ints, returning int  

/* Assign it */  
fp = add;  // function name decays to it's address, just like arrays  
fp = &add;  // also valid explicit address of  

/* Call it */  
int result = fp(3,4);  // 7, calls add through the pointer  
int result = (*fp)(3, 4);  // also valid, explicit dereference style  

The syntax "int (*fp)(int, int)" reads as: "fp is a pointer (*fp) to a function "(int, int) and returning "int". The parentheses around "*fp" are mandatory, without them, "int *fp(int, int)" means "fp is a function returning "int*", which is completely different.  

## typedef for function pointers, always do this  

Raw function pointer syntax is unreadable. Always typedef it:  

/* Without typedef, painful */  
int (*operation)(int, int);  
void apply(int (*op)(int, int), int a, int b);  

/* With typedef, clean */  
typedef int(*operation_fn)(int, int);  

operation_fn op = add;  
void apply(operation_fn op, int a, int b);  

## Callback functions, the key embedded use case  

A callback is a function you pass to another function (or register with a driver), which gets called later when an event occurs. This is fundamental to embedded, ISR callbacks, timer callbacks, HAL callbacks.  

typedef void (*callback_fn)(uint8_t data);  

/* UART driver, registers a callback to be called when byte received */  

static callback_fn rx_callback = NULL;  

void uart_register_callback(callback_fn cb)  
{  
&emsp;rx_callback = cb;  
}  

/* Called from ISR when byte arrives */  
void UART1_IRQHandler(void)  
{  
&emsp;uint8_t byte = UART1->DR;  
&emsp;if (rx_callback != NULL)  
&emsp;{  
&emsp;&emsp;rx_callback(byte);  // call whatever function was registered  
&emsp;}  
}  

/* Application Code */  
void my_handler(uint8_t data)  
{  
&emsp;process_byte(data);  
}  

int main(void)  
{  
&emsp;uart_register_callback(my_handler);  // register our function  
&emsp;// now my_handler() will be called every time a byte arrives  
}  

This decouples the driver from the application. The UART driver doesn't need to know anything about "my_handler", it just calls whatver function pointer was registered. This is how every well written embedded HAL works.  
Full Explanation:-  

i) typedef void (*callback_fn) (uint8_t data)  

This is just creating a named type for a function pointer.  

callback_fn = a pointer to any function that:  
- takes one argument: uint8_t data  
- returns nothing (void)  

So "callback_fn" is just a convenient name. ANy function matching that shape can be stored in it.  

ii) static callback_fn rx_callback = NULL;  

This of this as an empty slot sitting in memory, initially pointing to nothing (NULL).  

rx_callback = [ NULL ]  <- nobody registered yet   

#### Step by Step, what runs after what  

Step 1:  
i)main() starts  

ii)"uart_register_callback(my_handler)"  

iii)rx_callback = my_handler (slot now holds address of my_handler) i.e rx_callback = [address of my_handler] <- filled!  

Step 2:  

main() continues doing other things...CPU is running normally.  

Step 3:  

i) Hardware receives a byte on UART line  

ii) Hardware INTERRUPTS the CPU(whatever main was doing is PAUSED)  

iii) CPU automatically jumps to UART1_IRQHandler()  

Step 4:  

i) uint8_t byte = UART->DR -> read the received byte from hardware register  

ii) rx_callback != NULL? -> YES, we registered my_handler  

iii) rx_callback(byte) -> this calls my_handler(byte)  

iv) my_handler runs -> process_byte(data)  

v) ISR finishes  

vi) CPU resumes main() from where it was paused  

Why this design?  Why not call "my_handler" directly in ISR?  
Because the UART driver (the ISR) doesn't know about your application code. It's meant to be reusable.  

// Driver knows nothing about your app  
// But it exposes a slot, "tell me what to call when a byte arrives"  

// App fills that slot:  
uart_register_callback(my_handler);  

// Tomorrow you want different behaviour? Just register different function:  
uart_register_callback(my_logger);  
uart_register_callback(my_parser);  

The ISR doesn't change at all. Ony what's in the slot changes. This is the callback pattern, very common in embedded drivers.  

What happens in memory  

FLASH (code):  

UART1_IRQHandler -> ISR, hardwired to interrupt vector.  

uart_register_callbackm & my_handler -> your function, sitting at some address.  

RAM:  

rx_callback -> 4 bytes holding address of my_handler  
When ISR runs:  

rx_callback(byte) -> CPU reads address stored in rx_callback  

Jumps to that address -> lands inside my_handler  

#### One line summary of each piece  

CODE------------------ROLE  

callback_fn-------Type defination, shape of allowed functions  

rx_callback-------The slot, stores which function to call  

uart_register_callback()-----Fills the slot  

UART1_IRQHandler()--Runs automatically on interrupt, calls whatever is in the slot.  

my_handler()----Your actual logic, registered into the slot.  

## Dispatch table, function pointers arrays  

A dispatch table is an array of function pointers, indexed to select behaviour at runtime. This is extremely common in embedded for command parses, state machines, and protocol handlers.  

typedef void (*cmd_handler)(uint8_t *payload, size_t len)  

void handle_led_on (uint8_t *p, size_t l) { LED_ON(); }  
void handle_led_off(uint8_t *p, size_t l) { LED_OFF(); }  
void handle_reset (uint8_t *p, size_t l) { NVIC_SystemReset(); }  
void handle_status (uint8_t *p, size_t l) { send_status(); }  

/* Dispatch table indexed by command ID */  
cmd_handler dispatch_table[] = {
&emsp;[0x01] = handle_led_on,  
&emsp;[0x02] = handle_led_off,  
&emsp;[0x03] = handle_reset,  
&emsp;[0x04] = handle_status,  
};  

/* Command parser */  
void process_command(uint8_t cmd_id, uint8_t *payload, size_t len)  
{
&emsp;if (cmd_id < sizeof(dispatch_table)/sizeof(dispatch_table[0]) && dispatch_table[cmd_id] != NULL)  
&emsp;{
&emsp;&emsp;dispatch_table[cmd_id](payload, len);  // call appropriate handler  
&emsp;}
}

Without function pointers, this would be a giant "switch" statement. With a dispatch table,a dding a new command is just adding one entry to the table, no touching the core parsing logic.  

The problem this solves,  
Without a dispatch table, you'd write this:  

if(cmd_id == 0x01) handle_led_on(payload, len);  
else if (cmd_id == 0x02) handle_led_off(payload, len);  
else if (cmd_id == 0x03) handle_reset(payload, len);  
else if (cmd_id == 0x04) handle_status(payload, len);  

With 50 commands, this becomes 50 if-else blocks. The dispatch table replaces all of it with one line.  

Step 1 - The typedef (function pointer type)  

typedef void (*cmd_handler)(uint8_t *payload, size_t len);  

This creates a type called "cmd_handler". Any function that:  

- takes "uint8_t *payload" and "size_t len"  
- returns "void"  

can be stored as a "cmd_handler". All 4 handler functions match this shape.  

Step 2 - The dispatch table  

cmd_handler dispatch_table[] = {
[0x01] = handle_led_on,  
&emsp;[0x02] = handle_led_off,  
&emsp;[0x03] = handle_reset,  
&emsp;[0x04] = handle_status,  
};  

This is just an array of function pointers. Each slot holds the address of a function.  

dispatch_table in memory:  

index---[0x00]----[0x01]-------[0x02]--------[0x03]-------[0x04]  
&emsp;--NULL|handle_led_on|handle_led_off|handle_reset|handle_status|  

















