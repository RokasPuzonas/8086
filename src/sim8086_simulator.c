
u16 read_reg_value(struct cpu_state *state, enum reg_value reg)
{
    switch (reg)
    {
    case REG_AL: return state->ax & 0xFF;
    case REG_CL: return state->cx & 0xFF;
    case REG_DL: return state->dx & 0xFF;
    case REG_BL: return state->bx & 0xFF;
    case REG_AH: return (state->ax >> 8) & 0xFF;
    case REG_CH: return (state->cx >> 8) & 0xFF;
    case REG_DH: return (state->dx >> 8) & 0xFF;
    case REG_BH: return (state->bx >> 8) & 0xFF;
    case REG_AX: return state->ax;
    case REG_CX: return state->cx;
    case REG_DX: return state->dx;
    case REG_BX: return state->bx;
    case REG_SP: return state->sp;
    case REG_BP: return state->bp;
    case REG_SI: return state->si;
    case REG_DI: return state->di;
    default: panic("Unhandled register '%s'", reg_to_str(reg));
    }
}

void write_reg_value(struct cpu_state *state, enum reg_value reg, u16 value)
{
    switch (reg)
    {
    case REG_AL:
        state->ax = (state->ax & 0xFF00) & value;
        break;
    case REG_CL:
        state->cx = (state->cx & 0xFF00) & value;
        break;
    case REG_DL:
        state->dx = (state->dx & 0xFF00) & value;
        break;
    case REG_BL:
        state->bx = (state->bx & 0xFF00) & value;
        break;
    case REG_AH:
        state->ax = (state->ax & 0x00FF) & (value << 8);
        break;
    case REG_CH:
        state->cx = (state->cx & 0x00FF) & (value << 8);
        break;
    case REG_DH:
        state->dx = (state->dx & 0x00FF) & (value << 8);
        break;
    case REG_BH:
        state->bx = (state->bx & 0x00FF) & (value << 8);
        break;
    case REG_AX:
        state->ax = value;
        break;
    case REG_CX:
        state->cx = value;
        break;
    case REG_DX:
        state->dx = value;
        break;
    case REG_BX:
        state->bx = value;
        break;
    case REG_SP:
        state->sp = value;
        break;
    case REG_BP:
        state->bp = value;
        break;
    case REG_SI:
        state->si = value;
        break;
    case REG_DI:
        state->di = value;
        break;
    default:
        panic("Unhandled register '%s'", reg_to_str(reg));
    }
}

u16 read_src_value(struct cpu_state *state, struct src_value *src) {
    switch (src->variant)
    {
        case SRC_VALUE_REG:
            return read_reg_value(state, src->reg);
        case SRC_VALUE_IMMEDIATE8:
        case SRC_VALUE_IMMEDIATE16:
            return src->immediate;
        case SRC_VALUE_MEM:
            todo("Handle read from memory");
        default:
            panic("Unhandled src variant %d\n", src->variant);
    }
}

u16 read_src_or_mem_value(struct cpu_state *state, struct reg_or_mem_value *reg_or_mem) {
    if (reg_or_mem->is_reg) {
        return read_reg_value(state, reg_or_mem->reg);
    } else {
        todo("Handle read from memory");
    }
}
void write_src_or_mem_value(struct cpu_state *state, struct reg_or_mem_value *reg_or_mem, u16 value) {
    if (reg_or_mem->is_reg) {
        write_reg_value(state, reg_or_mem->reg, value);
    } else {
        todo("Handle write to memory");
    }
}

bool is_reg_16bit(enum reg_value reg) {
    switch (reg)
    {
    case REG_AL:
    case REG_CL:
    case REG_DL:
    case REG_BL:
    case REG_AH:
    case REG_CH:
    case REG_DH:
    case REG_BH:
        return false;
    case REG_AX:
    case REG_CX:
    case REG_DX:
    case REG_BX:
    case REG_SP:
    case REG_BP:
    case REG_SI:
    case REG_DI:
        return true;
    default: panic("Unhandled register '%s'", reg_to_str(reg));
    }
}

bool are_instruction_operands_16bit(struct instruction *inst) {
    if (inst->dest.is_reg) {
        return is_reg_16bit(inst->dest.reg);
    } else if (inst->src.variant == SRC_VALUE_REG) {
        return is_reg_16bit(inst->src.reg);
    } else {
        return inst->src.variant == SRC_VALUE_IMMEDIATE16;
    }
}

void execute_instruction(struct cpu_state *state, struct instruction *inst) {
    switch (inst->op)
    {
    case OP_MOV: {
        u16 src_value = read_src_value(state, &inst->src);
        write_src_or_mem_value(state, &inst->dest, src_value);
        break;
    }
    case OP_ADD: {
        u16 dest_value = read_src_or_mem_value(state, &inst->dest);
        u16 src_value = read_src_value(state, &inst->src);
        u16 result = dest_value + src_value;

        state->flags.zero = result == 0;
        if (are_instruction_operands_16bit(inst)) {
            state->flags.sign = (result >> 15) & 0b1;
        } else {
            state->flags.sign = (result >> 7) & 0b1;
        }

        write_src_or_mem_value(state, &inst->dest, result);
        break;
    }
    case OP_SUB: {
        u16 dest_value = read_src_or_mem_value(state, &inst->dest);
        u16 src_value = read_src_value(state, &inst->src);
        u16 result = dest_value - src_value;

        state->flags.zero = result == 0;
        if (are_instruction_operands_16bit(inst)) {
            state->flags.sign = (result >> 15) & 0b1;
        } else {
            state->flags.sign = (result >> 7) & 0b1;
        }

        write_src_or_mem_value(state, &inst->dest, result);
        break;
    }
    case OP_CMP: {
        u16 dest_value = read_src_or_mem_value(state, &inst->dest);
        u16 src_value = read_src_value(state, &inst->src);
        u16 result = dest_value - src_value;

        state->flags.zero = result == 0;
        if (are_instruction_operands_16bit(inst)) {
            state->flags.sign = (result >> 15) & 0b1;
        } else {
            state->flags.sign = (result >> 7) & 0b1;
        }
        break;
    }
    case OP_JNE: {
        if (!state->flags.zero) {
            i8 jmp_offset = inst->jmp_offset;
            state->ip += jmp_offset;
        }

        break;
    } default:
        todo("Unhandled instruction execution '%s'\n", operation_to_str(inst->op));
    }
}