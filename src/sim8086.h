#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>

#define u32 uint32_t
#define i32 int32_t
#define u16 uint16_t
#define i16 int16_t
#define u8  uint8_t
#define i8  int8_t

#define panic(...) fprintf(stderr, "PANIC(%s:%d): ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); abort()
#define todo(...) fprintf(stderr, "TODO(%s:%d): ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); abort()
#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))
#define MEMORY_SIZE 65536 // 2^16

enum operation {
    OP_MOV,
    OP_ADD,
    OP_SUB,
    OP_CMP,
    OP_JE,
    OP_JL,
    OP_JLE,
    OP_JB,
    OP_JBE,
    OP_JP,
    OP_JO,
    OP_JS,
    OP_JNE,
    OP_JNL,
    OP_JNLE,
    OP_JNB,
    OP_JNBE,
    OP_JNP,
    OP_JNO,
    OP_JNS,
    OP_LOOP,
    OP_LOOPZ,
    OP_LOOPNZ,
    OP_JCXZ,
    __OP_COUNT
};

// Order and place of these `enum reg_value` enums is IMPORTANT! Don't rearrange!
enum reg_value {
    REG_AL, REG_CL, REG_DL, REG_BL, REG_AH, REG_CH, REG_DH, REG_BH,
    REG_AX, REG_CX, REG_DX, REG_BX, REG_SP, REG_BP, REG_SI, REG_DI,
    __REG_COUNT
};

// Order and place of these `enum mem_base` enums is IMPORTANT! Don't rearrange!
enum mem_base {
    MEM_BASE_BX_SI,
    MEM_BASE_BX_DI,
    MEM_BASE_BP_SI,
    MEM_BASE_BP_DI,
    MEM_BASE_SI,
    MEM_BASE_DI,
    MEM_BASE_BP,
    MEM_BASE_BX,
    MEM_BASE_DIRECT_ADDRESS,
    __MEM_BASE_COUNT
};

struct mem_value {
    enum mem_base base;
    union {
        i16 disp;
        u16 direct_address;
    };
};

struct reg_or_mem_value {
    bool is_reg;
    union {
        enum reg_value reg;
        struct mem_value mem;
    };
};

enum src_value_variant {
    SRC_VALUE_REG,
    SRC_VALUE_MEM,
    SRC_VALUE_IMMEDIATE8,
    SRC_VALUE_IMMEDIATE16
};

struct src_value {
    enum src_value_variant variant;
    union {
        enum reg_value reg;
        struct mem_value mem;
        u16 immediate;
    };
};

// TODO: Store "wide" flag on instruction, it is useful to know when doing most operations
struct instruction {
    enum operation op;
    struct reg_or_mem_value dest;
    struct src_value src;
    i8 jmp_offset;
};

struct memory {
    u8 mem[MEMORY_SIZE];
};

struct cpu_state {
    u16 ax;
    u16 bx;
    u16 cx;
    u16 dx;
    u16 sp;
    u16 bp;
    u16 si;
    u16 di;

    struct {
        bool zero;
        bool sign;
        // TODO: Add all flags
    } flags;

    u16 ip;
};