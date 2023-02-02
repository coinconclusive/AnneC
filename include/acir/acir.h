#ifndef ACIR_H
#define ACIR_H

#include <annec_anchor.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

// O(NAME, MNEMONIC, OPERAND_COUNT, SIGNATURE, DESCRIPTION, CONSTEVAL)
#define ACIR_OPCODES_ENUM(O, S) \
  O(SET, <b>, 2, ".T!void T, wT", "init binding", true) S \
  O(RET, ret, 1, ".T T", "return, must be last in block", false) S \
  O(REF, ref, 2, ".T lT, w*T", "address of, reference", false) S \
  O(DER, der, 2, ".T!void *T, wT", "dereference", false) S \
  O(ADD, add, 3, ".T!void T, T, wT", "add two numbers", true) S \
  O(SUB, sub, 3, ".T!void T, T, wT", "substract two numbers", true) S \
  O(MUL, mul, 3, ".T!void T, T, wT", "multiply two numbers", true) S \
  O(DIV, div, 3, ".T!void T, T, wT", "divide two numbers", true) S \
  O(MOD, mod, 3, ".T!void!float32!float64 T, T, wT", "\"modulo\" two numbers", true) S \
  O(NEG, neg, 2, ".T!void T, wT", "negate number", true) S \
  O(EQL, eql, 3, ".T!void T, T, wbool", "check if two numbers are equal", true) S \
  O(NEQ, neq, 3, ".T!void T, T, wbool", "inverse of EQL", true) S \
  O(LTH, lth, 3, ".T!void T, T, wbool", "check if a number is less", true) S \
  O(GTH, gth, 3, ".T!void T, T, wbool", "check if a number is greater", true) S \
  O(LEQ, leq, 3, ".T!void T, T, wbool", "check if a number is less or equal", true) S \
  O(GEQ, geq, 3, ".T!void T, T, wbool", "check if a number is greater or equal", true) S \
  O(AND, and, 3, " bool, bool, wbool", "\"and\" booleans", true) S \
  O(COR, cor, 3, " bool, bool, wbool", "\"or\" booleans", true) S \
  O(XOR, xor, 3, " bool, bool, wbool", "\"xor\" booleans", true) S \
  O(NOT, not, 2, " bool, wbool", "negate boolean", true) S \
  O(BIT_AND, bitand, 3, ".T!float32!float64!void T, T, wT", "bitwise \"and\" booleans", true) S \
  O(BIT_COR, bitcor, 3, ".T!float32!float64!void T, T, wT", "bitwise \"or\" booleans", true) S \
  O(BIT_XOR, bitxor, 3, ".T!float32!float64!void T, T, wT", "bitwise \"xor\" booleans", true) S \
  O(BIT_NOT, bitnot, 2, ".T!float32!float64!void T, wT", "bitwise negate boolean", true) S \
  O(EFF, eff, 2, ".T any, wT", "perform side effect", false)

typedef uint8_t AcirOpcode;

#define COMMA ,
#define TMP(NAME, ...) ACIR_OPCODE_##NAME
enum AcirOpcodes { ACIR_OPCODES_ENUM(TMP, COMMA), ACIR_OPCODE_MAX_ };
#undef TMP
#undef COMMA

const char *AcirOpcode_Name(AcirOpcode opcode);
const char *AcirOpcode_Mnemonic(AcirOpcode opcode);
int AcirOpcode_OperandCount(AcirOpcode opcode);
const char *AcirOpcode_Signature(AcirOpcode opcode);
const char *AcirOpcode_Description(AcirOpcode opcode);
bool AcirOpcode_ConstEval(AcirOpcode opcode);

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

#define ACIR_INSTR_NULL_INDEX ((size_t)-1)

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

// TODO: move these to a local header file? maybe just impl file?

typedef size_t AcirOptimizerBindingFlags;
enum AcirOptimizerBindingFlags {
  ACIR_OPTIMIZER_BINDING_FLAG_NONE = 0,
  ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT = 1 << 0,
  ACIR_OPTIMIZER_BINDING_FLAG_MAX_ = 0xFFFFFFFFFFFFFFFF,
};

typedef struct {
  bool exists, used;
  AcirOptimizerBindingFlags flags;
  AcirImmediateValue constant;
} AcirOptimizer_Binding;

typedef struct {
  bool exists;
  bool used;
  size_t prev;
} AcirOptimizer_Instr;

typedef struct {
  const AcirFunction *source;
  AcirBuilder *builder;
  size_t instrCount;
  AcirOptimizer_Instr *instrs;
  size_t bindingCount;
  AcirOptimizer_Binding *bindings;
  AnchAllocator *allocator;
  size_t lastInstr;
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
