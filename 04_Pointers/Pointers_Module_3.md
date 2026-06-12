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

## Function pointers in structs, driver abstraction  

This is how real embedded driver layers are built. A struct of function pointers acts like a virtual function table (vtable), the same concept C++ uses under the hood for virtual functions.  

/* Generic sensor interface */  
typedef struct  
{  
&emsp;int (*init) (void);  
&emsp;int (*read) (int16_t *value);  
&emsp;void (*sleep) (void);  
&emsp;void (*wakeup) (void);   
} sensor_driver_t;  

/* Concrete implementation for sensor A */  
int sensorA_init (void)  { /* init SPI, configure */ return 0;  }  
int sensorA_read (int16_t *value) { *value = spi_read_temp(); return 0; }  
void sensorA_sleep (void)  { gpio_write(SLEEP_PIN, 1); }  
void sensorA_wakeup (void) { gpio_write(SLEEP_PIN, 0); }  

sensor_driver_t sensorA = {  

&emsp;.init = sensorA_init,  
&emsp;.read = sensorA_read,  
&emsp;.sleep = sensorA_sleep,  
&emsp;.wakeup = sensorA_wakeup,  
};  

sensor_driver_t sensorB = {  

&emsp;.init = sensorB_init,  
&emsp;.read = sensorB_read,  
&emsp;.sleep = sensorB_sleep,  
&emsp;.wakeup = sensorB_wakeup,  
};  

/* Application code, works with any sensor */  

void read_sensor(sensor_driver_t *drv, int16_t *out)  {  
&emsp;drv->init();  
&emsp;drv->read(out);  
&emsp;drv->sleep();  
}  

/* Works for both sensors without changing application code */  
read_sensor(&sensorA, &temp_a);  
read_sensor(&sensorB, &temp_b);  

The Core idea, what problem this solves??  

You have two different sensors, both need init,read,sleep,wakeup, but each does it differently internally. Without this pattern:  

// You'd need separate functions for every sensor  
if (sensor_type == A)  
{  
&emsp;sensorA_init();  
&emsp;sensorA_read(&val);  
&emsp;sensorA_sleep();  
}  
else if (sensor_type == B)  
{  
&emsp;sensorB_init();  
&emsp;sensorB_read(&val);  
&emsp;sensorB_sleep();  
}  

Step 1 - The struct with function pointers  

typedef struct  
{  
&emsp;int (*init) (void);  
&emsp;int (*read) (int16_t *value);  
&emsp;void (*sleep) (void);  
&emsp;void (*wakeup) (void);   
} sensor_driver_t;  

This struct doesn't hold data like usual structs. It holds 4 function pointers, slots for 4 functions.  

sensor_driver_t in memory:  

| init | address slot | <- holds address of some init function  
| read | address slot | <- holds address of some read function  
| sleep| address slot | <- holds address of some sleep function  
|wakeup| address slot | <- holds address of some sleep function  

Think of it as a contract, any sensor that fills these 4 slots can be used by the application  

Step 2 - Concrete implementations  

These are the real functions for sensor A:  

int sensorA_init (void)  { /* init SPI, configure */ return 0;  }  
int sensorA_read (int16_t *value) { *value = spi_read_temp(); return 0; }  
void sensorA_sleep (void)  { gpio_write(SLEEP_PIN, 1); }  
void sensorA_wakeup (void) { gpio_write(SLEEP_PIN, 0); }  

Each function does hardware specific work. "sensorA_read" dereferences the pointer to write the temperature value back to the caller:  

*value = spi_read_temp();  
// writes result into wherever value is pointing  
// caller gets the result through their variable  

Step 3 - Filling the Struct (Binding)  

sensor_driver_t sensorA = {  

&emsp;.init = sensorA_init,  // put adddress of sensorA_init into init slot
&emsp;.read = sensorA_read,  // put address of sensorA_read into read slot
&emsp;.sleep = sensorA_sleep,  
&emsp;.wakeup = sensorA_wakeup,  
};  

Now sensor A's struct looks like this in memory:  

sensorA:  
| init | address of sensorA_init |  
| read | address of sensorA_read |  
| sleep| address of sensorA_sleep|  
|wakeup| address of sensorA_wakeup|  

sensorB:  
| init | address of sensorB_init |  
| read | address of sensorB_read |  
| sleep| address of sensorB_sleep|  
|wakeup| address of sensorB_wakeup|  

Same shape, different addresses inside.  

Step 4 - The Generic application function  

void read_sensor(sensor_driver_t *drv, int16_t *out)  {  
&emsp;drv->init();  
&emsp;drv->read(out);  
&emsp;drv->sleep();  
}  

"drv" is a pointer to whichever sensor struct you pass in. "drv->init" goes to that struct and reads the address stored in the "init" slot, the calls it

read_sensor(&sensorA, &temp_a):  

drv = address of sensorA struct  

drv->init() -> go to sensorA struct -> read init slot -> call sensorA_init()  

drv->read() -> go to sensorA struct -> read read slot -> call sensorA_read(&temp_a)  

drv->sleep() -> go to sensorA struct -> read sleep slot -> call sensorA_sleep()  

Same with sensor B...  

Same code path, different functions executing, this is polymorphism in C.  

##### Full Picture, memory aand flow  

