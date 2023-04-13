#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "os.h"
#include <errno.h>
#include "asm.c"

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

void print_usage(const char *program) {
	fprintf(stderr, "Usage: %s <test|dump> <file>\n", program);
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

int main(int argc, char **argv) {
	if (argc != 3) {
		print_usage(argv[0]);
		return -1;
	}

	if (strequal(argv[1], "test")) {
		char *asm_file = argv[2];
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

	} else if (strequal(argv[1], "dump")) {
		char *input_file = argv[2];

		if (strendswith(input_file, ".asm")) {
			char bin_filename[MAX_PATH_SIZE];
			get_tmp_file(bin_filename, "nasm_output");

			if (compile_asm(input_file, bin_filename)) {
				printf("ERROR: Failed to compile '%s'", input_file);
				return -1;
			}

			FILE *assembly = fopen(bin_filename, "rb");
			if (assembly == NULL) {
				printf("ERROR: Opening file '%s': %d\n", bin_filename, errno);
				return -1;
			}
			dissassemble(assembly, stdout);
			fclose(assembly);

			remove(bin_filename);
		} else {
			FILE *assembly = fopen(input_file, "r");
			if (assembly == NULL) {
				printf("ERROR: Opening file '%s': %d\n", input_file, errno);
				return -1;
			}
			dissassemble(assembly, stdout);
			fclose(assembly);
		}

	} else {
		print_usage(argv[0]);
		return -1;
	}

	return 0;
}
