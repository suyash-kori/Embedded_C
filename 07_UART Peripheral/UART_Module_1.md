# UART Mastery Series (Architecture Level)  

## Why UART exists  

Inside a microcontroller everything is parallel.  

CPU works with:-  

- 32-bit registers  
- 32-bit buses  
- Parallel memory  

But the real world communicates serially:-  

- GPS modules  
- Bluetooth modules  
- Wifi modules  
- Debug consoles  
- Modems  
- Sensors  
- Other microcontroller  

Why serial?  
Because wires are expensive and noisy  

Paralle communication requires:-  
32 wires + clock + ground  

Serial needs:-  
1 wire + ground  

UART exists to convert parallel<--->serial  

UART -> Hardware block that converts CPU parallel data into timed serial bit stream.  

## Where UART sits inside MCU architecture  

Inside MCU, UART is a peripheral  

Data flow:-  

CPU -> Memory -> UART peripheral -> GPIO Pin -> Wire -> Others  

Reverse direction:-  

Wire -> GPIO Pin -> UART Peripheral -> Memory -> CPU  

UART is basically a bridge between CPU world and wire world  

## UART is NOT a protocol (V.imp)  

Many begineers misunderstand this,  
UART is hardware, not a communication protocol.  

