#ifndef ACIR_H
#define ACIR_H

#include <annec_anchor.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

// O(NAME, MNEMONIC, OPERAND_COUNT, SIGNATURE, DESCRIPTION)
#define ACIR_OPCODES_ENUM(O, S) \
  O(SET, <b>, 2, ".T T, wT", "init binding") S \
  O(RET, ret, 1, ".T T", "return, must be last in block") S \
  O(REF, ref, 2, ".T lT, w*T", "address of, reference") S \
  O(DER, der, 2, ".T *T, wT", "dereference") S \
  O(ADD, add, 3, ".T T, T, wT", "add two numbers") S \
  O(SUB, sub, 3, ".T T, T, wT", "substract two numbers") S \
  O(MUL, mul, 3, ".T T, T, wT", "multiply two numbers") S \
  O(DIV, div, 3, ".T T, T, wT", "divide two numbers") S \
  O(MOD, mod, 3, ".T T, T, wT", "\"modulo\" two numbers") S \
  O(NEG, neg, 2, ".T T, wT", "negate number") S \
  O(INC, inc, 1, ".T wT", "increment number") S \
  O(DEC, dec, 1, ".T wT", "decrement number") S \
  O(EQL, eql, 3, ".T T, T, wbool", "check if two numbers are equal") S \
  O(NEQ, neq, 3, ".T T, T, wbool", "inverse of EQL") S \
  O(LTH, lth, 3, ".T T, T, wbool", "check if a number is less") S \
  O(GTH, gth, 3, ".T T, T, wbool", "check if a number is greater") S \
  O(LEQ, leq, 3, ".T T, T, wbool", "check if a number is less or equal") S \
  O(GEQ, geq, 3, ".T T, T, wbool", "check if a number is greater or equal") S \
  O(AND, and, 3, " bool, bool, wbool", "\"and\" booleans") S \
  O(COR, cor, 3, " bool, bool, wbool", "\"or\" booleans") S \
  O(XOR, xor, 3, " bool, bool, wbool", "\"xor\" booleans") S \
  O(NOT, not, 2, " bool, wbool", "negate boolean") S \
  O(BIT_AND, bitand, 3, " bool, bool, wbool", "bitwise \"and\" booleans") S \
  O(BIT_COR, bitcor, 3, " bool, bool, wbool", "bitwise \"or\" booleans") S \
  O(BIT_XOR, bitxor, 3, " bool, bool, wbool", "bitwise \"xor\" booleans") S \
  O(BIT_NOT, bitnot, 2, " bool, wbool", "bitwise negate boolean")

typedef uint8_t AcirOpcode;

#define COMMA ,
#define TMP(NAME, MNEMONIC, OPERAND_COUNT, SIGNATURE, DESCRIPTION) ACIR_OPCODE_##NAME
enum AcirOpcodes { ACIR_OPCODES_ENUM(TMP, COMMA), ACIR_OPCODE_MAX_ };
#undef TMP
#undef COMMA

const char *AcirOpcode_Name(AcirOpcode opcode);
const char *AcirOpcode_Mnemonic(AcirOpcode opcode);
int AcirOpcode_OperandCount(AcirOpcode opcode);
const char *AcirOpcode_Signature(AcirOpcode opcode);
const char *AcirOpcode_Description(AcirOpcode opcode);


// O(NAME, MNEMONIC, DESCRIPTION)
#define ACIR_BASIC_VALUE_TYPES_ENUM(O, S) \
  O(SINT64, sint64, "signed 64-bit integer") S \
  O(UINT64, uint64, "unsigned 64-bit integer") S \
  O(SINT32, sint32, "signed 32-bit integer") S \
  O(UINT32, uint32, "unsigned 32-bit integer") S \
  O(SINT16, sint16, "signed 16-bit integer") S \
  O(UINT16, uint16, "unsigned 16-bit integer") S \
  O(SINT8, sint8, "signed 8-bit integer") S \
  O(UINT8, uint8, "unsigned 8-bit integer") S \
  O(FLOAT32, float32, "single-precision float") S \
  O(FLOAT64, float64, "double-precision float") S \
  O(BOOL, bool, "boolean type") S \
  O(VOID, void, "void type")

typedef uint8_t AcirBasicValueType;

#define COMMA ,
#define TMP(NAME, MNEMONIC, DESCRIPTION) ACIR_BASIC_VALUE_TYPE_##NAME
enum AcirBasicValueTypes { ACIR_BASIC_VALUE_TYPES_ENUM(TMP, COMMA), ACIR_BASIC_VALUE_TYPE_MAX_ };
#undef TMP
#undef COMMA

