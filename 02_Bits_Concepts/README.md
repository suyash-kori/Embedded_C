# Bit Manipulation in Embedded C

Bit manipulation is one of the most important skills in embedded systems.

Microcontrollers interact with hardware using **registers**, and registers are controlled by setting and clearing individual bits.

---

## Why Bit Manipulation?

In embedded systems we often need to:
- Enable/Disable peripherals
- Configure GPIO pins
- Set communication modes
- Control interrupts

All of this is done by modifying specific bits in registers.

Example register:
00000000

Each bit controls a hardware feature.

---

## Basic Bitwise Operators

| Operator | Name | Use |
|---|---|---|
| `&` | AND | Clear bits |
| `|` | OR | Set bits |
| `^` | XOR | Toggle bits |
| `~` | NOT | Invert bits |
| `<<` | Left Shift | Move bits left |
| `>>` | Right Shift | Move bits right |

---

## Important Bit Operations

### 1️⃣ Set a Bit
```c
reg |= (1 << pos);

### 1️⃣ Set a Bit
```c
### 2️⃣ Clear a Bit
```c
reg &= ~(1 << pos);
### 3️⃣ Toggle a Bit
```c
reg ^= (1 << pos);

## Real Embedded Example

Enabling UART transmit might look like:

UART_CTRL_REG |= (1 << 3);

This sets bit 3 in the control register.