FLASH (code):  

sensorA.init <-----0x0801  
sensorA.read <-----0x0815  
sensorA_sleep<-----0x0830  

sensorB_init <-----0x0900  
sensorB_read <-----0x0914  
sensorB_sleep<-----0x0928  

read_sensor <------generic, knows nothing  about A or B  

RAM (data):  

sensorA.init = 0x0801  
sensorA.read = 0x0815  
sensorA.sleep= 0x0830  

sensorB.init = 0x0900  
sensorB.read = 0x0914  
sensorB.sleep= 0x0928  

Call: read_sensor(&sensorA, &temp_a)  

It calls:-  

drv->init() -> CPU reads 0x0801 from sensorA.init -> jumps there -> sensorA_init runs  

drv->read() -> CPU reads 0x0815 from sensorA.read -> jumps there -> sensorA_read runs  

drv->sleep() -> CPU reads 0x0830 from sensorA.sleep -> jumps there -> sensorA_sleep runs  

Why this pattern is used everywhere in embedded  

Benefit--------------------------------Example  

Add sensor C-----Just write 4 functions + fill one struct - application untouched

Swap sensor at runtime---Pass different struct pointer, same application code runs

Test/mock easily----Create a fake struct with dummy functions for testing  

Clean separation----Driver code knows hardware, application code knows nothing about hardware  

This is essentially how object-oriented programming works, in C, manually.  
In C++ this same pattern is done automatically with classes and vitual functions.  


## A complete working example trying everything together  

#include <stdio.h>  
#include <stdint.h>  

/* Function pointer type for a math operation */  
typedef int (*math_fn)(int, int);  

/* Operations */  
int add(int a, int b) { return a + b; }  
int sub(int a, int b) { return a - b; }  
int mul(int a, int b) { return a * b; }  

/* Higher order function, takes a function pointer */  

void apply_to_array(int *arr, size_t len, int operand, math_fn op)  
{  
&emsp;for(size_t i = 0; i < len; i++)  
&emsp;{  
&emsp;&emsp;arr[i] = op(arr[i], operand);  
&emsp;}  
}  

/* Pass by pointer, modifies original */  
void scale_and_offset(int *arr, size_t len, int scale, int offset)  
{
&emsp;for(size_t i = 0; i < len; i++)  
&emsp;{  

&emsp;&emsp;arr[i] = arr[i] * scale + offset;  

&emsp;}  

}  

int main(void)  
{
&emsp;int arr[5] = {1, 2, 3, 4, 5};  

&emsp;apply_to_array(arr, 5, 10, add);  // adds 10 to all each element  
&emsp;/* arr = {11, 12, 13, 14, 15} */  

&emsp;apply_to_array(arr, 5, 2, mul);  // multiplies each by two  
&emsp;/* arr = {22, 24, 26, 28, 30} */  

&emsp;scale_and_offset(arr, 5, 1, -5);  // subtracts 5 from each  
&emsp;/* arr = {17, 19, 21, 23, 25} */  

&emsp;for (int i = 0, i < 5; i++)  
&emsp;{  

&emsp;&emsp;printf("%d ", arr[i]);  

&emsp;}  

}

## Questions and Answer  

1) What is wrong with this code?  

int* get_buffer(void)  
{  
    int buf[16];  
    return buf;  

}  

Answer:-  

Returns the address of a local array. The array lives on the stack and is distroyed when the function returns. The caller receives a dangling pointer. Fix: make "buf" static, or have the caller pass in a buffer.  

2) What is a callback function and how do you implement one in C?  

Answer:-  

A callback is a function whose address is passed to another function, to be called later when a specific event occurs. Implemented via function pointers. Used heavily in embedded for ISR handlers, timer callbacks, and HAL event hooks. The pattern: define a "typedef" for the function signature, store a pointer of that type, call it when the event fires, always check for NULL before calling.  

3) How do you declare a function pointer to "int foo(char*, int)?  

Answer:-  

int (*fp)(char*, int);  
// or with typedef:  
typedef int (*foo_fn)(char*, int);  
foo_fn fp;  

4) What is the difference between passing a struct by value vs by pointer?  

Answer:-  

By value: The entire struct is copied onto the stack. For a 256-byte struct, that's 256 bytes of stack space and a copy operation every call, expensive on embedded. By pointer: only 4 bytes (the address) are passed. The function accesses the original directly. Always pass structs by pointer in C. Use "const" pointer if the function shouldn't modify it.  

5) How does a dispatch table work and why is it better than a switch statement?  

Answer:-  

A dispatch table is an array of function pointers indexed by a command or even ID.Instead of a switch with N cases, you do one array lookup and one indirect function call, 0(1) regardless of number of commands. Adding a new command means adding one array entry, not touching the dispatch logic. More maintainable and faster for large numbers of commands.  

6) Explain how the function pointers struct pattern replaces virtual functions in C.  

Answer:-  

A struct of function pointers acts as a manual vtable. Different "implementations" fill in the struct fields with their specific functions. Code that opeartes on the struct calls through the pointers without knowing the concrete implementation. This is runtime polymorphism in C, the same concept as C++ virtual functions but explicit. Used in Linux kernel drivers, Zephyr RTOS,and most professional embedded HAL layers.















