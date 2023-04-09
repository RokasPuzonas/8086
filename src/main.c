#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "os.h"
#include "cpu.c"
#include <errno.h>

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

void get_tmp_file(char *filename, const char *prefix) {
	const char *dir = get_tmp_dir();
	sprintf(filename, "%s\\%sXXXXXX", dir, prefix);
	int fd = mkstemp(filename);
	close(fd);
}

int main() {
	char tmp_filename[MAX_PATH_SIZE];
	get_tmp_file(tmp_filename, "nasm_output");

	char *example_filename = "examples/many_register_mov.asm";

	char command[512] = { 0 };
	snprintf(command, sizeof(command), "nasm \"%s\" -o \"%s\"", example_filename, tmp_filename);
	if (system(command)) {
		printf("ERROR: Failed to compile '%s'", example_filename);
		return -1;
	}

	FILE *assembly = fopen(tmp_filename, "r");
	if (assembly == NULL) {
		printf("ERROR: Opening file '%s': %d\n", tmp_filename, errno);
		return -1;
	}
	dissassemble(assembly, stdout);
	fclose(assembly);

	remove(tmp_filename);

	return 0;
}
