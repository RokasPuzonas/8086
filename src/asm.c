#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#define u32 uint32_t
#define u16 uint16_t
#define u8  uint8_t
#define i16 int16_t
#define i8  int8_t

#define todo(...) fprintf(stderr, "TODO(%s:%d): ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); abort()
#define dbg(...) printf("; "); printf(__VA_ARGS__); printf("\n")

const char *wide_reg_names[8]     = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
const char *non_wide_reg_names[8] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };

// eac - Effective address calculation
const char *eac_names[8] = {
    "bx + si",
    "bx + di",
    "bp + si",
    "bp + di",
    "si",
    "di",
    "bp",
    "bx"
};

const char *lookup_reg_name(u8 idx, bool wide) {
    assert(0 <= idx && idx <= 7);
    if (wide) {
        return wide_reg_names[idx];
    } else {
        return non_wide_reg_names[idx];
    }
}

void snprintf_mod_with_disp(char *rm_name, size_t size, u8 rm, i16 displacement) {
    if (displacement < 0) {
        snprintf(rm_name, size, "[%s - %d]", eac_names[rm], -displacement);
    } else {
        snprintf(rm_name, size, "[%s + %d]", eac_names[rm], displacement);
    }
}

void get_rm_name(char *rm_name, size_t size, FILE *src, u8 rm, u8 mod, bool wide) {
    if (mod == 0b11000000) { // Mod = 0b11
        strcpy(rm_name, lookup_reg_name(rm, wide));
    } else if (mod == 0b10000000) { // Mod = 0b10
        i16 displacement = fgetc(src) | (fgetc(src) << 8);
        snprintf_mod_with_disp(rm_name, size, rm, displacement);
    } else if (mod == 0b01000000) { // Mod = 0b01
        i16 displacement = fgetc(src);
        if (displacement & 0b10000000) {
            displacement |= (0b11111111 << 8);
        }
        snprintf_mod_with_disp(rm_name, size, rm, displacement);
    } else if (mod == 0b00000000) { // Mod = 0b00
        if (rm == 0b110) { // Direct address
            u16 displacement = fgetc(src) | (fgetc(src) << 8);
            snprintf(rm_name, size, "[%d]", displacement);
        } else {
            snprintf(rm_name, size, "[%s]", eac_names[rm]);
        }
    }
}

// Handy reference: Table 4-12. 8086 Instruction Encoding
void dissassemble(FILE *src, FILE *dst) {
    fprintf(dst, "bits 16\n\n");
    while (true) {
        u8 byte1 = fgetc(src);
        if (feof(src)) break;

        // Register memory to/from register
        if ((byte1 & 0b11111100) == 0b10001000) {
            u8 byte2 = fgetc(src);
            bool wide = byte1 & 0b1;
            bool direction = (byte1 & 0b10) >> 1;

            u8 mod =  byte2 & 0b11000000;
            u8 reg = (byte2 & 0b00111000) >> 3;
            u8 rm  =  byte2 & 0b00000111;
            const char *reg_name = lookup_reg_name(reg, wide);
            char rm_name[24] = { 0 };
            get_rm_name(rm_name, sizeof(rm_name), src, rm, mod, wide);

            if (direction) {
                fprintf(dst, "mov %s, %s\n", reg_name, rm_name);
            } else {
                fprintf(dst, "mov %s, %s\n", rm_name, reg_name);
            }

        // Immediate to register
        } else if ((byte1 & 0b11110000) == 0b10110000) {
            bool wide = (byte1 & 0b1000) >> 3;
            u8 reg = byte1 & 0b111;

            u16 immediate;
            if (wide) {
                immediate = fgetc(src) | (fgetc(src) << 8);
            } else {
                immediate = fgetc(src);
            }

            fprintf(dst, "mov %s, %d\n", lookup_reg_name(reg, wide), immediate);

        // Immediate to memory
        } else if ((byte1 & 0b11111110) == 0b11000110) {
            u8 byte2 = fgetc(src);

            bool wide = byte1 & 0b1;
            u8 mod = byte2 & 0b11000000;
            u8 rm  = byte2 & 0b00000111;
            char rm_name[24] = { 0 };
            get_rm_name(rm_name, sizeof(rm_name), src, rm, mod, wide);

            u16 immediate;
            if (wide) {
                immediate = fgetc(src) | (fgetc(src) << 8);
                fprintf(dst, "mov %s, word %d\n", rm_name, immediate);
            } else {
                immediate = fgetc(src);
                fprintf(dst, "mov %s, byte %d\n", rm_name, immediate);
            }

        // Memory to accumulator
        } else if ((byte1 & 0b11111110) == 0b10100000) {
            bool wide = byte1 & 0b1;

            u16 immediate;
            if (wide) {
                immediate = fgetc(src) | (fgetc(src) << 8);
            } else {
                immediate = fgetc(src);
            }

            fprintf(dst, "mov ax, [%d]\n", immediate);

        // Accumulator to memory
        } else if ((byte1 & 0b11111110) == 0b10100010) {
            bool wide = byte1 & 0b1;

            u16 immediate;
            if (wide) {
                immediate = fgetc(src) | (fgetc(src) << 8);
            } else {
                immediate = fgetc(src);
            }

            fprintf(dst, "mov [%d], ax\n", immediate);

        } else {
            todo("unhandled byte %d", byte1);
        }
    }
}