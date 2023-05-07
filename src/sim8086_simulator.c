
u16 read_reg_value(struct cpu_state *cpu, enum reg_value reg)
{
    switch (reg) {
    case REG_AL: return cpu->ax & 0xFF;
    case REG_CL: return cpu->cx & 0xFF;
    case REG_DL: return cpu->dx & 0xFF;
    case REG_BL: return cpu->bx & 0xFF;
    case REG_AH: return (cpu->ax >> 8) & 0xFF;
    case REG_CH: return (cpu->cx >> 8) & 0xFF;
    case REG_DH: return (cpu->dx >> 8) & 0xFF;
    case REG_BH: return (cpu->bx >> 8) & 0xFF;
    case REG_AX: return cpu->ax;
    case REG_CX: return cpu->cx;
    case REG_DX: return cpu->dx;
    case REG_BX: return cpu->bx;
    case REG_SP: return cpu->sp;
    case REG_BP: return cpu->bp;
    case REG_SI: return cpu->si;
    case REG_DI: return cpu->di;
    default: panic("Unhandled register '%s'", reg_to_str(reg));
    }
}

void write_reg_value(struct cpu_state *cpu, enum reg_value reg, u16 value)
{
    switch (reg) {
    case REG_AL:
        cpu->ax = (cpu->ax & 0xFF00) & value;
        break;
    case REG_CL:
        cpu->cx = (cpu->cx & 0xFF00) & value;
        break;
    case REG_DL:
        cpu->dx = (cpu->dx & 0xFF00) & value;
        break;
    case REG_BL:
        cpu->bx = (cpu->bx & 0xFF00) & value;
        break;
    case REG_AH:
        cpu->ax = (cpu->ax & 0x00FF) & (value << 8);
        break;
    case REG_CH:
        cpu->cx = (cpu->cx & 0x00FF) & (value << 8);
        break;
    case REG_DH:
        cpu->dx = (cpu->dx & 0x00FF) & (value << 8);
        break;
    case REG_BH:
        cpu->bx = (cpu->bx & 0x00FF) & (value << 8);
        break;
    case REG_AX:
        cpu->ax = value;
        break;
    case REG_CX:
        cpu->cx = value;
        break;
    case REG_DX:
        cpu->dx = value;
        break;
    case REG_BX:
        cpu->bx = value;
        break;
    case REG_SP:
        cpu->sp = value;
        break;
    case REG_BP:
        cpu->bp = value;
        break;
    case REG_SI:
        cpu->si = value;
        break;
    case REG_DI:
        cpu->di = value;
        break;
    default:
        panic("Unhandled register '%s'", reg_to_str(reg));
    }
}

u16 read_mem_base_value(struct cpu_state *cpu, enum mem_base base) {
    switch (base) {
    case MEM_BASE_BX_SI: return read_reg_value(cpu, REG_BX) + read_reg_value(cpu, REG_SI);
    case MEM_BASE_BX_DI: return read_reg_value(cpu, REG_BX) + read_reg_value(cpu, REG_DI);
    case MEM_BASE_BP_SI: return read_reg_value(cpu, REG_BP) + read_reg_value(cpu, REG_SI);
    case MEM_BASE_BP_DI: return read_reg_value(cpu, REG_BP) + read_reg_value(cpu, REG_DI);
    case MEM_BASE_SI:    return read_reg_value(cpu, REG_SI);
    case MEM_BASE_DI:    return read_reg_value(cpu, REG_DI);
    case MEM_BASE_BP:    return read_reg_value(cpu, REG_BP);
    case MEM_BASE_BX:    return read_reg_value(cpu, REG_BX);
    default: return 0;
    }
}

u16 calculate_mem_address(struct cpu_state *cpu, struct mem_value *addr) {
    if (addr->base == MEM_BASE_DIRECT_ADDRESS) {
        return addr->direct_address;
    } else {
        return read_mem_base_value(cpu, addr->base) + addr->disp;
    }
}

