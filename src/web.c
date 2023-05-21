#include <emscripten.h>

#define SIM8086_EMCC
#include "sim8086/prelude.h"

#define EXPORT EMSCRIPTEN_KEEPALIVE
#define dbg(...) emscripten_log(EM_LOG_CONSOLE, __VA_ARGS__)

EXPORT struct memory memory_state;
EXPORT struct cpu_state cpu_state;
EXPORT struct instruction current_instruction;

/* -------------------- Decoder ----------------------- */

EXPORT int decode_inst_at_ip() {
    return decode_instruction(&memory_state, &cpu_state.ip, &current_instruction);
}

EXPORT void get_inst_str(char *buff, size_t buff_size) {
    instruction_to_str(buff, buff_size, &current_instruction);
}

/* -------------------- Memory ----------------------- */

EXPORT int load_to_memory_state(u8 *assembly, u16 assembly_size, u16 start) {
    return load_mem_from_buff(&memory_state, assembly, assembly_size, start);
}

EXPORT u8 *get_memory_state_base() {
    return memory_state.mem;
}

EXPORT size_t get_memory_state_size(u8 *assembly, u16 assembly_size, u16 start) {
    return MEMORY_SIZE;
}

/* -------------------- CPU ----------------------- */

EXPORT void cpu_reset()
{
    memset(&cpu_state, 0, sizeof(cpu_state));
}

#define CPU_STATE_GETTER(field) EXPORT u16 cpu_get_##field() { return cpu_state.field; }
#define CPU_STATE_SETTER(field) EXPORT void cpu_set_##field(u16 value) { cpu_state.field = value; }
#define CPU_STATE_ACCESOR(field) CPU_STATE_SETTER(field) CPU_STATE_GETTER(field)

CPU_STATE_ACCESOR(ip)
CPU_STATE_ACCESOR(ax)
CPU_STATE_ACCESOR(bx)
CPU_STATE_ACCESOR(cx)
CPU_STATE_ACCESOR(dx)
CPU_STATE_ACCESOR(sp)
CPU_STATE_ACCESOR(bp)
CPU_STATE_ACCESOR(si)
CPU_STATE_ACCESOR(di)

EXPORT bool cpu_get_zero_flag()
{
    return cpu_state.flags.zero;
}

EXPORT bool cpu_get_sign_flag()
{
    return cpu_state.flags.sign;
}