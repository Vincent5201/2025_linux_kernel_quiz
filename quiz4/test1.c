#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POLY  0x82f63b78
void generate() {
    printf("static const uint32_t crc32_table[16] = {\n");
    for (size_t i = 0; i < 16; i++) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 4; j++)
            crc = (crc >> 1) ^ (-(int)(crc & 1) & POLY);
        printf("    0x%08X%s", crc, (i % 4 == 3) ? ",\n" : ", ");
    }

    printf("};\n");
}

int main(void)
{
    generate();
}