# Electric Layer + Signal Timing (The magic of Async)  

Today we will discus:- How does UART receive bits without a clock line?  

### UART Physical Wires  

UART uses only two signals:-  

TX -> Transmit  
RX -> Receiver  

Connection rule:-  

Device A Tx -> Device B Rx  
Device B Tx -> Device A Rx  
GND must be common  

No clock line. No chip select. Nothing else.  
This is why it is called asynchronous serial.  

### Voltage Levels (Logic levels)  

UART itself doesn't define voltage itself (levels)  
GPIO electrical standard decides it.  

#### Common levels:-  

|-----------------------Logic-1----------Logic-0-----|  
MCU UART (TTL)----------3.3V/5V-------------0V-------|  
|----------------------------------------------------|  
RS-232-(PC-old-serial)---(-12V)------------(+12V)----|  

MCU UART != RS-232 UART  
RS-232 uses inverted and higher voltages.  

That's why ICs like MAX232 exist (voltage converter)  

For MCU-to-module communication -> TTL UART (3.3V)  

### Idle Line State (V.Imp)  

UART line rests at:- HIGH (logic 1)  
Why?  
Because communication begins with a falling edge.  
Idle = silence  
Start bit = someone starts talking  

This single transition is how receiver synchronizes.  

### The Big Problem: No Clock  

SPI/I2C have a clock. UART has none.  

So how does receiver know:-  
- When bit starts?  
- When bit ends?  
- When next bit begins?  

This is the genius part of UART.  

### Bit Time (The heart of UART)  

UART agrees on baud rate beforehand.  
Ex:-  

9600 baud -> 9600 bits/sec  
So one bit duration:-  
Bit time = 1/Baud rate = 1/9600 = 104us  
This is the shared timing contract.Both transmitter and receiver run their own clocks, but agree on bit duration.  

### How Receiver Detects incoming Data  

Receiver continously watches RX line:-  

HIGH----HIGH----HIGH-----HIGH----  
Suddenly it seems:-  
HIGH->LOW transition  

This must be the start bit.  
This falling edge is the synchronization event.  
Receiver now starts it's timer.  

### The Genius Trick: Middle Sampling  

After detecting start edge, receiver does NOT sample immediately. It waits half bit time.  

Why??  
Because edges can be noisy. So receiver samples in the middle of each bit where signal is stable.  


### Timeline example (9600 baud)  

Start detected at t = 0  

Wait 0.5 bit -> reach middle of start bit  
Then every 1 bit -> sample data bits  
t = 0.5 bit -> confirm start bit  
t = 1.5 bit -> Data bit 0  
t = 2.5 bit -> Data bit 1  
Receiver samples centre of each bit. This is the core magic of UART.  

### Why Stop Bit must be high  

Stop bit gives timing cushion. Two devices never have perfectly identical clocks. Small clock mismatch accumalates during 10 bits. Stop bit provides recovery time before next frame.  

Without stop bit -> frames would drift and break  

### Clock Error Tolerance (Key insight)  

UART works even if clocks are slightly different.  
Typical tolerance:- +/- 2 % clock mismatch works fine.  

Why??  
Because synchronization happens every frame using start bit. UART resynchronizes for every byte.  
This is why UART is so robust.  


## Baud Rate Generation & Clock Math  

Today you'll understand why UART sometimes works perfectly and sometimes gives garbage characters.  

### What is Baud Rate Really?  

Baud rate = number of bits per second.  
Ex:-  
9600 baud --> 9600 bits/sec  
115200 baud --> 115200 bits/sec  

But here is the key truth:-  
UART does NOT generate exact baud rates.It generates approximate baud rates from system clock. This is the root of many UART bugs.  

### Where Baud Rate Comes from in MCU?  

Inside MCU:-  
System Clock -> UART baud generator -> Bit timing  

MCU clock examples:-  
8MHz  
16MHz  
48MHz  
72MHz  
80MHz  
UART must divide this clock to create bit timing.  

### Basic Baud Rate Formula  

Typical UART uses:-  
Baud = PeripheralClock / (16 * Divider)  
Why 16??  
Because of "Oversampling"(will discuss this in further modules)  

Ex:- 16 MHz MCU @ 9600 baud  

Divider = 16000000 / (16*9600)  
&emsp;&emsp;= 16000000 / 153600  
&emsp;&emsp;= 104.166  

But divider must be on integer. So MCU chooses:- Divider = 104  
Actual baud becomes:-  

Actual baud = 16000000 / (16*104)  
&emsp;&emsp;= 9615 baud  
Not 9600!  
Error = (9615-9600)/(9600) = 0.16% (This is acceptable)  

### Baud Rate Error, The Silent killer  

Receiver also generates it's own baud. So two errors combine:-  

Tx error + Rx error = total error  

UART usually tolerates +/-(2%) total error.  
If error > 2-3% -> you get:-  
- random characters  
- farming errors  
- corrupted data  
This is why UART fails at some speeds.  

### Why 115200 sometimes FAIL  

Let's try 115200 with 16MHz clock  
Divider = 16000000 / (16*115200)  
&emsp;&emsp; = 8.68 -> chooses 9  

Actual baud = 16000000 / (16*9) = 111111 baud  
Error = ~ (-3.5%)  

This is beyond safe limit.  
That's why,  
- 115200 works on same MCU's  
- fails on others  

IMPORTANT:- Real engineers always check baud errors tables.  





