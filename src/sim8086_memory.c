#include <stdio.h>

// TODO: add error codes

int load_mem_from_stream(struct memory *mem, FILE *stream, u32 start) {
    u32 offset = 0;
    while (true) {
        u8 byte = fgetc(stream);
        if (feof(stream)) break;
        if (start + offset >= MEMORY_SIZE) return -1;
        mem->mem[start + offset] = byte;
        offset++;
    }
    return offset;
}

int load_mem_from_file(struct memory *mem, const char *filename, u32 start) {
    FILE *stream = fopen(filename, "rb");
    if (stream == NULL) {
        return -1;
    }
    int byte_count = load_mem_from_stream(mem, stream, start);
    fclose(stream);
    return byte_count;
}

// TODO: Make this error some kind of error, when reading past end
u8 read_byte_at(struct memory *mem, u32 address) {
    return mem->mem[address % MEMORY_SIZE];
}

u8 pull_byte_at(struct memory *mem, u32 *address) {
    u8 byte = read_byte_at(mem, *address);
    (*address)++;
    return byte;
}