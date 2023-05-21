
static const char *reg_to_str(enum reg_value reg) {
    const char *reg_value_str[__REG_COUNT] = {
        "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
        "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
    };

    assert(0 <= reg && reg <= __REG_COUNT);
    return reg_value_str[reg];
}

static void mem_to_str(char *buff, size_t max_size, struct mem_value *mem) {
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
    switch (value->variant) {
    case SRC_VALUE_REG:
        strncpy(buff, reg_to_str(value->reg), max_size);
        break;
    case SRC_VALUE_MEM:
        mem_to_str(buff, max_size, &value->mem);
        break;
    case SRC_VALUE_IMMEDIATE16:
        snprintf(buff, max_size, "%d", value->immediate);
        break;
    case SRC_VALUE_IMMEDIATE8:
        snprintf(buff, max_size, "%d", (u8)value->immediate);
        break;
    }
}

static const char *operation_to_str(enum operation op) {
    const char *operation_str[__OP_COUNT] = {
        "mov", "add", "sub", "cmp", "je", "jl", "jle", "jb", "jbe", "jp", "jo",
        "js", "jne", "jnl","jnle", "jnb", "jnbe", "jnp", "jno", "jns", "loop",
        "loopz", "loopnz", "jcxz"
    };

    assert(0 <= op && op <= __OP_COUNT);
    return operation_str[op];
}

static void instruction_to_str(char *buff, size_t max_size, struct instruction *inst) {
    switch (inst->op) {
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
        if (is_dest_mem && inst->src.variant == SRC_VALUE_IMMEDIATE16) {
            snprintf(buff, max_size, "%s %s, word %s", opcode, dest, src);
        } else if (is_dest_mem && inst->src.variant == SRC_VALUE_IMMEDIATE8) {
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
