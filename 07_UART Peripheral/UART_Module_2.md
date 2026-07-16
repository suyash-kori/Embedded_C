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




