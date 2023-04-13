#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#define u32 uint32_t
#define i32 int32_t
#define u16 uint16_t
#define i16 int16_t
#define u8  uint8_t
#define i8  int8_t

#define panic(...) fprintf(stderr, "ABORT(%s:%d): ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); abort()
#define todo(...) fprintf(stderr, "TODO(%s:%d): ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); abort()
#define dbg(...) printf("; "); printf(__VA_ARGS__); printf("\n")
#define ARRAY_LEN(arr) sizeof(arr) / sizeof(arr[0])

// TODO: find a way to merge "to/from register" with "to/from accumulator" branches into a single code path

enum decode_error {
    DECODE_OK,
    DECODE_ERR_EOF,
    DECODE_ERR_MISSING_BYTES,
    DECODE_ERR_UNKNOWN_OP,
};

enum operation {
    OP_MOV,
    OP_ADD,
    OP_SUB,
    OP_CMP,
    OP_JE, OP_JL, OP_JLE, OP_JB, OP_JBE,
    OP_JP,
    OP_JO,
    OP_JS,
    OP_JNE, OP_JNL, OP_JNLE, OP_JNB, OP_JNBE,
    OP_JNP,
    OP_JNO,
    OP_JNS,
    OP_LOOP,
    OP_LOOPZ,
    OP_LOOPNZ,
    OP_JCXZ,
    __OP_COUNT
};
const char *operation_str[__OP_COUNT] = {
    "mov", "add", "sub", "cmp", "je", "jl", "jle", "jb", "jbe", "jp", "jo",
    "js", "jne", "jnl","jnle", "jnb", "jnbe", "jnp", "jno", "jns", "loop",
    "loopz", "loopnz", "jcxz"
};
const enum operation cond_jmp_lookup[16] = {
    [0b0100] = OP_JE,
    [0b1100] = OP_JL,
    [0b1110] = OP_JLE,
    [0b0010] = OP_JB,
    [0b0110] = OP_JBE,
    [0b1010] = OP_JP,
    [0b0000] = OP_JO,
    [0b1000] = OP_JS,
    [0b0101] = OP_JNE,
    [0b1101] = OP_JNL,
    [0b1111] = OP_JNLE,
    [0b0011] = OP_JNB,
    [0b0111] = OP_JNBE,
    [0b1011] = OP_JNP,
    [0b0001] = OP_JNO,
    [0b1001] = OP_JNS
};
const enum operation cond_loop_jmp_lookup[4] = {
    [0b10] = OP_LOOP,
    [0b01] = OP_LOOPZ,
    [0b00] = OP_LOOPNZ,
    [0b11] = OP_JCXZ
};

