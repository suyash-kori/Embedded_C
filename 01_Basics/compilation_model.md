## C Compilation Model (Embedded Perspective)

Flow of converting C code to firmware:

main.c → Preprocessor → Compiler → Assembler → Linker → Firmware.elf/.hex

### Preprocessor
Handles:
#include
#define
#ifdef

### Compiler
Converts C → Assembly.

### Assembler
Assembly → Object file (.o)

### Linker
Combines all object files and creates final firmware image.