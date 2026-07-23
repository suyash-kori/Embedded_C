# UART Frame Structure (The Anatomy of a Byte)  

When we send a byte over UART, what actually goes on the wire bit-by-bit?  

When UART sends 1 byte, it does NOT send just 8 bits, It sends a frame.  

A UART frame is like an envelope around your data.  

## Typical frame looks like this:-  

idle----start----D0--D1--D2--D3--D4--D5--D6--D7-------Parity-----Stop  
1--------0--------------data-bits---------------------optional-----1    
|  
V  
not included  
in the frame  
 
Ex- (8N1 format)  
 
|Start|8-Data-bits|Stop|  
Total bits transmitted = 10 bits per byte  
UART always sends more bits than your data.  

## Why do we need a frame?  

UART has No Clock line.  
So the receiver must figure out:-  

&emsp;- When transmission begins  
&emsp;- When bits start/end  
&emsp;- When byte is finished  

The frame provides this synchronization.  

## Bit-by-Bit Deep Explanation  

### Idle State (Line resting state)  

When nothing is transmitted:-  

TX line = HIGH (logic 1)  

Why not LOW???  

Because the receiver needs a falling edge to detect the start of transmission.  

So line stays HIGH forever until data begins.  
Think of it like silence before speaking.  

### Start Bit  

Transmission begins by pulling line LOW for one bit time.  

Start bit = 0  
This falling edge tells the receiver:-  

"Wake up! Data is coming!"  

This is how UART synchronizes without a clock.  

### Data Bits (The actual payload)  

Now the real byte is sent.  
Imp-Rules:-  

- Sent LSB first  
- Usually 8 bits, but can be 5-9 bits  

Example:-  
sending ASCII 'A'  
'A'= 0x41 = 01000001  

UART sends it reversed order:-  

10000010  
|  
V  
LSB First  

Why LSB first?  
Because old teleprinters worked that way.  

### Optional Parity Bit (Error detection)  

Parity is simple error checking.  
It answers:-  

"Did a bit flip during transmission?"  

Two types:-  
Even parity:-  
Total number of 1s must be even.  
Odd parity:-  
Total number of 1s must be odd.  

Ex:-  

Data = 10100010 (3 one's)  

Even parity -> add 1 -> becomes 4 one's  
Odd parity -> add 0 -> remains 3 one's  

Parity helps detect single-bit errors  

But many systems disable it to increase speed.  
Common configs:-  

- 8N1 -> No parity (most common)  
- 8E1 -> Even parity  
- 8O1 -> Odd parity  

### Stop Bit(Breathing space)  

Stop bit = HIGH  
Purpose:-  

- Marks end of frame  
- Gives receiver time to process byte  
- Allows clock drift tolerance  

Can be:-  
- 1 stop bit  
- 1.5 stop bit  
- 2 stop bit  

Most common -> 1 stop bit  

Putting it all together:-  

Sending ASCII "A" (0x41) using 8N1:-  

|-Start-----Data-bits-(LSB-first)-----Stop-| 
|---0---------1-0-0-0-0-0-1-0----------1---|  

So on wire you actually transmit:-  

Total bits = 10 bits  
This leads to a huge practical insight:-  

### UART Throughput Reality:-  

At 9600 baud:-  
You think you send:- 9600 bits/sec  
But per byte you send 10 bits.  

So real throughput:-  
960 bytes/sec  

UART has 20% overhead  
This becomes important in next Modules(Throughput and Buffers)  

### Most common UART configuration  

We will see this everywhere:-  

- 9600----->8N1  
- 115200--->8N1  
Meaning:-  
- 8 bits data  
- No parity  
- 1 stop bit  








