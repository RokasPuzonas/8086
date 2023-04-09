#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>

#define u32 uint32_t
#define u16 uint16_t
#define u8  uint8_t

const char *wide_reg_names[8]     = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
const char *non_wide_reg_names[8] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };

const char *lookup_reg_name(u8 idx, bool wide) {
    if (wide) {
        return wide_reg_names[idx];
    } else {
        return non_wide_reg_names[idx];
    }
}

void dissassemble(FILE *src, FILE *dst) {
    fprintf(dst, "bits 16\n\n");
    while (!feof(src)) {
        u8 byte1 = fgetc(src);
        u8 byte2 = fgetc(src);

        // Register memory to/from register
        if ((byte1 & 0b11111100) == 0b10001000) {
            bool wide = byte1 & 0b1;
            bool direction = (byte1 & 0b10) >> 1;

            u8 mod =  byte2 & 0b11000000;
            u8 reg = (byte2 & 0b00111000) >> 3;
            u8 rm  =  byte2 & 0b00000111;
            if (mod == 0b11000000) { // Mod = 0b11
                const char *reg_name = lookup_reg_name(reg, wide);
                const char *rm_name  = lookup_reg_name(rm, wide);
                if (direction) {
                    fprintf(dst, "mov %s, %s\n", reg_name, rm_name);
                } else {
                    fprintf(dst, "mov %s, %s\n", rm_name, reg_name);
                }
            } else if (mod == 0b10000000) { // Mod = 0b10
            } else if (mod == 0b01000000) { // Mod = 0b01
            } else if (mod == 0b00000000) { // Mod = 0b00
            }
        }
    }
}