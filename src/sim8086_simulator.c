
u16 get_register_value(struct cpu_state *state, enum reg_value reg)
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

void set_register_value(struct cpu_state *state, enum reg_value reg, u16 value)
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

void execute_instruction(struct cpu_state *state, struct instruction *inst) {
    switch (inst->op)
    {
    case OP_MOV:
        if (!inst->dest.is_reg) {
            todo("Handle MOV to memory");
        }
        if (inst->src.variant == SRC_VALUE_MEM) {
            todo("Handle MOV from memory");
        }

        u16 src_value;
        if (inst->src.variant == SRC_VALUE_REG) {
            src_value = get_register_value(state, inst->src.reg);
        } else if (inst->src.variant == SRC_VALUE_IMMEDIATE8 || inst->src.variant == SRC_VALUE_IMMEDIATE16) {
            src_value = inst->src.immediate;
        }

        set_register_value(state, inst->dest.reg, src_value);
        break;
    default:
        todo("Unhandled instruction execution '%s'\n", operation_to_str(inst->op));
    }
}