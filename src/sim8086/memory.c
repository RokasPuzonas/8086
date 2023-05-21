// TODO: add error codes

int load_mem_from_buff(struct memory *mem, u8 *buff, u16 buff_size, u16 start)
{
    if (start + buff_size > MEMORY_SIZE) return -1;
    memcpy(mem->mem + start, buff, buff_size);
    return 0;
}

int load_mem_from_stream(struct memory *mem, FILE *stream, u16 start) {
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

int load_mem_from_file(struct memory *mem, const char *filename, u16 start) {
    FILE *stream = fopen(filename, "rb");
    if (stream == NULL) {
        return -1;
    }
    int byte_count = load_mem_from_stream(mem, stream, start);
    fclose(stream);
    return byte_count;
}

// TODO: Make this error some kind of error, when reading past end
u8 read_u8_at(struct memory *mem, u16 address) {
    return mem->mem[address % MEMORY_SIZE];
}

u16 read_u16_at(struct memory *mem, u16 address) {
    return read_u8_at(mem, address) | (read_u8_at(mem, address+1) << 8);
}

void write_u8_at(struct memory *mem, u16 address, u8 value) {
    mem->mem[address % MEMORY_SIZE] = value;
}

void write_u16_at(struct memory *mem, u16 address, u16 value) {
    write_u8_at(mem, address+0, (value >> 0) & 0xFF);
    write_u8_at(mem, address+1, (value >> 8) & 0xFF);
}

u16 pull_u16_at(struct memory *mem, u16 *address) {
    u16 value = read_u16_at(mem, *address);
    (*address) += 2;
    return value;
}

u8 pull_u8_at(struct memory *mem, u16 *address) {
    u8 byte = read_u8_at(mem, *address);
    (*address)++;
    return byte;
}