u16 read_mem_value(struct memory *mem, struct cpu_state *cpu, struct mem_value *value, bool wide) {
    u16 addr = calculate_mem_address(cpu, value);

    return wide ? read_u16_at(mem, addr) : read_u8_at(mem, addr);
}

void write_mem_value(struct memory *mem, struct cpu_state *cpu, struct mem_value *location, u16 value, bool wide) {
    u16 addr = calculate_mem_address(cpu, location);

    if (wide) {
        write_u16_at(mem, addr, value);
    } else {
        write_u8_at(mem, addr, value);
    }
}

u16 read_src_value(struct memory *mem, struct cpu_state *cpu, struct src_value *src, bool wide) {
    switch (src->variant) {
        case SRC_VALUE_REG:
            return read_reg_value(cpu, src->reg);
        case SRC_VALUE_IMMEDIATE8:
        case SRC_VALUE_IMMEDIATE16:
            return src->immediate;
        case SRC_VALUE_MEM:
            return read_mem_value(mem, cpu, &src->mem, wide);
        default:
            panic("Unhandled src variant %d\n", src->variant);
    }
}

u16 read_reg_or_mem_value(struct memory *mem, struct cpu_state *cpu, struct reg_or_mem_value *reg_or_mem, bool wide) {
    if (reg_or_mem->is_reg) {
        return read_reg_value(cpu, reg_or_mem->reg);
    } else {
        return read_mem_value(mem, cpu, &reg_or_mem->mem, wide);
    }
}

void write_reg_or_mem_value(struct memory *mem, struct cpu_state *cpu, struct reg_or_mem_value *reg_or_mem, u16 value, bool wide) {
    if (reg_or_mem->is_reg) {
        write_reg_value(cpu, reg_or_mem->reg, value);
    } else {
        write_mem_value(mem, cpu, &reg_or_mem->mem, value, wide);
    }
}

bool is_reg_16bit(enum reg_value reg) {
    switch (reg) {
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
    } else if (inst->src.variant == SRC_VALUE_IMMEDIATE8) {
        return false;
    } else if (inst->src.variant == SRC_VALUE_IMMEDIATE16) {
        return true;
    } else {
        panic("Failed to determine instruction width\n");
    }
}

void update_sign_flag(struct cpu_state *cpu, struct instruction *inst, u16 result) {
    if (are_instruction_operands_16bit(inst)) {
        cpu->flags.sign = (result >> 15) & 0b1;
    } else {
        cpu->flags.sign = (result >> 7) & 0b1;
    }
}

void execute_instruction(struct memory *mem, struct cpu_state *cpu, struct instruction *inst) {
    switch (inst->op) {
    case OP_MOV: {
        bool wide = are_instruction_operands_16bit(inst);
        u16 src_value = read_src_value(mem, cpu, &inst->src, wide);
        write_reg_or_mem_value(mem, cpu, &inst->dest, src_value, wide);
        break;
    }
    case OP_ADD: {
        bool wide = are_instruction_operands_16bit(inst);
        u16 dest_value = read_reg_or_mem_value(mem, cpu, &inst->dest, wide);
        u16 src_value = read_src_value(mem, cpu, &inst->src, wide);
        u16 result = dest_value + src_value;

        cpu->flags.zero = result == 0;
        update_sign_flag(cpu, inst, result);

        write_reg_or_mem_value(mem, cpu, &inst->dest, result, wide);
        break;
    }
    case OP_SUB: {
        bool wide = are_instruction_operands_16bit(inst);
        u16 dest_value = read_reg_or_mem_value(mem, cpu, &inst->dest, wide);
        u16 src_value = read_src_value(mem, cpu, &inst->src, wide);
        u16 result = dest_value - src_value;

        cpu->flags.zero = result == 0;
        update_sign_flag(cpu, inst, result);

        write_reg_or_mem_value(mem, cpu, &inst->dest, result, wide);
        break;
    }
    case OP_CMP: {
        bool wide = are_instruction_operands_16bit(inst);
        u16 dest_value = read_reg_or_mem_value(mem, cpu, &inst->dest, wide);
        u16 src_value = read_src_value(mem, cpu, &inst->src, wide);
        u16 result = dest_value - src_value;

        cpu->flags.zero = result == 0;
        update_sign_flag(cpu, inst, result);
        break;
    }
    case OP_JNE: {
        if (!cpu->flags.zero) {
            cpu->ip += inst->jmp_offset;
        }

        break;
    } default:
        todo("Unhandled instruction execution '%s'\n", operation_to_str(inst->op));
    }
}

