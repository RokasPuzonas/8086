#define dbg(...) printf("; "); printf(__VA_ARGS__); printf("\n")

// TODO: find a way to merge "to/from register" with "to/from accumulator" branches into a single code path

enum decode_error {
    DECODE_OK,
    DECODE_ERR_EOF,
    DECODE_ERR_MISSING_BYTES,
    DECODE_ERR_UNKNOWN_OP,
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

static i16 extend_sign_bit(i8 number) {
    if (number & 0b10000000) {
        return number | (0b11111111 << 8);
    } else {
        return number;
    }
}

const char *decode_error_to_str(enum decode_error err) {
    switch (err) {
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
static void decode_reg_or_mem(
        struct reg_or_mem_value *value,
        struct memory *mem,
        u16 *addr,
        u8 rm,
        u8 mod,
        bool wide
    ) {
    if (mod == 0b11) { // Mod = 0b11, register
        value->is_reg = true;
        value->reg = decode_reg(rm, wide);
    } else if (mod == 0b10) { // Mod = 0b10, memory with i16 displacement
        i16 displacement = pull_u16_at(mem, addr);
        value->is_reg = false;
        value->mem.base = decode_mem_base(rm);
        value->mem.disp = displacement;
    } else if (mod == 0b01) { // Mod = 0b01, memory with i8 displacement
        i8 displacement = pull_u8_at(mem, addr);
        value->is_reg = false;
        value->mem.base = decode_mem_base(rm);
        value->mem.disp = extend_sign_bit(displacement);
    } else if (mod == 0b00) { // Mod = 0b00, memory no displacement (most of the time)
        value->is_reg = false;
        if (rm == 0b110) { // Direct address
            u16 address = pull_u16_at(mem, addr);
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

static void deocde_reg_or_mem_to_src(
        struct src_value *value,
        struct memory *mem,
        u16 *addr,
        u8 rm,
        u8 mod,
        bool wide
    ) {
    struct reg_or_mem_value reg_or_mem;
    decode_reg_or_mem(&reg_or_mem, mem, addr, rm, mod, wide);
    if (reg_or_mem.is_reg) {
        value->variant = SRC_VALUE_REG;
        value->reg = reg_or_mem.reg;
    } else {
        value->variant = SRC_VALUE_MEM;
        value->mem = reg_or_mem.mem;
    }
}

// TODO: change to readinf from a byte buffer
// TODO: add handling for 'DECODE_ERR_MISSING_BYTES'
// Handy reference: Table 4-12. 8086 Instruction Encoding
enum decode_error decode_instruction(struct memory *mem, u16 *addr, struct instruction *output) {
    u8 byte1 = pull_u8_at(mem, addr);

    // MOVE: Register memory to/from register
    if ((byte1 & 0b11111100) == 0b10001000) {
        u8 byte2 = pull_u8_at(mem, addr);
        bool wide = byte1 & 0b1;
        bool direction = (byte1 & 0b10) >> 1;

        u8 mod = (byte2 & 0b11000000) >> 6;
        u8 reg = (byte2 & 0b00111000) >> 3;
        u8 rm  =  byte2 & 0b00000111;

        output->op = OP_MOV;
        if (direction) {
            output->dest.is_reg = true;
            output->dest.reg = decode_reg(reg, wide);
            deocde_reg_or_mem_to_src(&output->src, mem, addr, rm, mod, wide);
        } else {
            output->src.variant = SRC_VALUE_REG;
            output->src.reg = decode_reg(reg, wide);
            decode_reg_or_mem(&output->dest, mem, addr, rm, mod, wide);
        }

    // MOVE: Immediate to register
    } else if ((byte1 & 0b11110000) == 0b10110000) {
        bool wide = (byte1 & 0b1000) >> 3;
        u8 reg = byte1 & 0b111;

        output->op = OP_MOV;
        output->dest.is_reg = true;
        output->dest.reg = decode_reg(reg, wide);

        if (wide) {
            output->src.variant = SRC_VALUE_IMMEDIATE16;
            output->src.immediate = pull_u16_at(mem, addr);
        } else {
            output->src.variant = SRC_VALUE_IMMEDIATE8;
            output->src.immediate = pull_u8_at(mem, addr);
        }


    // MOVE: Immediate to register/memory
    } else if ((byte1 & 0b11111110) == 0b11000110) {
        u8 byte2 = pull_u8_at(mem, addr);

        bool wide = byte1 & 0b1;
        u8 mod = (byte2 & 0b11000000) >> 6;
        u8 rm  = byte2 & 0b00000111;

        output->op = OP_MOV;
        decode_reg_or_mem(&output->dest, mem, addr, rm, mod, wide);

        if (wide) {
            output->src.variant = SRC_VALUE_IMMEDIATE16;
            output->src.immediate = pull_u16_at(mem, addr);
        } else {
            output->src.variant = SRC_VALUE_IMMEDIATE8;
            output->src.immediate = pull_u8_at(mem, addr);
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
            output->src.mem.disp = pull_u16_at(mem, addr);
        } else {
            output->src.mem.disp = pull_u8_at(mem, addr);
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
            output->dest.mem.disp = pull_u16_at(mem, addr);
        } else {
            output->dest.mem.disp = pull_u8_at(mem, addr);
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

        u8 byte2 = pull_u8_at(mem, addr);
        u8 mod = (byte2 & 0b11000000) >> 6;
        u8 reg = (byte2 & 0b00111000) >> 3;
        u8 rm  =  byte2 & 0b00000111;

        if (direction) {
            output->dest.is_reg = true;
            output->dest.reg = decode_reg(reg, wide);
            deocde_reg_or_mem_to_src(&output->src, mem, addr, rm, mod, wide);
        } else {
            output->src.variant = SRC_VALUE_REG;
            output->src.reg = decode_reg(reg, wide);
            decode_reg_or_mem(&output->dest, mem, addr, rm, mod, wide);
        }

    // ADD/SUB/CMP: immediate with register/memory
    } else if ((byte1 & 0b11111100) == 0b10000000) {
        u8 byte2 = pull_u8_at(mem, addr);
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

        decode_reg_or_mem(&output->dest, mem, addr, rm, mod, wide);

        if (wide) {
            output->src.variant = SRC_VALUE_IMMEDIATE16;
            if (sign_extend) {
                output->src.immediate = pull_u8_at(mem, addr);
                output->src.immediate = extend_sign_bit(output->src.immediate);
            } else {
                output->src.immediate = pull_u16_at(mem, addr);
            }
        } else {
            output->src.variant = SRC_VALUE_IMMEDIATE8;
            output->src.immediate = pull_u8_at(mem, addr);
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
            output->src.variant = SRC_VALUE_IMMEDIATE16;
            output->src.immediate = pull_u16_at(mem, addr);
        } else {
            output->src.variant = SRC_VALUE_IMMEDIATE8;
            output->src.immediate = pull_u8_at(mem, addr);
        }

    // Conditional jumps
    } else if ((byte1 & 0b11110000) == 0b01110000) {
        i8 jmp_offset = pull_u8_at(mem, addr);
        u8 opcode = byte1 & 0b00001111;
        output->op = cond_jmp_lookup[opcode];
        output->jmp_offset = jmp_offset;

    // Conditional loop jumps
    } else if ((byte1 & 0b11111100) == 0b11100000) {
        i8 jmp_offset = pull_u8_at(mem, addr);
        u8 opcode = byte1 & 0b00000011;
        output->op = cond_loop_jmp_lookup[opcode];
        output->jmp_offset = jmp_offset;

    } else {
        return DECODE_ERR_UNKNOWN_OP;
    }

    return DECODE_OK;
}
