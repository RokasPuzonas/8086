#include <emscripten.h>
#include <emscripten/emscripten.h>

#define SIM8086_EMCC
#include "sim8086/prelude.h"

#define EXPORT EMSCRIPTEN_KEEPALIVE
#define dbg(...) emscripten_log(EM_LOG_CONSOLE, __VA_ARGS__)

EXPORT struct memory memory_state;
EXPORT struct cpu_state cpu_state;

EXPORT void step() {
    struct instruction inst;
	enum decode_error err = decode_instruction(&memory_state, &cpu_state.ip, &inst);
	if (err == DECODE_OK) {
		execute_instruction(&memory_state, &cpu_state, &inst);
	}
}

EXPORT void reset_cpu() {
	memset(&cpu_state, 0, sizeof(cpu_state));
}

/* -------------------- Decoder ----------------------- */

EXPORT u16 decode_inst_at(u16 addr, char *buff, size_t buff_size) {
	struct instruction inst;
	u16 after_addr = addr;
	enum decode_error err = decode_instruction(&memory_state, &after_addr, &inst);
	if (err == DECODE_OK) {
		instruction_to_str(buff, buff_size, &inst);
		return after_addr - addr;
	} else {
		return 0;
	}
}

/* -------------------- Memory ----------------------- */

EXPORT int set_memory_state(u8 *buffer, u16 buffer_size, u16 start) {
    return load_mem_from_buff(&memory_state, buffer, buffer_size, start);
}

EXPORT u8 *get_memory_state_base() {
    return memory_state.mem;
}

EXPORT size_t get_memory_state_size() {
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