int estimate_ea_clocks(struct mem_value *value) {
    bool has_disp = value->disp != 0;
    switch (value->base)
    {
    case MEM_BASE_DIRECT_ADDRESS:
        return 6;
    case MEM_BASE_SI:
    case MEM_BASE_DI:
    case MEM_BASE_BP:
    case MEM_BASE_BX:
        return 5 + (has_disp ? 4 : 0);
    case MEM_BASE_BP_DI:
    case MEM_BASE_BX_SI:
        return 7 + (has_disp ? 4 : 0);
    case MEM_BASE_BX_DI:
    case MEM_BASE_BP_SI:
        return 8 + (has_disp ? 4 : 0);
    default:
        panic("Unhandled EA clocks estimation case '%d'\n", value->base);
    }
}

u32 estimate_instruction_clocks(struct instruction *inst) {
    switch (inst->op) {
    case OP_MOV: {
        bool is_src_memory = inst->src.variant == SRC_VALUE_MEM;
        bool is_dest_memory = !inst->dest.is_reg;
        bool is_src_accumulator  = inst->src.variant == SRC_VALUE_REG && inst->src.reg == REG_AX;
        bool is_dest_accumulator = inst->dest.is_reg && inst->dest.reg == REG_AX;
        bool is_src_reg = inst->src.variant == SRC_VALUE_REG;
        bool is_dest_reg = inst->dest.is_reg;
        bool is_src_immediate = inst->src.variant == SRC_VALUE_IMMEDIATE8 || inst->src.variant == SRC_VALUE_IMMEDIATE16;

        if ((is_src_accumulator && is_dest_memory) || (is_dest_accumulator && is_src_memory)) {
            return 10;
        } else if (is_src_reg && is_dest_reg) {
            return 2;
        } else if (is_dest_reg && is_src_memory) {
            return 8 + estimate_ea_clocks(&inst->src.mem);
        } else if (is_dest_memory && is_src_reg) {
            return 9 + estimate_ea_clocks(&inst->dest.mem);
        } else if (is_dest_reg && is_src_immediate) {
            return 4;
        } else if (is_dest_memory && is_src_immediate) {
            return 10 + estimate_ea_clocks(&inst->dest.mem);
        }

        break;
    }
    case OP_ADD: {
        bool is_src_memory = inst->src.variant == SRC_VALUE_MEM;
        bool is_dest_memory = !inst->dest.is_reg;
        bool is_src_reg = inst->src.variant == SRC_VALUE_REG;
        bool is_dest_reg = inst->dest.is_reg;
        bool is_dest_accumulator = inst->dest.is_reg && inst->dest.reg == REG_AX;
        bool is_src_immediate = inst->src.variant == SRC_VALUE_IMMEDIATE8 || inst->src.variant == SRC_VALUE_IMMEDIATE16;

        if (is_src_reg && is_dest_reg) {
            return 3;
        } else if (is_dest_reg && is_src_memory) {
            return 9 + estimate_ea_clocks(&inst->src.mem);
        } else if (is_dest_memory && is_src_reg) {
            return 16 + estimate_ea_clocks(&inst->dest.mem);
        } else if (is_dest_reg && is_src_immediate) {
            return 4;
        } else if (is_dest_memory && is_src_immediate) {
            return 17 + estimate_ea_clocks(&inst->dest.mem);
        } else if (is_dest_accumulator && is_src_immediate) {
            return 4;
        }

        break;
    } default:
        todo("Unhandled instruction estimation '%s'\n", operation_to_str(inst->op));
    }

    todo("Unhandled estimation variant '%s'\n", operation_to_str(inst->op));
}