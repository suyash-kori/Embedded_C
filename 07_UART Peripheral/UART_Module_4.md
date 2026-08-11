# Errors, Noise & Reliability  

Today is about what goes wrong in real life. In labs UART looks perfect.In products -> cables, noise, clocks, buffers....things break.  

A senior embedded engineer must understand UART failure modes.  

## Why UART needs Error Detection  

UART has:  
- No clock line  
- No CRC (by default)  
- No packet acknowledgment  

It is a best effort byte stream.  