const char *AcirBasicValueType_Name(AcirBasicValueType type);
const char *AcirBasicValueType_Mnemonic(AcirBasicValueType type);
const char *AcirBasicValueType_Description(AcirBasicValueType type);

typedef struct AcirValueType AcirValueType;

typedef struct {
  AcirValueType *returnType;
  size_t argumentCount;
  AcirValueType *argumentTypes[];
} AcirFunctionValueType;

struct AcirValueType {
  enum {
    ACIR_VALUE_TYPE_BASIC,
    ACIR_VALUE_TYPE_POINTER,
    ACIR_VALUE_TYPE_FUNCTION,
  } type;
  union {
    AcirBasicValueType basic;
    const AcirValueType *pointer;
    const AcirFunctionValueType *function;
  };
};

bool AcirBasicValueType_Equals(const AcirValueType *self, const AcirValueType *other);

typedef uint8_t AcirOperandType;
enum AcirOperandTypes {
  ACIR_OPERAND_TYPE_IMMEDIATE,
  ACIR_OPERAND_TYPE_BINDING,
};

typedef enum {
  ACIR_OPERAND_KIND_READABLE,
  ACIR_OPERAND_KIND_WRITABLE,
} AcirOperandKind;

typedef struct {
  const AcirValueType *type;
  union {
    uint64_t uint64;
    int64_t sint64;
    uint32_t uint32;
    int32_t sint32;
    uint16_t uint16;
    int16_t sint16;
    uint8_t uint8;
    int8_t sint8;
    float float32;
    double float64;
    bool boolean;
  };
} AcirImmediateValue;

typedef size_t AcirVariableValue;

typedef struct {
  AcirOperandType type; 
  union {
    AcirImmediateValue imm;
    AcirVariableValue idx;
  };
} AcirOperand;

const char *AcirOperandType_Name(AcirOperandType type);

#define ACIR_INSTR_INDEX_LAST (~(size_t)0)

typedef struct AcirInstr AcirInstr;
struct AcirInstr {
  size_t index;
  AcirOpcode opcode;
  const AcirValueType *type;
  size_t next;
  AcirOperand out;
  union {
    struct { AcirOperand lhs, rhs; };
    AcirOperand val;
  };
};

// TODO: swap arguments around
void AcirInstr_Print(AnchCharWriteStream *out, const AcirInstr *self);
void AcirOperand_Print(AnchCharWriteStream *out, const AcirOperand *self);
void AcirValueType_Print(AnchCharWriteStream *out, const AcirValueType *self);

typedef struct {
  const AcirValueType *type;
  const AcirInstr *code;
  const char *name;
  size_t instrCount;
  const AcirInstr *instrs;
} AcirFunction;

int AcirFunction_Validate(AcirFunction *self, AnchAllocator *allocator);

typedef struct {
  AcirFunction *target;
  AnchAllocator *allocator;
  AcirInstr *instrs;
} AcirBuilder;

void AcirBuilder_Init(AcirBuilder *self, AcirFunction *target, AnchAllocator *allocator);
void AcirBuilder_Free(AcirBuilder *self);
AcirInstr *AcirBuilder_Add(AcirBuilder *self, size_t index);

typedef size_t AcirOptimizerBindingFlags;
enum AcirOptimizerBindingFlags {
  ACIR_OPTIMIZER_BINDING_FLAG_NONE = 0,
  ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT = 1 << 0,
  ACIR_OPTIMIZER_BINDING_FLAG_MAX_ = 0xFFFFFFFFFFFFFFFF,
};

typedef struct {
  bool exists;
  AcirOptimizerBindingFlags flags;
  AcirImmediateValue constant;
} AcirOptimizer_Binding;

typedef struct {
  bool exists;
  bool used;
  const AcirInstr *prev;
} AcirOptimizer_Instr;

typedef struct {
  const AcirFunction *source;
  AcirBuilder *builder;
  size_t instrCount;
  AcirOptimizer_Instr *instrs;
  size_t bindingCount;
  AcirOptimizer_Binding *bindings;
  AnchAllocator *allocator;
  bool didAnalyze;
} AcirOptimizer;

typedef struct {
  const AcirFunction *source;
  AcirBuilder *builder;
  AnchAllocator *allocator;
} AcirOptimizer_InitInfo;

void AcirOptimizer_Init(AcirOptimizer *self, const AcirOptimizer_InitInfo *info);
void AcirOptimizer_Free(AcirOptimizer *self);
void AcirOptimizer_Analyze(AcirOptimizer *self);
void AcirOptimizer_ConstantFold(AcirOptimizer *self);
void AcirOptimizer_DeadCode(AcirOptimizer *self);

#endif