// Order and place of these `enum reg_value` enums is IMPORTANT! Don't rearrange!
enum reg_value {
    REG_AL, REG_CL, REG_DL, REG_BL, REG_AH, REG_CH, REG_DH, REG_BH,
    REG_AX, REG_CX, REG_DX, REG_BX, REG_SP, REG_BP, REG_SI, REG_DI,
    __REG_COUNT
};
const char *reg_value_str[__REG_COUNT] = {
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

// Order and place of these `enum mem_base` enums is IMPORTANT! Don't rearrange!
enum mem_base {
    MEM_BASE_BX_SI,
    MEM_BASE_BX_DI,
    MEM_BASE_BP_SI,
    MEM_BASE_BP_DI,
    MEM_BASE_SI,
    MEM_BASE_DI,
    MEM_BASE_BP,
    MEM_BASE_BX,
    MEM_BASE_DIRECT_ADDRESS,
    __MEM_BASE_COUNT
};
const char *mem_base_str[8] = {
    "bx + si",
    "bx + di",
    "bp + si",
    "bp + di",
    "si",
    "di",
    "bp",
    "bx"
};

struct mem_value {
    enum mem_base base;
    i16 disp;
    // IMPORTANT! Keep in mind that `disp` should be interpreted as `u16`, if `base == MEM_BASE_DIRECT_ADDRESS`
};

struct reg_or_mem_value {
    bool is_reg;
    union {
        enum reg_value reg;
        struct mem_value mem;
    };
};

enum src_value_variant {
    SRC_VALUE_REG,
    SRC_VALUE_MEM,
    SRC_VALUE_immediate8,
    SRC_VALUE_immediate16
};

struct src_value {
    enum src_value_variant variant;
    union {
        enum reg_value reg;
        struct mem_value mem;
        u16 immediate;
    };
};

struct instruction {
    enum operation op;
    struct reg_or_mem_value dest;
    struct src_value src;
    i8 jmp_offset;
};

static const char *reg_to_str(enum reg_value reg) {
    assert(0 <= reg && reg <= __REG_COUNT);
    return reg_value_str[reg];
}

static void mem_to_str(char *buff, size_t max_size, struct mem_value *mem) {
    assert(0 <= mem->base && mem->base <= __MEM_BASE_COUNT);
    if (mem->base == MEM_BASE_DIRECT_ADDRESS) {
        snprintf(buff, max_size, "[%d]", (u16)mem->disp);
    } else if (mem->disp > 0) {
        snprintf(buff, max_size, "[%s + %d]", mem_base_str[mem->base], mem->disp);
    } else if (mem->disp < 0) {
        snprintf(buff, max_size, "[%s - %d]", mem_base_str[mem->base], -mem->disp);
    } else {
        snprintf(buff, max_size, "[%s]", mem_base_str[mem->base]);
    }
}

static void reg_or_mem_to_str(char *buff, size_t max_size, struct reg_or_mem_value *value) {
    if (value->is_reg) {
        strncpy(buff, reg_to_str(value->reg), max_size);
    } else {
        mem_to_str(buff, max_size, &value->mem);
    }
}

static void src_to_str(char *buff, size_t max_size, struct src_value *value) {
    switch (value->variant)
    {
    case SRC_VALUE_REG:
        strncpy(buff, reg_to_str(value->reg), max_size);
        break;
    case SRC_VALUE_MEM:
        mem_to_str(buff, max_size, &value->mem);
        break;
    case SRC_VALUE_immediate16:
        snprintf(buff, max_size, "%d", value->immediate);
        break;
    case SRC_VALUE_immediate8:
        snprintf(buff, max_size, "%d", (u8)value->immediate);
        break;
    }
}

static const char *operation_to_str(enum operation op) {
    assert(0 <= op && op <= __OP_COUNT);
    return operation_str[op];
}

static void instruction_to_str(char *buff, size_t max_size, struct instruction *inst) {
    switch (inst->op)
    {
    case OP_MOV:
    case OP_CMP:
    case OP_SUB:
    case OP_ADD: {
        char dest[32];
        char src[32];
        const char *opcode = operation_to_str(inst->op);
        reg_or_mem_to_str(dest, sizeof(dest), &inst->dest);
        src_to_str(src, sizeof(src), &inst->src);

        bool is_dest_mem = !inst->dest.is_reg;
        if (is_dest_mem && inst->src.variant == SRC_VALUE_immediate16) {
            snprintf(buff, max_size, "%s %s, word %s", opcode, dest, src);
        } else if (is_dest_mem && inst->src.variant == SRC_VALUE_immediate8) {
            snprintf(buff, max_size, "%s %s, byte %s", opcode, dest, src);
        } else {
            snprintf(buff, max_size, "%s %s, %s", opcode, dest, src);
        }
        break;
    }
    case OP_JE:
    case OP_JL:
    case OP_JLE:
    case OP_JB:
    case OP_JBE:
    case OP_JP:
    case OP_JO:
    case OP_JS:
    case OP_JNE:
    case OP_JNL:
    case OP_JNLE:
    case OP_JNB:
    case OP_JNBE:
    case OP_JNP:
    case OP_JNO:
    case OP_LOOP:
    case OP_LOOPZ:
    case OP_LOOPNZ:
    case OP_JCXZ:
    case OP_JNS: {
        const char *opcode = operation_to_str(inst->op);
        i8 offset = inst->jmp_offset+2;
        if (offset >= 0) {
            snprintf(buff, max_size, "%s $+%d", opcode, offset);
        } else {
            snprintf(buff, max_size, "%s $%d", opcode, offset);
        }
        break;
    }
    default:
        panic("Invalid instruction opcode %d\n", inst->op);
    }
}

static i16 extend_sign_bit(i8 number) {
    if (number & 0b10000000) {
        return number | (0b11111111 << 8);
    } else {
        return number;
    }
}

const char *decode_error_to_str(enum decode_error err) {
    switch (err)
    {
    case DECODE_OK:
        return "ok";
    case DECODE_ERR_EOF:
        return "EOF";
    case DECODE_ERR_MISSING_BYTES:
        return "Decoder expected more bytes, but hit EOF";
    case DECODE_ERR_UNKNOWN_OP:
        return "Unable to decode opcode from byte";
    default:
        return "<unknown>";
    }
}

// This function assumes that the `enum reg_value` values are in a convenient order, for conversion.
// Look at "Table 4-9. REG (Register) Field Encoding" for more details
static enum reg_value decode_reg(u8 reg, bool wide) {
    return reg + (u8)(wide) * 8;
}

// This function assumes that the `enum mem_base` values are in a convenient order, for conversion.
// Look at "Table 4-10. R/M (Register/Memory) Field Encoding" for more details
static enum mem_base decode_mem_base(u8 rm) {
    return rm;
}

// Table 4-10. R/M (Register/Memory) Field Encoding
static void decode_reg_or_mem(struct reg_or_mem_value *value, FILE *src, u8 rm, u8 mod, bool wide) {
    if (mod == 0b11) { // Mod = 0b11, register
        value->is_reg = true;
        value->reg = decode_reg(rm, wide);
    } else if (mod == 0b10) { // Mod = 0b10, memory with i16 displacement
        i16 displacement = fgetc(src) | (fgetc(src) << 8);
        value->is_reg = false;
        value->mem.base = decode_mem_base(rm);
        value->mem.disp = displacement;
    } else if (mod == 0b01) { // Mod = 0b01, memory with i8 displacement
        i8 displacement = fgetc(src);
        value->is_reg = false;
        value->mem.base = decode_mem_base(rm);
        value->mem.disp = extend_sign_bit(displacement);
    } else if (mod == 0b00) { // Mod = 0b00, memory no displacement (most of the time)
        value->is_reg = false;
        if (rm == 0b110) { // Direct address
            u16 address = fgetc(src) | (fgetc(src) << 8);
            value->mem.base = MEM_BASE_DIRECT_ADDRESS;
            value->mem.disp = address;
        } else {
            value->mem.base = decode_mem_base(rm);
            value->mem.disp = 0;
        }
    } else {
        panic("unknown 'mod' value: %d\n", mod);
    }
}

static void deocde_reg_or_mem_to_src(struct src_value *value, FILE *src, u8 rm, u8 mod, bool wide) {
    struct reg_or_mem_value reg_or_mem;
    decode_reg_or_mem(&reg_or_mem, src, rm, mod, wide);
    if (reg_or_mem.is_reg) {
        value->variant = SRC_VALUE_REG;
        value->reg = reg_or_mem.reg;
    } else {
        value->variant = SRC_VALUE_MEM;
        value->mem = reg_or_mem.mem;
    }
}

// TODO: add handling for 'DECODE_ERR_MISSING_BYTES'
// Handy reference: Table 4-12. 8086 Instruction Encoding
enum decode_error decode_instruction(FILE *src, struct instruction *output) {
    u8 byte1 = fgetc(src);
    if (feof(src)) return DECODE_ERR_EOF;

    // MOVE: Register memory to/from register
    if ((byte1 & 0b11111100) == 0b10001000) {
        u8 byte2 = fgetc(src);
        bool wide = byte1 & 0b1;
        bool direction = (byte1 & 0b10) >> 1;

        u8 mod = (byte2 & 0b11000000) >> 6;
        u8 reg = (byte2 & 0b00111000) >> 3;
        u8 rm  =  byte2 & 0b00000111;

        output->op = OP_MOV;
        if (direction) {
            output->dest.is_reg = true;
            output->dest.reg = decode_reg(reg, wide);
            deocde_reg_or_mem_to_src(&output->src, src, rm, mod, wide);
        } else {
            output->src.variant = SRC_VALUE_REG;
            output->src.reg = decode_reg(reg, wide);
            decode_reg_or_mem(&output->dest, src, rm, mod, wide);
        }

    // MOVE: Immediate to register
    } else if ((byte1 & 0b11110000) == 0b10110000) {
        bool wide = (byte1 & 0b1000) >> 3;
        u8 reg = byte1 & 0b111;

        output->op = OP_MOV;
        output->dest.is_reg = true;
        output->dest.reg = decode_reg(reg, wide);

        if (wide) {
            output->src.variant = SRC_VALUE_immediate16;
            output->src.immediate = fgetc(src) | (fgetc(src) << 8);
        } else {
            output->src.variant = SRC_VALUE_immediate8;
            output->src.immediate = fgetc(src);
        }

    // MOVE: Immediate to register/memory
    } else if ((byte1 & 0b11111110) == 0b11000110) {
        u8 byte2 = fgetc(src);

        bool wide = byte1 & 0b1;
        u8 mod = (byte2 & 0b11000000) >> 6;
        u8 rm  = byte2 & 0b00000111;

        output->op = OP_MOV;
        decode_reg_or_mem(&output->dest, src, rm, mod, wide);

        if (wide) {
            output->src.variant = SRC_VALUE_immediate16;
            output->src.immediate = fgetc(src) | (fgetc(src) << 8);
        } else {
            output->src.variant = SRC_VALUE_immediate8;
            output->src.immediate = fgetc(src);
        }

    // MOVE: Memory to accumulator
    } else if ((byte1 & 0b11111110) == 0b10100000) {
        output->op = OP_MOV;
        output->dest.is_reg = true;
        output->dest.reg = REG_AX;
        output->src.variant = SRC_VALUE_MEM;
        output->src.mem.base = MEM_BASE_DIRECT_ADDRESS;

        bool wide = byte1 & 0b1;
        if (wide) {
            output->src.mem.disp = fgetc(src) | (fgetc(src) << 8);
        } else {
            output->src.mem.disp = fgetc(src);
        }

    // MOVE: Accumulator to memory
    } else if ((byte1 & 0b11111110) == 0b10100010) {
        bool wide = byte1 & 0b1;

        output->op = OP_MOV;
        output->src.variant = SRC_VALUE_REG;
        output->src.reg = wide ? REG_AX : REG_AL;
        output->dest.is_reg = false;
        output->dest.mem.base = MEM_BASE_DIRECT_ADDRESS;

        if (wide) {
            output->dest.mem.disp = fgetc(src) | (fgetc(src) << 8);
        } else {
            output->dest.mem.disp = fgetc(src);
        }

    // ADD/SUB/CMP: Reg/memory with register to either
    } else if ((byte1 & 0b11000100) == 0b00000000) {
        u8 variant = (byte1 & 0b00111000) >> 3;
        if (variant == 0b000) {
            output->op = OP_ADD;
        } else if (variant == 0b101) {
            output->op = OP_SUB;
        } else if (variant == 0b111) {
            output->op = OP_CMP;
        }

        bool wide      =  byte1 & 0b01;
        bool direction = (byte1 & 0b10) >> 1;

        u8 byte2 = fgetc(src);
        u8 mod = (byte2 & 0b11000000) >> 6;
        u8 reg = (byte2 & 0b00111000) >> 3;
        u8 rm  =  byte2 & 0b00000111;

        if (direction) {
            output->dest.is_reg = true;
            output->dest.reg = decode_reg(reg, wide);
            deocde_reg_or_mem_to_src(&output->src, src, rm, mod, wide);
        } else {
            output->src.variant = SRC_VALUE_REG;
            output->src.reg = decode_reg(reg, wide);
            decode_reg_or_mem(&output->dest, src, rm, mod, wide);
        }

    // ADD/SUB/CMP: immediate with register/memory
    } else if ((byte1 & 0b11111100) == 0b10000000) {
        u8 byte2 = fgetc(src);
        u8 variant = (byte2 & 0b00111000) >> 3;

        if (variant == 0b000) {
            output->op = OP_ADD;
        } else if (variant == 0b101) {
            output->op = OP_SUB;
        } else if (variant == 0b111) {
            output->op = OP_CMP;
        }

        bool wide        =  byte1 & 0b01;
        bool sign_extend = (byte1 & 0b10) >> 1;
        u8 mod = (byte2 & 0b11000000) >> 6;
        u8 rm  = byte2 & 0b00000111;

        decode_reg_or_mem(&output->dest, src, rm, mod, wide);

        if (wide) {
            output->src.variant = SRC_VALUE_immediate16;
            if (sign_extend) {
                output->src.immediate = fgetc(src);
                output->src.immediate = extend_sign_bit(output->src.immediate);
            } else {
                output->src.immediate = fgetc(src) | (fgetc(src) << 8);
            }
        } else {
            output->src.variant = SRC_VALUE_immediate8;
            output->src.immediate = fgetc(src);
        }

    // ADD/SUB/CMP: immediate with accumulator
    } else if ((byte1 & 0b11000110) == 0b00000100) {
        bool wide = byte1 & 0b1;

        output->dest.is_reg = true;
        output->dest.reg = wide ? REG_AX : REG_AL;

        u8 variant = (byte1 & 0b00111000) >> 3;
        if (variant == 0b000) {
            output->op = OP_ADD;
        } else if (variant == 0b101) {
            output->op = OP_SUB;
        } else if (variant == 0b111) {
            output->op = OP_CMP;
        }

        if (wide) {
            output->src.variant = SRC_VALUE_immediate16;
            output->src.immediate = fgetc(src) | (fgetc(src) << 8);
        } else {
            output->src.variant = SRC_VALUE_immediate8;
            output->src.immediate = fgetc(src);
        }

    // Conditional jumps
    } else if ((byte1 & 0b11110000) == 0b01110000) {
        i8 jmp_offset = fgetc(src);
        u8 opcode = byte1 & 0b00001111;
        output->op = cond_jmp_lookup[opcode];
        output->jmp_offset = jmp_offset;

    // Conditional jumps
    } else if ((byte1 & 0b11111100) == 0b11100000) {
        i8 jmp_offset = fgetc(src);
        u8 opcode = byte1 & 0b00000011;
        output->op = cond_loop_jmp_lookup[opcode];
        output->jmp_offset = jmp_offset;

    } else {
        return DECODE_ERR_UNKNOWN_OP;
    }

    return DECODE_OK;
}

int dissassemble(FILE *src, FILE *dst) {
    fprintf(dst, "bits 16\n\n");

    char buff[256];
    struct instruction inst;
    int counter = 1;
    while (true) {
        enum decode_error err = decode_instruction(src, &inst);
        if (err == DECODE_ERR_EOF) break;
        if (err != DECODE_OK) {
            fprintf(stderr, "ERROR: Failed to decode %d instruction: %s\n", counter, decode_error_to_str(err));
            return -1;
        }

        instruction_to_str(buff, sizeof(buff), &inst);
        fprintf(dst, buff);
        fprintf(dst, "\n");
        counter += 1;
    }

    return 0;
}