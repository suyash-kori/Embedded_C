#include <stdio.h>

#define SET_BIT(num, pos)     (num |= (1 << pos))
#define CLEAR_BIT(num, pos)   (num &= ~(1 << pos))
#define TOGGLE_BIT(num, pos)  (num ^= (1 << pos))

int main()
{
    unsigned int reg = 0x00;

    printf("Initial value: 0x%X\n", reg);

    // Set bit 3
    SET_BIT(reg, 3);
    printf("After setting bit 3: 0x%X\n", reg);

    // Clear bit 3
    CLEAR_BIT(reg, 3);
    printf("After clearing bit 3: 0x%X\n", reg);

    // Toggle bit 2
    TOGGLE_BIT(reg, 2);
    printf("After toggling bit 2: 0x%X\n", reg);

    return 0;
}