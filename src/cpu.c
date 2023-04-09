#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>

#define u32 uint32_t
#define u16 uint16_t
#define u8  uint8_t

struct cpu_state {
    u16 ax, bx, cx, dx, sp, bp, si, di;
};

const char *lookup_reg_name(u8 idx, bool wide) {
    if (wide) {
        if        (idx == 0b000) {
            return "ax";
        } else if (idx == 0b001) {
            return "cx";
        } else if (idx == 0b010) {
            return "dx";
        } else if (idx == 0b011) {
            return "bx";
        } else if (idx == 0b100) {
            return "sp";
        } else if (idx == 0b101) {
            return "bp";
        } else if (idx == 0b110) {
            return "si";
        } else if (idx == 0b111) {
            return "di";
        }
    } else {
        if        (idx == 0b000) {
            return "al";
        } else if (idx == 0b001) {
            return "cl";
        } else if (idx == 0b010) {
            return "dl";
        } else if (idx == 0b011) {
            return "bl";
        } else if (idx == 0b100) {
            return "ah";
        } else if (idx == 0b101) {
            return "ch";
        } else if (idx == 0b110) {
            return "dh";
        } else if (idx == 0b111) {
            return "bh";
        }
    }

    printf("ERROR: Unknown register %d, wide %d", idx, wide);
    abort();
}

void dissassemble(FILE *src, FILE *dst) {
    fprintf(dst, "bits 16\n\n");
    while (!feof(src)) {
        u8 byte1 = fgetc(src);
        u8 byte2 = fgetc(src);

        if ((byte1 & 0b11111100) == 0b10001000) {
            bool wide = byte1 & 0b1;
            bool direction = (byte1 & 0b10) >> 1;

            u8 mod =  byte2 & 0b11000000;
            u8 reg = (byte2 & 0b00111000) >> 3;
            u8 rm  =  byte2 & 0b00000111;
            if (mod == 0b11000000) {
                const char *reg_name = lookup_reg_name(reg, wide);
                const char *rm_name  = lookup_reg_name(rm, wide);
                if (direction) {
                    fprintf(dst, "mov %s, %s\n", reg_name, rm_name);
                } else {
                    fprintf(dst, "mov %s, %s\n", rm_name, reg_name);
                }
            } else {
                printf("ERROR: Unsupported mod '%d'", mod);
                abort();
            }
        }
    }
}