
const registers = {}
for (const reg of ["ax", "bx", "cx", "dx", "sp", "bp", "si", "di", "ip"]) {
	registers[reg] = {
		set: Module.cwrap(`cpu_set_${reg}`, null, ["number"]),
		get: Module.cwrap(`cpu_get_${reg}`, "number", [])
	}
}

const decodeInstAt = Module.cwrap("decode_inst_at", null, ["number", "number", "number"])
function getCurrentInstructionAt(address) {
	let text = undefined
	const inst = Module._malloc(64)
	const size = decodeInstAt(address, inst, 64)
	if (size > 0) {
		text = Module.AsciiToString(inst)
	}
	Module._free(inst)
	return [text, size]
}

const setMemoryState = Module.cwrap("set_memory_state", null, ["array", "number", "number"])
function setMemoryAt(address, value) {
	setMemoryBufferAt(address, [value])
}

function setMemoryBufferAt(address, buffer) {
	setMemoryState(buffer, buffer.length, address)
}

const getMemoryBaseAddress = Module.cwrap("get_memory_state_base", null, [])
const getMemorySize = Module.cwrap("get_memory_state_size", null, [])
function getMemory() {
	return new Uint8Array(wasmMemory.buffer, getMemoryBaseAddress(), getMemorySize())
}

const resetCPU = Module.cwrap("reset_cpu", null, [])
const stepCPU = Module.cwrap("step", null, [])
