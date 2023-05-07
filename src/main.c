#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "os.h"
#include "sim8086.h"

#include "sim8086.c"
#include "sim8086_memory.c"
#include "sim8086_decoder.c"
#include "sim8086_simulator.c"

// TODO: refactor cli commands, there is a lot of repeating code for reading assemblies and compiling them.

#define strequal(a, b) strcmp(a, b) == 0

const char *get_tmp_dir() {
#ifdef IS_WINDOWS
	char *dir;
    if ((dir = getenv("TMPDIR")) != NULL) return dir;
    if ((dir = getenv("TEMP"))   != NULL) return dir;
    if ((dir = getenv("TMP"))    != NULL) return dir;
	return NULL;
#elif IS_LINUX
	return "/tmp";
#endif
}

int strendswith(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

char *strdup(const char *src) {
    char *dst = malloc(strlen (src) + 1);
    if (dst == NULL) return NULL;
    strcpy(dst, src);
    return dst;
}

void get_tmp_file(char *filename, const char *prefix) {
	const char *dir = get_tmp_dir();
	sprintf(filename, "%s\\%sXXXXXX", dir, prefix);
	int fd = mkstemp(filename);
	close(fd);
}

int compile_asm(const char *src, const char *dst) {
	char command[512] = { 0 };
	snprintf(command, sizeof(command), "nasm \"%s\" -O0 -o \"%s\"", src, dst);
	return system(command);
}

int compare_files(const char *expected, const char *gotten) {
	int rc = -1;
	FILE *f1 = fopen(expected, "r");
	FILE *f2 = fopen(gotten, "r");

	int i = 0;
	while (!feof(f1) && !feof(f2)) {
		int byte1 = fgetc(f1);
		int byte2 = fgetc(f2);
		if (byte1 != byte2) {
			printf("Mismatch byte at %d, expected %d got %d\n", i, byte1, byte2);
			goto err;
		}
		i++;
	}

	if (!feof(f1) || !feof(f2)) {
		goto err;
	}

	rc = 0;
err:
	fclose(f1);
	fclose(f2);
	return rc;
}

int dissassemble(FILE *src, FILE *dst) {
    fprintf(dst, "bits 16\n\n");

	struct memory mem = { 0 };
	int byte_count = load_mem_from_stream(&mem, src, 0);
	if (byte_count == -1) {
		fprintf(stderr, "ERROR: Failed to load file to memory\n");
		return -1;
	}

    char buff[256];
    struct instruction inst;
	u16 inst_address = 0;
    while (inst_address < byte_count) {
        enum decode_error err = decode_instruction(&mem, &inst_address, &inst);
        if (err == DECODE_ERR_EOF) break;
        if (err != DECODE_OK) {
            fprintf(stderr, "ERROR: Failed to decode instruction at 0x%08x: %s\n", inst_address, decode_error_to_str(err));
            return -1;
        }

        instruction_to_str(buff, sizeof(buff), &inst);
        fprintf(dst, buff);
        fprintf(dst, "\n");
    }

    return 0;
}

int simulate(FILE *src, struct memory *mem) {
	int byte_count = load_mem_from_stream(mem, src, 0);
	if (byte_count == -1) {
		fprintf(stderr, "ERROR: Failed to load file to memory\n");
		return -1;
	}

	struct cpu_state state = { 0 };
    struct instruction inst;
    while (state.ip < byte_count) {
        enum decode_error err = decode_instruction(mem, &state.ip, &inst);
        if (err == DECODE_ERR_EOF) break;
        if (err != DECODE_OK) {
            fprintf(stderr, "ERROR: Failed to decode instruction at 0x%08x: %s\n", state.ip, decode_error_to_str(err));
            return -1;
        }
		execute_instruction(mem, &state, &inst);
    }

	printf("Final registers:\n");
	printf("      ax: 0x%04x (%d)\n", state.ax, state.ax);
	printf("      bx: 0x%04x (%d)\n", state.bx, state.bx);
	printf("      cx: 0x%04x (%d)\n", state.cx, state.cx);
	printf("      dx: 0x%04x (%d)\n", state.dx, state.dx);
	printf("      sp: 0x%04x (%d)\n", state.sp, state.sp);
	printf("      bp: 0x%04x (%d)\n", state.bp, state.bp);
	printf("      si: 0x%04x (%d)\n", state.si, state.si);
	printf("      di: 0x%04x (%d)\n", state.di, state.di);
	printf("      ip: 0x%04x (%d)\n", state.ip, state.ip);
	printf("   flags: %s%s\n", state.flags.sign ? "S" : "", state.flags.zero ? "Z" : "");
	return 0;
}

int estimate_clocks(FILE *src) {
	struct memory mem = { 0 };
	int byte_count = load_mem_from_stream(&mem, src, 0);
	if (byte_count == -1) {
		fprintf(stderr, "ERROR: Failed to load file to memory\n");
		return -1;
	}

	u32 total_clocks = 0;
	char buff[256];
	struct cpu_state state = { 0 };
    struct instruction inst;
    while (state.ip < byte_count) {
        enum decode_error err = decode_instruction(&mem, &state.ip, &inst);
        if (err == DECODE_ERR_EOF) break;
        if (err != DECODE_OK) {
            fprintf(stderr, "ERROR: Failed to decode instruction at 0x%08x: %s\n", state.ip, decode_error_to_str(err));
            return -1;
        }

		execute_instruction(&mem, &state, &inst);

		u32 clocks = estimate_instruction_clocks(&inst);
		total_clocks += clocks;
		instruction_to_str(buff, sizeof(buff), &inst);
        printf("%s ; Clocks = %d (+%d)\n", buff, total_clocks, clocks);
    }

	return 0;
}

void print_usage(const char *program) {
	fprintf(stderr, "Usage: %s <command> ...\n", program);
	fprintf(stderr, "\ttest-dump <file.asm> - disassemble and test output\n");
	fprintf(stderr, "\tdump <file> - disassemble\n");
	fprintf(stderr, "\tsim <file> - simulate program\n");
	fprintf(stderr, "\tsim-dump <file> <output> - simulate program and dump memory to file\n");
	fprintf(stderr, "\tclocks <file> - output estimation of clocks\n");
}

int test_decoder(const char *asm_file) {
	if (!strendswith(asm_file, ".asm")) {
		printf("ERROR: Expected *.asm file, gotten '%s'", asm_file);
		return -1;
	}

	char *bin_filename = "test-input.o";
	if (compile_asm(asm_file, bin_filename)) {
		printf("ERROR: Failed to compile '%s'", asm_file);
		return -1;
	}

	FILE *bin_test = fopen(bin_filename, "rb");
	if (bin_test == NULL) {
		printf("ERROR: Opening file '%s': %d\n", bin_filename, errno);
		return -1;
	}

	char *dissassembly_filename = "test-dump.asm";
	FILE *dissassembly = fopen(dissassembly_filename, "wb+");
	if (dissassembly == NULL) {
		printf("ERROR: Opening file '%s': %d\n", dissassembly_filename, errno);
		return -1;
	}
	dissassemble(bin_test, dissassembly);
	fclose(dissassembly);

	char *dissassembly_dump_filename = "test-dump-asm.o";
	if (compile_asm(dissassembly_filename, dissassembly_dump_filename)) {
		printf("ERROR: Failed to compile '%s'", dissassembly_filename);
		return -1;
	}

	if (!compare_files(bin_filename, dissassembly_dump_filename)) {
		printf("Test success\n");
	} else {
		printf("Test failed\n");
	}

	return 0;
}

int dump_decompilation(const char *input) {
	if (strendswith(input, ".asm")) {
		char bin_filename[MAX_PATH_SIZE];
		get_tmp_file(bin_filename, "nasm_output");

		if (compile_asm(input, bin_filename)) {
			printf("ERROR: Failed to compile '%s'", input);
			return -1;
		}

		FILE *assembly = fopen(bin_filename, "rb");
		if (assembly == NULL) {
			printf("ERROR: Opening file '%s': %d\n", bin_filename, errno);
			remove(bin_filename);
			return -1;
		}
		dissassemble(assembly, stdout);
		fclose(assembly);

		remove(bin_filename);
	} else {
		FILE *assembly = fopen(input, "r");
		if (assembly == NULL) {
			printf("ERROR: Opening file '%s': %d\n", input, errno);
			return -1;
		}
		dissassemble(assembly, stdout);
		fclose(assembly);
	}

	return 0;
}

int run_simulation_with_memory(const char *input, struct memory *mem) {
	if (strendswith(input, ".asm")) {
		char bin_filename[MAX_PATH_SIZE];
		get_tmp_file(bin_filename, "nasm_output");

		if (compile_asm(input, bin_filename)) {
			printf("ERROR: Failed to compile '%s'", input);
			return -1;
		}

		FILE *assembly = fopen(bin_filename, "rb");
		if (assembly == NULL) {
			printf("ERROR: Opening file '%s': %d\n", bin_filename, errno);
			remove(bin_filename);
			return -1;
		}
		simulate(assembly, mem);
		fclose(assembly);

		remove(bin_filename);
	} else {
		FILE *assembly = fopen(input, "r");
		if (assembly == NULL) {
			printf("ERROR: Opening file '%s': %d\n", input, errno);
			return -1;
		}
		simulate(assembly, mem);
		fclose(assembly);
	}

	return 0;
}

int run_simulation(const char *input) {
	struct memory mem = { 0 };
	return run_simulation_with_memory(input, &mem);
}

int run_simulation_and_dump(const char *input, char const *output) {
	struct memory mem = { 0 };
	int rc = run_simulation_with_memory(input, &mem);
	if (rc) return rc;

	FILE *output_file = fopen(output, "wb");
	if (output_file == NULL) {
		return -1;
	}

	int written = fwrite(mem.mem, sizeof(u8), MEMORY_SIZE, output_file);
	if (written != MEMORY_SIZE) {
		fclose(output_file);
		return -1;
	}

	return fclose(output_file);
}

int run_estimate_clocks(const char *input) {
	if (strendswith(input, ".asm")) {
		char bin_filename[MAX_PATH_SIZE];
		get_tmp_file(bin_filename, "nasm_output");

		if (compile_asm(input, bin_filename)) {
			printf("ERROR: Failed to compile '%s'", input);
			return -1;
		}

		FILE *assembly = fopen(bin_filename, "rb");
		if (assembly == NULL) {
			printf("ERROR: Opening file '%s': %d\n", bin_filename, errno);
			remove(bin_filename);
			return -1;
		}
		estimate_clocks(assembly);
		fclose(assembly);

		remove(bin_filename);
	} else {
		return -1;
	}

	return 0;
}

int main(int argc, char **argv) {
	if (argc <= 2) {
		print_usage(argv[0]);
		return -1;
	}

	if (strequal(argv[1], "test-dump") && argc == 3) {
		return test_decoder(argv[2]);

	} else if (strequal(argv[1], "dump") && argc == 3) {
		return dump_decompilation(argv[2]);

	} else if (strequal(argv[1], "sim") && argc == 3) {
		return run_simulation(argv[2]);

	} else if (strequal(argv[1], "sim-dump") && argc == 4) {
		return run_simulation_and_dump(argv[2], argv[3]);

	} else if (strequal(argv[1], "clocks") && argc == 3) {
		return run_estimate_clocks(argv[2]);

	} else {
		print_usage(argv[0]);
		return -1;
	}
}
